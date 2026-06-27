#include "TraktClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrl>

#include <algorithm>

namespace {
constexpr auto kBaseUrl = "https://api.trakt.tv";

QByteArray jsonBody(const QJsonObject &object)
{
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

QString apiErrorMessage(const QByteArray &payload, const QString &fallback)
{
    const QJsonObject object = QJsonDocument::fromJson(payload).object();
    const QString description = object.value(QStringLiteral("error_description")).toString().trimmed();
    if (!description.isEmpty()) {
        return description;
    }
    const QString error = object.value(QStringLiteral("error")).toString().trimmed();
    return error.isEmpty() ? fallback : error;
}

int httpStatus(QNetworkReply *reply)
{
    return reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
}

QString imdbId(const QJsonObject &ids)
{
    const QString id = ids.value(QStringLiteral("imdb")).toString().trimmed();
    return id.startsWith(QStringLiteral("tt")) ? id : QString();
}

QString contentId(const QJsonObject &ids)
{
    const QString imdb = imdbId(ids);
    if (!imdb.isEmpty()) {
        return imdb;
    }
    const int tmdb = ids.value(QStringLiteral("tmdb")).toInt();
    if (tmdb > 0) {
        return QStringLiteral("tmdb:%1").arg(tmdb);
    }
    const int trakt = ids.value(QStringLiteral("trakt")).toInt();
    if (trakt > 0) {
        return QStringLiteral("trakt:%1").arg(trakt);
    }
    return {};
}

qint64 pausedAtSeconds(const QJsonObject &entry)
{
    const QDateTime pausedAt = QDateTime::fromString(entry.value(QStringLiteral("paused_at")).toString(), Qt::ISODate);
    return pausedAt.isValid() ? pausedAt.toSecsSinceEpoch() : QDateTime::currentSecsSinceEpoch();
}

QVariantList playbackItemsFromJson(const QByteArray &payload, const QString &kind)
{
    QVariantList items;
    const QJsonArray entries = QJsonDocument::fromJson(payload).array();
    for (const QJsonValue &value : entries) {
        const QJsonObject entry = value.toObject();
        const double progress = entry.value(QStringLiteral("progress")).toDouble();
        if (progress <= 0.0 || progress >= 92.0) {
            continue;
        }

        QVariantMap item;
        if (kind == QStringLiteral("movie")) {
            const QJsonObject movie = entry.value(QStringLiteral("movie")).toObject();
            const QString id = contentId(movie.value(QStringLiteral("ids")).toObject());
            if (id.isEmpty()) {
                continue;
            }
            item.insert(QStringLiteral("type"), QStringLiteral("movie"));
            item.insert(QStringLiteral("id"), id);
            item.insert(QStringLiteral("baseId"), id);
            item.insert(QStringLiteral("name"), movie.value(QStringLiteral("title")).toString());
        } else {
            const QJsonObject show = entry.value(QStringLiteral("show")).toObject();
            const QJsonObject episode = entry.value(QStringLiteral("episode")).toObject();
            const QString baseId = contentId(show.value(QStringLiteral("ids")).toObject());
            const int season = episode.value(QStringLiteral("season")).toInt();
            const int number = episode.value(QStringLiteral("number")).toInt();
            if (baseId.isEmpty() || season <= 0 || number <= 0) {
                continue;
            }
            item.insert(QStringLiteral("type"), QStringLiteral("series"));
            item.insert(QStringLiteral("id"), baseId);
            item.insert(QStringLiteral("baseId"), baseId);
            item.insert(QStringLiteral("season"), season);
            item.insert(QStringLiteral("episode"), number);
            item.insert(QStringLiteral("name"), show.value(QStringLiteral("title")).toString());
            item.insert(QStringLiteral("episodeTitle"), episode.value(QStringLiteral("title")).toString());
        }

        item.insert(QStringLiteral("source"), QStringLiteral("trakt"));
        item.insert(QStringLiteral("progressPercent"), progress);
        item.insert(QStringLiteral("updatedAt"), pausedAtSeconds(entry));
        items.append(item);
    }
    return items;
}
} // namespace

TraktClient::TraktClient(QObject *parent)
    : QObject(parent)
{
    m_pollTimer.setSingleShot(true);
    connect(&m_pollTimer, &QTimer::timeout, this, &TraktClient::pollDeviceToken);
    load();
}

QString TraktClient::clientId() const { return m_clientId; }
QString TraktClient::clientSecret() const { return m_clientSecret; }
QString TraktClient::username() const { return m_username; }
QString TraktClient::statusMessage() const { return m_statusMessage; }
QString TraktClient::userCode() const { return m_userCode; }
QString TraktClient::verificationUrl() const { return m_verificationUrl; }
QVariantList TraktClient::playbackProgress() const { return m_playbackProgress; }
bool TraktClient::connected() const { return !m_accessToken.isEmpty() && !m_refreshToken.isEmpty(); }
bool TraktClient::authPending() const { return !m_deviceCode.isEmpty(); }
bool TraktClient::busy() const { return m_busy; }

void TraktClient::setClientId(const QString &clientId)
{
    const QString trimmed = clientId.trimmed();
    if (m_clientId == trimmed) {
        return;
    }
    m_clientId = trimmed;
    persistCredentials();
    emit changed();
}

void TraktClient::setClientSecret(const QString &clientSecret)
{
    const QString trimmed = clientSecret.trimmed();
    if (m_clientSecret == trimmed) {
        return;
    }
    m_clientSecret = trimmed;
    persistCredentials();
    emit changed();
}

void TraktClient::startDeviceAuthorization()
{
    if (m_clientId.isEmpty() || m_clientSecret.isEmpty()) {
        setStatus(QStringLiteral("Add your Trakt client ID and client secret first"));
        return;
    }

    cancelDeviceAuthorization();
    setBusy(true);
    setStatus(QStringLiteral("Requesting Trakt device code"));
    postJson(QStringLiteral("/oauth/device/code"),
             jsonBody({{QStringLiteral("client_id"), m_clientId}}),
             [this](QNetworkReply *reply) { handleDeviceCodeReply(reply); });
}

void TraktClient::cancelDeviceAuthorization()
{
    m_pollTimer.stop();
    finishPendingAuth();
}

void TraktClient::disconnectTrakt()
{
    cancelDeviceAuthorization();
    clearTokens();
    setStatus(QStringLiteral("Trakt disconnected"));
}

void TraktClient::load()
{
    QSettings settings;
    m_clientId = settings.value(QStringLiteral("trakt/clientId")).toString();
    m_clientSecret = settings.value(QStringLiteral("trakt/clientSecret")).toString();
    m_accessToken = settings.value(QStringLiteral("trakt/accessToken")).toString();
    m_refreshToken = settings.value(QStringLiteral("trakt/refreshToken")).toString();
    m_tokenCreatedAt = settings.value(QStringLiteral("trakt/tokenCreatedAt"), 0).toLongLong();
    m_tokenExpiresIn = settings.value(QStringLiteral("trakt/tokenExpiresIn"), 0).toInt();
    m_username = settings.value(QStringLiteral("trakt/username")).toString();
    if (connected()) {
        setStatus(m_username.isEmpty()
                      ? QStringLiteral("Trakt connected")
                      : QStringLiteral("Trakt connected as %1").arg(m_username));
        fetchPlaybackProgress();
    }
}

void TraktClient::persistCredentials()
{
    QSettings settings;
    settings.setValue(QStringLiteral("trakt/clientId"), m_clientId);
    settings.setValue(QStringLiteral("trakt/clientSecret"), m_clientSecret);
}

void TraktClient::persistTokens()
{
    QSettings settings;
    settings.setValue(QStringLiteral("trakt/accessToken"), m_accessToken);
    settings.setValue(QStringLiteral("trakt/refreshToken"), m_refreshToken);
    settings.setValue(QStringLiteral("trakt/tokenCreatedAt"), m_tokenCreatedAt);
    settings.setValue(QStringLiteral("trakt/tokenExpiresIn"), m_tokenExpiresIn);
    settings.setValue(QStringLiteral("trakt/username"), m_username);
}

void TraktClient::clearTokens()
{
    QSettings settings;
    settings.remove(QStringLiteral("trakt/accessToken"));
    settings.remove(QStringLiteral("trakt/refreshToken"));
    settings.remove(QStringLiteral("trakt/tokenCreatedAt"));
    settings.remove(QStringLiteral("trakt/tokenExpiresIn"));
    settings.remove(QStringLiteral("trakt/username"));
    m_accessToken.clear();
    m_refreshToken.clear();
    m_tokenCreatedAt = 0;
    m_tokenExpiresIn = 0;
    m_username.clear();
    m_playbackProgress.clear();
    m_pendingPlaybackMovies.clear();
    m_pendingPlaybackEpisodes.clear();
    m_playbackMoviesPending = false;
    m_playbackEpisodesPending = false;
    emit changed();
}

void TraktClient::postJson(const QString &path, const QByteArray &body, const std::function<void(QNetworkReply *)> &handler)
{
    QNetworkRequest request(QUrl(QString::fromLatin1(kBaseUrl) + path));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Miru/0.1"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");
    QNetworkReply *reply = m_network.post(request, body);
    connect(reply, &QNetworkReply::finished, this, [reply, handler]() {
        reply->deleteLater();
        handler(reply);
    });
}

