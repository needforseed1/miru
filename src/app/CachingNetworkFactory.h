#pragma once

#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkRequest>
#include <QQmlNetworkAccessManagerFactory>
#include <QStandardPaths>

// QNetworkAccessManager backed by a persistent on-disk HTTP cache. The QML
// engine uses this for every networked Image (posters, backgrounds, logos),
// so artwork is written to disk once and reloaded from there on later launches
// instead of being re-downloaded each time the app starts.
class CachingNetworkAccessManager : public QNetworkAccessManager
{
public:
    explicit CachingNetworkAccessManager(QObject *parent = nullptr)
        : QNetworkAccessManager(parent)
    {
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                            + QStringLiteral("/artwork");
        QDir().mkpath(dir);

        auto *diskCache = new QNetworkDiskCache(this);
        diskCache->setCacheDirectory(dir);
        diskCache->setMaximumCacheSize(256LL * 1024 * 1024); // 256 MB of posters
        setCache(diskCache);
    }

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &request,
                                 QIODevice *outgoingData) override
    {
        QNetworkRequest req(request);
        if (op == GetOperation) {
            // Poster art is content-addressed and effectively immutable, so
            // prefer a cached copy and only touch the network on a miss.
            req.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                             QNetworkRequest::PreferCache);
            req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, true);
        }
        return QNetworkAccessManager::createRequest(op, req, outgoingData);
    }
};

class CachingNetworkFactory : public QQmlNetworkAccessManagerFactory
{
public:
    QNetworkAccessManager *create(QObject *parent) override
    {
        return new CachingNetworkAccessManager(parent);
    }
};
