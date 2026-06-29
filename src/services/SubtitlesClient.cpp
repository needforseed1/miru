#include "SubtitlesClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

SubtitlesClient::SubtitlesClient(QObject *parent)
    : QObject(parent)
{
}

void SubtitlesClient::fetchSubtitles(const QString &type, const QString &id)
{
    if (type.isEmpty() || id.isEmpty()) {
        return;
    }

    const QString encodedId = QString::fromUtf8(QUrl::toPercentEncoding(id, ":"));
    const QUrl url(QStringLiteral("%1/subtitles/%2/%3.json").arg(m_baseUrl, type, encodedId));

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("AIOStreamsLinux/0.1"));

    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, type, id]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(QStringLiteral("Subtitles error: %1").arg(reply->errorString()));
            return;
        }

        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
        if (!document.isObject()) {
            emit errorOccurred(QStringLiteral("Invalid JSON from OpenSubtitles"));
            return;
        }

        QVariantList subtitles;
        const QJsonArray array = document.object().value(QStringLiteral("subtitles")).toArray();
        for (const QJsonValue &value : array) {
            const QJsonObject sub = value.toObject();
            const QString subUrl = sub.value(QStringLiteral("url")).toString();
            if (subUrl.isEmpty()) {
                continue;
            }
            QVariantMap item;
            item.insert(QStringLiteral("url"), subUrl);
            item.insert(QStringLiteral("lang"), sub.value(QStringLiteral("lang")).toString());
            item.insert(QStringLiteral("id"), sub.value(QStringLiteral("id")).toString());
            subtitles.append(item);
        }

        emit subtitlesReady(type, id, subtitles);
    });
}