void TraktClient::getAuthorized(const QString &path, const std::function<void(QNetworkReply *)> &handler)
{
    if (!connected()) {
        return;
    }
    if (tokenExpiresSoon()) {
        refreshAccessToken([this, path, handler](bool ok) {
            if (ok) {
                getAuthorized(path, handler);
            }
        });
        return;
    }

    QNetworkRequest request(QUrl(QString::fromLatin1(kBaseUrl) + path));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Miru/0.1"));
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("trakt-api-version", "2");
    request.setRawHeader("trakt-api-key", m_clientId.toUtf8());
    request.setRawHeader("Authorization", QByteArray("Bearer ") + m_accessToken.toUtf8());
    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, handler]() {
        reply->deleteLater();
        handler(reply);
    });
}

void TraktClient::postAuthorized(const QString &path, const QByteArray &body, const std::function<void(QNetworkReply *)> &handler)
{
    if (!connected()) {
        return;
    }
    if (tokenExpiresSoon()) {
        refreshAccessToken([this, path, body, handler](bool ok) {
            if (ok) {
                postAuthorized(path, body, handler);
            }
        });
        return;
    }

    QNetworkRequest request(QUrl(QString::fromLatin1(kBaseUrl) + path));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Miru/0.1"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("trakt-api-version", "2");
    request.setRawHeader("trakt-api-key", m_clientId.toUtf8());
    request.setRawHeader("Authorization", QByteArray("Bearer ") + m_accessToken.toUtf8());
    QNetworkReply *reply = m_network.post(request, body);
    connect(reply, &QNetworkReply::finished, this, [reply, handler]() {
        reply->deleteLater();
        handler(reply);
    });
}

