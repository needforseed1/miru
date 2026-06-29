#pragma once

#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include <functional>

class QJsonArray;
class QJsonObject;

class CinemetaClient : public QObject
{
    Q_OBJECT

public:
    explicit CinemetaClient(QObject *parent = nullptr);

    void fetchCatalog(const QString &type, const QString &catalogId);
    void fetchMeta(const QString &type, const QString &id);
    void search(const QString &type, const QString &query);

    // Reads the active addon's manifest and emits the home catalog sections
    // (movie/series catalogs with no required extra). Falls back to a
    // Cinemeta-style "Popular" pair if the manifest can't be read.
    void discoverCatalogs();

    // Active metadata/catalog addon (e.g. a self-hosted AIOMetadata).
    // Empty = Cinemeta. Used for catalogs, search and meta.
    void setBaseUrl(const QString &url);

signals:
    void catalogsDiscovered(const QVariantList &sections);
    void catalogReady(const QString &type, const QString &catalogId, const QVariantList &items);
    void searchReady(const QString &type, const QString &query, const QVariantList &items);
    void metaReady(const QString &type, const QString &id, const QVariantMap &meta);
    void errorOccurred(const QString &message);

private:
    struct PendingSearch {
        QString type;
        QString query;
    };

    void getJson(const QUrl &url, const std::function<void(const QJsonObject &)> &handler,
                 const std::function<void()> &errorHandler = {});
    void drainPendingSearches();
    void fallbackCatalogs();
    QVariantMap normalizeMeta(const QJsonObject &meta) const;
    QVariantList normalizeVideos(const QJsonArray &videos) const;
    QString activeBase() const { return m_addonUrl.isEmpty() ? m_baseUrl : m_addonUrl; }

    QNetworkAccessManager m_network;
    QString m_baseUrl = QStringLiteral("https://v3-cinemeta.strem.io");
    QString m_addonUrl; // empty => use m_baseUrl
    QHash<QString, QString> m_searchCatalogId; // type -> catalog id for search
    QVector<PendingSearch> m_pendingSearches;
    bool m_manifestReady = false;
    bool m_manifestLoading = false;
};
