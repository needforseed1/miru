#include "AIOStreamsClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>

#include <algorithm>

#include <chrono>

AIOStreamsClient::AIOStreamsClient(QObject *parent)
    : QObject(parent)
{
    m_badges.load(QStringLiteral(":/resources/Sterzeck_badge.json"));
    // Give up on a stalled stream request so the UI doesn't spin forever.
    m_network.setTransferTimeout(std::chrono::seconds{15});
}

void AIOStreamsClient::setBaseUrl(const QString &url)
{
    m_baseUrl = url.trimmed();
}

void AIOStreamsClient::fetchStreams(const QString &type, const QString &id)
{
    const QString base = normalizedBaseUrl();
    if (base.isEmpty()) {
        emit errorOccurred(QStringLiteral("AIOStreams addon URL is not configured"));
        return;
    }

    const QString encodedId = QString::fromUtf8(QUrl::toPercentEncoding(id, ":"));
    const QUrl url(QStringLiteral("%1/stream/%2/%3.json").arg(base, type, encodedId));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("AIOStreamsLinux/0.1"));

    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, type]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(QStringLiteral("AIOStreams error: %1").arg(reply->errorString()));
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            emit errorOccurred(QStringLiteral("Invalid JSON from AIOStreams"));
            return;
        }

        QVariantList streams;
        const QJsonArray streamArray = document.object().value(QStringLiteral("streams")).toArray();
        for (const QJsonValue &value : streamArray) {
            const QJsonObject stream = value.toObject();
            if (isPlayableStream(stream)) {
                streams.append(normalizeStream(stream));
            }
        }

        // For an episode request, flag releases that have no explicit episode
        // marker (S01 packs, "Complete", etc.) and rank them after the actual
        // single-episode releases so a pack isn't the top hit.
        if (type == QStringLiteral("series")) {
            static const QRegularExpression episodeMarker(
                QStringLiteral("(?i)(s\\d{1,2}[ ._-]*e\\d{1,3}|(?<![a-z0-9])\\d{1,2}x\\d{1,3}(?![a-z0-9]))"));
            for (QVariant &entry : streams) {
                QVariantMap item = entry.toMap();
                const QString name = item.value(QStringLiteral("filename")).toString().isEmpty()
                    ? item.value(QStringLiteral("title")).toString()
                    : item.value(QStringLiteral("filename")).toString();
                item.insert(QStringLiteral("seasonPack"), !episodeMarker.match(name).hasMatch());
                entry = item;
            }
            std::stable_sort(streams.begin(), streams.end(), [](const QVariant &a, const QVariant &b) {
                return a.toMap().value(QStringLiteral("seasonPack")).toBool()
                     < b.toMap().value(QStringLiteral("seasonPack")).toBool();
            });
        }

        emit streamsReady(streams);
    });
}

QString AIOStreamsClient::normalizedBaseUrl() const
{
    QString base = m_baseUrl.trimmed();
    if (base.endsWith(QStringLiteral("/manifest.json"))) {
        base.chop(QStringLiteral("/manifest.json").size());
    }
    while (base.endsWith(QLatin1Char('/'))) {
        base.chop(1);
    }
    return base;
}

QVariantMap AIOStreamsClient::normalizeStream(const QJsonObject &stream) const
{
    const QString name = stream.value(QStringLiteral("name")).toString();
    const QString description = stream.value(QStringLiteral("description"))
                                    .toString(stream.value(QStringLiteral("title")).toString());

    const QJsonObject behaviorHints = stream.value(QStringLiteral("behaviorHints")).toObject();
    const QString filename = behaviorHints.value(QStringLiteral("filename")).toString();
    const double videoSize = behaviorHints.value(QStringLiteral("videoSize")).toDouble();

    // Carry through any request headers the addon needs for playback
    // (e.g. WebDAV/debrid Authorization), so mpv can fetch the stream.
    QVariantMap headers;
    const QJsonObject requestHeaders =
        behaviorHints.value(QStringLiteral("proxyHeaders")).toObject()
                     .value(QStringLiteral("request")).toObject();
    for (auto it = requestHeaders.constBegin(); it != requestHeaders.constEnd(); ++it) {
        headers.insert(it.key(), it.value().toString());
    }

    const QString combined = QStringLiteral("%1 %2 %3").arg(name, description, filename);

    // The readable release name shown as the row title, with the addon's
    // formatted name/description preserved verbatim underneath.
    const QString title = !filename.isEmpty() ? filename
                          : (!name.trimmed().isEmpty() ? name.trimmed() : QStringLiteral("Unnamed release"));

    QVariantMap item;
    item.insert(QStringLiteral("title"), title);
    item.insert(QStringLiteral("name"), name);
    item.insert(QStringLiteral("description"), description);
    item.insert(QStringLiteral("filename"), filename);
    item.insert(QStringLiteral("url"), stream.value(QStringLiteral("url")).toString());
    item.insert(QStringLiteral("size"), formatSize(videoSize));
    item.insert(QStringLiteral("quality"), detectQuality(combined));
    item.insert(QStringLiteral("hdr"),
                combined.contains(QStringLiteral("HDR"), Qt::CaseInsensitive)
                    || combined.contains(QStringLiteral("Dolby Vision"), Qt::CaseInsensitive)
                    || combined.contains(QStringLiteral(" DV"), Qt::CaseInsensitive));
    item.insert(QStringLiteral("usenet"),
                combined.contains(QStringLiteral("usenet"), Qt::CaseInsensitive)
                    || combined.contains(QStringLiteral("nzb"), Qt::CaseInsensitive));
    item.insert(QStringLiteral("headers"), headers);

    // Colorful concise badges matched from the release name.
    const QString badgeText = !filename.isEmpty() ? filename : combined;
    item.insert(QStringLiteral("badges"), m_badges.match(badgeText));
    return item;
}

QString AIOStreamsClient::formatSize(double bytes) const
{
    if (bytes <= 0) {
        return QString();
    }

    static const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = bytes;
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        ++unit;
    }
    return QStringLiteral("%1 %2").arg(size, 0, 'f', size < 10.0 ? 1 : 0).arg(QLatin1String(units[unit]));
}

bool AIOStreamsClient::isPlayableStream(const QJsonObject &stream) const
{
    if (stream.contains(QStringLiteral("infoHash"))) {
        return false;
    }

    const QString url = stream.value(QStringLiteral("url")).toString().trimmed();
    if (url.startsWith(QStringLiteral("magnet:"), Qt::CaseInsensitive)) {
        return false;
    }

    return url.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)
        || url.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive);
}

QString AIOStreamsClient::detectQuality(const QString &text) const
{
    if (text.contains(QStringLiteral("2160"), Qt::CaseInsensitive) || text.contains(QStringLiteral("4K"), Qt::CaseInsensitive)) {
        return QStringLiteral("2160p");
    }
    if (text.contains(QStringLiteral("1080"), Qt::CaseInsensitive)) {
        return QStringLiteral("1080p");
    }
    if (text.contains(QStringLiteral("720"), Qt::CaseInsensitive)) {
        return QStringLiteral("720p");
    }
    return QStringLiteral("Unknown");
}