bool TraktClient::tokenExpiresSoon() const
{
    if (m_tokenCreatedAt <= 0 || m_tokenExpiresIn <= 0) {
        return true;
    }
    return QDateTime::currentSecsSinceEpoch() >= m_tokenCreatedAt + m_tokenExpiresIn - 60;
}

void TraktClient::refreshAccessToken(const std::function<void(bool)> &handler)
{
    if (m_clientId.isEmpty() || m_clientSecret.isEmpty() || m_refreshToken.isEmpty()) {
        handler(false);
        return;
    }

    postJson(QStringLiteral("/oauth/token"),
             jsonBody({{QStringLiteral("refresh_token"), m_refreshToken},
                       {QStringLiteral("client_id"), m_clientId},
                       {QStringLiteral("client_secret"), m_clientSecret},
                       {QStringLiteral("grant_type"), QStringLiteral("refresh_token")}}),
             [this, handler](QNetworkReply *reply) {
        const QByteArray payload = reply->readAll();
        const int status = httpStatus(reply);
        if (reply->error() != QNetworkReply::NoError || status < 200 || status >= 300) {
            setStatus(apiErrorMessage(payload, QStringLiteral("Trakt token refresh failed")));
            emit errorOccurred(m_statusMessage);
            handler(false);
            return;
        }

        applyTokenResponse(payload);
        handler(true);
    });
}

