#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

#include "BadgeMatcher.h"

class QJsonObject;
class QJsonArray;
class QNetworkReply;

class AIOStreamsClient : public QObject
{
    Q_OBJECT

public:
    explicit AIOStreamsClient(QObject *parent = nullptr);

    void setBaseUrl(const QString &url);
    void fetchStreams(const QString &type, const QString &id);

#ifdef MIRU_TESTING
    QVariantList normalizeStreamsForTesting(const QJsonArray &streamArray, const QString &type = {}) const;
#endif

signals:
    void streamsReady(const QVariantList &streams);
    void errorOccurred(const QString &message);

private:
    void requestStreams(const QString &type, const QString &id, int attempt);
    void cancelActiveRequest();
    QString normalizedBaseUrl() const;
    QVariantList normalizeStreams(const QJsonArray &streamArray, const QString &type) const;
    QVariantMap normalizeStream(const QJsonObject &stream) const;
    bool isPlayableStream(const QJsonObject &stream) const;
    QString detectQuality(const QString &text) const;
    QString formatSize(double bytes) const;

    QNetworkAccessManager m_network;
    QNetworkReply *m_activeReply = nullptr; // in-flight stream request, if any
    QString m_baseUrl;
    BadgeMatcher m_badges;
};
