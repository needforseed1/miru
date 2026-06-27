#include "TraktClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrl>

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

void TraktClient::handleDeviceCodeReply(QNetworkReply *reply)
{
    setBusy(false);
    const QByteArray payload = reply->readAll();
    if (reply->error() != QNetworkReply::NoError) {
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
    m_deviceCodeExpiresAt = QDateTime::currentDateTimeUtc().addSecs(expiresIn);

    if (m_deviceCode.isEmpty() || m_userCode.isEmpty() || m_verificationUrl.isEmpty()) {
        finishPendingAuth();
        setStatus(QStringLiteral("Invalid Trakt device authorization response"));
        emit errorOccurred(m_statusMessage);
        return;
    }

    setStatus(QStringLiteral("Open %1 and enter code %2").arg(m_verificationUrl, m_userCode));
    m_pollTimer.start(m_pollIntervalSeconds * 1000);
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
    if (reply->error() != QNetworkReply::NoError) {
        const QString error = object.value(QStringLiteral("error")).toString();
        if (error == QStringLiteral("authorization_pending")) {
            m_pollTimer.start(m_pollIntervalSeconds * 1000);
            return;
        }
        if (error == QStringLiteral("slow_down")) {
            m_pollIntervalSeconds += 5;
            m_pollTimer.start(m_pollIntervalSeconds * 1000);
            return;
        }
        finishPendingAuth();
        setStatus(apiErrorMessage(payload, QStringLiteral("Trakt authorization failed")));
        emit errorOccurred(m_statusMessage);
        return;
    }

    applyTokenResponse(payload);
    finishPendingAuth();
    setStatus(QStringLiteral("Trakt connected"));
    fetchUserSettings();
}

void TraktClient::fetchUserSettings()
{
    if (!connected()) {
        return;
    }

    QNetworkRequest request(QUrl(QString::fromLatin1(kBaseUrl) + QStringLiteral("/users/settings")));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Miru/0.1"));
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("trakt-api-version", "2");
    request.setRawHeader("trakt-api-key", m_clientId.toUtf8());
    request.setRawHeader("Authorization", QByteArray("Bearer ") + m_accessToken.toUtf8());
    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        handleUserSettingsReply(reply);
    });
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
    emit changed();
}