void TraktClient::fetchPlaybackProgress()
{
    if (!connected()) {
        return;
    }
    if (tokenExpiresSoon()) {
        refreshAccessToken([this](bool ok) {
            if (ok) {
                fetchPlaybackProgress();
            }
        });
        return;
    }

    m_playbackMoviesPending = true;
    m_playbackEpisodesPending = true;
    m_pendingPlaybackMovies.clear();
    m_pendingPlaybackEpisodes.clear();
    getAuthorized(QStringLiteral("/sync/playback/movies"),
                  [this](QNetworkReply *reply) { handlePlaybackProgressReply(QStringLiteral("movie"), reply); });
    getAuthorized(QStringLiteral("/sync/playback/episodes"),
                  [this](QNetworkReply *reply) { handlePlaybackProgressReply(QStringLiteral("episode"), reply); });
}

void TraktClient::sendPlaybackProgress(const QVariantMap &media, double position, double duration)
{
    if (!connected() || duration <= 0.0 || position < 0.0) {
        return;
    }

    const double progress = std::clamp((position / duration) * 100.0, 0.0, 100.0);
    const QJsonObject body = scrobbleBody(media, progress);
    if (body.isEmpty()) {
        return;
    }

    postAuthorized(QStringLiteral("/scrobble/stop"), jsonBody(body), [this](QNetworkReply *reply) {
        const QByteArray payload = reply->readAll();
        const int status = httpStatus(reply);
        if ((reply->error() == QNetworkReply::NoError && status >= 200 && status < 300) || status == 409) {
            setStatus(QStringLiteral("Trakt progress updated"));
            fetchPlaybackProgress();
            return;
        }

        setStatus(apiErrorMessage(payload, status > 0
            ? QStringLiteral("Failed to update Trakt playback progress (HTTP %1)").arg(status)
            : QStringLiteral("Failed to update Trakt playback progress")));
        emit errorOccurred(m_statusMessage);
    });
}

QJsonObject TraktClient::scrobbleBody(const QVariantMap &media, double progressPercent) const
{
    const QString type = media.value(QStringLiteral("type")).toString();
    const QString id = media.value(QStringLiteral("id")).toString();
    const QString baseId = media.value(QStringLiteral("baseId")).toString();
    const QString imdb = type == QStringLiteral("series") ? baseId : id;
    if (!imdb.startsWith(QStringLiteral("tt"))) {
        return {};
    }

    QJsonObject body;
    body.insert(QStringLiteral("progress"), progressPercent);
    if (type == QStringLiteral("movie")) {
        QJsonObject ids;
        ids.insert(QStringLiteral("imdb"), imdb);
        QJsonObject movie;
        movie.insert(QStringLiteral("ids"), ids);
        const QString title = media.value(QStringLiteral("name")).toString();
        if (!title.isEmpty()) {
            movie.insert(QStringLiteral("title"), title);
        }
        body.insert(QStringLiteral("movie"), movie);
        return body;
    }

    if (type == QStringLiteral("series")) {
        const int season = media.value(QStringLiteral("season")).toInt();
        const int episodeNumber = media.value(QStringLiteral("episode")).toInt();
        if (season <= 0 || episodeNumber <= 0) {
            return {};
        }

        QJsonObject ids;
        ids.insert(QStringLiteral("imdb"), imdb);
        QJsonObject show;
        show.insert(QStringLiteral("ids"), ids);
        const QString title = media.value(QStringLiteral("name")).toString();
        if (!title.isEmpty()) {
            show.insert(QStringLiteral("title"), title);
        }

        QJsonObject episode;
        episode.insert(QStringLiteral("season"), season);
        episode.insert(QStringLiteral("number"), episodeNumber);
        const QString episodeTitle = media.value(QStringLiteral("episodeTitle")).toString();
        if (!episodeTitle.isEmpty()) {
            episode.insert(QStringLiteral("title"), episodeTitle);
        }

        body.insert(QStringLiteral("show"), show);
        body.insert(QStringLiteral("episode"), episode);
        return body;
    }

    return {};
}

void TraktClient::handleDeviceCodeReply(QNetworkReply *reply)
{
    setBusy(false);
    const QByteArray payload = reply->readAll();
    const int status = httpStatus(reply);
    if (reply->error() != QNetworkReply::NoError || status < 200 || status >= 300) {
        setStatus(apiErrorMessage(payload, QStringLiteral("Failed to request Trakt device code")));
        emit errorOccurred(m_statusMessage);
        return;
    }

    const QJsonObject object = QJsonDocument::fromJson(payload).object();
    m_deviceCode = object.value(QStringLiteral("device_code")).toString();
    m_userCode = object.value(QStringLiteral("user_code")).toString();
    m_verificationUrl = object.value(QStringLiteral("verification_url")).toString();
    const int expiresIn = object.value(QStringLiteral("expires_in")).toInt(600);
    m_pollIntervalSeconds = object.value(QStringLiteral("interval")).toInt(5);
    m_pollFailures = 0;
    m_deviceCodeExpiresAt = QDateTime::currentDateTimeUtc().addSecs(expiresIn);

    if (m_deviceCode.isEmpty() || m_userCode.isEmpty() || m_verificationUrl.isEmpty()) {
        finishPendingAuth();
        setStatus(QStringLiteral("Invalid Trakt device authorization response"));
        emit errorOccurred(m_statusMessage);
        return;
    }

    setStatus(QStringLiteral("Open %1 and enter code %2").arg(m_verificationUrl, m_userCode));
    scheduleDeviceTokenPoll();
    emit changed();
}

void TraktClient::pollDeviceToken()
{
    if (m_deviceCode.isEmpty()) {
        return;
    }
    if (QDateTime::currentDateTimeUtc() >= m_deviceCodeExpiresAt) {
        finishPendingAuth();
        setStatus(QStringLiteral("Trakt device code expired"));
        return;
    }

    setBusy(true);
    postJson(QStringLiteral("/oauth/device/token"),
             jsonBody({{QStringLiteral("code"), m_deviceCode},
                       {QStringLiteral("client_id"), m_clientId},
                       {QStringLiteral("client_secret"), m_clientSecret}}),
             [this](QNetworkReply *reply) { handleDeviceTokenReply(reply); });
}

void TraktClient::handleDeviceTokenReply(QNetworkReply *reply)
{
    setBusy(false);
    const QByteArray payload = reply->readAll();
    const QJsonObject object = QJsonDocument::fromJson(payload).object();
    const QString error = object.value(QStringLiteral("error")).toString();
    if (error == QStringLiteral("authorization_pending") || error == QStringLiteral("pending")) {
        m_pollFailures = 0;
        scheduleDeviceTokenPoll();
        return;
    }
    if (error == QStringLiteral("slow_down")) {
        m_pollFailures = 0;
        m_pollIntervalSeconds += 5;
        scheduleDeviceTokenPoll();
        return;
    }

    const int status = httpStatus(reply);
    if (reply->error() != QNetworkReply::NoError || status < 200 || status >= 300) {
        if (isTerminalDeviceAuthError(error)) {
            finishPendingAuth();
            setStatus(apiErrorMessage(payload, QStringLiteral("Trakt authorization failed")));
            emit errorOccurred(m_statusMessage);
            return;
        }

        ++m_pollFailures;
        setStatus(QStringLiteral("Waiting for Trakt approval (retrying token check)…"));
        scheduleDeviceTokenPoll();
        return;
    }

    applyTokenResponse(payload);
    finishPendingAuth();
    setStatus(QStringLiteral("Trakt connected"));
    fetchUserSettings();
    fetchPlaybackProgress();
}

void TraktClient::scheduleDeviceTokenPoll()
{
    if (m_deviceCode.isEmpty()) {
        return;
    }
    const int backoffSeconds = std::min(m_pollFailures * 5, 30);
    m_pollTimer.start((m_pollIntervalSeconds + backoffSeconds) * 1000);
}

bool TraktClient::isTerminalDeviceAuthError(const QString &error) const
{
    return error == QStringLiteral("access_denied")
        || error == QStringLiteral("expired_token")
        || error == QStringLiteral("invalid_client")
        || error == QStringLiteral("invalid_grant");
}

void TraktClient::fetchUserSettings()
{
    if (!connected()) {
        return;
    }

    getAuthorized(QStringLiteral("/users/settings"),
                  [this](QNetworkReply *reply) { handleUserSettingsReply(reply); });
}

void TraktClient::handleUserSettingsReply(QNetworkReply *reply)
{
    const QByteArray payload = reply->readAll();
    if (reply->error() == QNetworkReply::NoError) {
        const QJsonObject user = QJsonDocument::fromJson(payload).object().value(QStringLiteral("user")).toObject();
        const QString username = user.value(QStringLiteral("username")).toString().trimmed();
        if (!username.isEmpty()) {
            m_username = username;
            persistTokens();
            setStatus(QStringLiteral("Trakt connected as %1").arg(m_username));
            return;
        }
    }
    persistTokens();
    emit changed();
}

void TraktClient::handlePlaybackProgressReply(const QString &kind, QNetworkReply *reply)
{
    const QByteArray payload = reply->readAll();
    const int status = httpStatus(reply);
    if (reply->error() != QNetworkReply::NoError || status < 200 || status >= 300) {
        setStatus(apiErrorMessage(payload, QStringLiteral("Failed to load Trakt playback progress")));
        emit errorOccurred(m_statusMessage);
        if (kind == QStringLiteral("movie")) {
            m_playbackMoviesPending = false;
        } else {
            m_playbackEpisodesPending = false;
        }
        publishPlaybackProgressIfReady();
        return;
    }

    if (kind == QStringLiteral("movie")) {
        m_pendingPlaybackMovies = playbackItemsFromJson(payload, kind);
        m_playbackMoviesPending = false;
    } else {
        m_pendingPlaybackEpisodes = playbackItemsFromJson(payload, kind);
        m_playbackEpisodesPending = false;
    }
    publishPlaybackProgressIfReady();
}

void TraktClient::publishPlaybackProgressIfReady()
{
    if (m_playbackMoviesPending || m_playbackEpisodesPending) {
        return;
    }

    QVariantList items = m_pendingPlaybackMovies;
    items.append(m_pendingPlaybackEpisodes);
    std::sort(items.begin(), items.end(), [](const QVariant &left, const QVariant &right) {
        return left.toMap().value(QStringLiteral("updatedAt")).toLongLong()
            > right.toMap().value(QStringLiteral("updatedAt")).toLongLong();
    });
    m_playbackProgress = items;
    emit changed();
}

void TraktClient::applyTokenResponse(const QByteArray &payload)
{
    const QJsonObject object = QJsonDocument::fromJson(payload).object();
    m_accessToken = object.value(QStringLiteral("access_token")).toString();
    m_refreshToken = object.value(QStringLiteral("refresh_token")).toString();
    m_tokenCreatedAt = static_cast<qint64>(object.value(QStringLiteral("created_at")).toDouble());
    m_tokenExpiresIn = object.value(QStringLiteral("expires_in")).toInt();
    persistTokens();
    emit changed();
}

void TraktClient::setStatus(const QString &message)
{
    if (m_statusMessage == message) {
        return;
    }
    m_statusMessage = message;
    emit changed();
}

void TraktClient::setBusy(bool busy)
{
    if (m_busy == busy) {
        return;
    }
    m_busy = busy;
    emit changed();
}

void TraktClient::finishPendingAuth()
{
    m_pollTimer.stop();
    m_deviceCode.clear();
    m_userCode.clear();
    m_verificationUrl.clear();
    m_deviceCodeExpiresAt = {};
    m_pollFailures = 0;
    emit changed();
}
