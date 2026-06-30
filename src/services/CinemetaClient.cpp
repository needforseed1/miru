#include "CinemetaClient.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkDiskCache>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrl>

namespace {
// Cinemeta uses "0" (and sometimes empty) as a sentinel for "no rating",
// which is very common for episodes. Treat any non-positive value as absent.
QString sanitizeRating(const QString &raw)
{
    bool ok = false;
    const double value = raw.trimmed().toDouble(&ok);
    if (!ok || value <= 0.0) {
        return QString();
    }
    return raw.trimmed();
}
} // namespace

CinemetaClient::CinemetaClient(QObject *parent)
    : QObject(parent)
{
    // Cache catalog/manifest/meta JSON on disk. Repeat launches serve it
    // straight from disk while it is still fresh (per the addon's Cache-Control
    // max-age), and fall back to a fast revalidation otherwise.
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                        + QStringLiteral("/metadata");
    QDir().mkpath(dir);

    auto *diskCache = new QNetworkDiskCache(this);
    diskCache->setCacheDirectory(dir);
    diskCache->setMaximumCacheSize(32LL * 1024 * 1024);
    m_network.setCache(diskCache);
}

void CinemetaClient::fetchCatalog(const QString &type, const QString &catalogId)
{
    const QUrl url(QStringLiteral("%1/catalog/%2/%3.json").arg(activeBase(), type, catalogId));
    getJson(url, [this, type, catalogId](const QJsonObject &object) {
        QVariantList items;
        const QJsonArray metas = object.value(QStringLiteral("metas")).toArray();
        for (const QJsonValue &value : metas) {
            items.append(normalizeMeta(value.toObject()));
        }
        emit catalogReady(type, catalogId, items);
    });
}

void CinemetaClient::setBaseUrl(const QString &url)
{
    QString base = url.trimmed();
    while (base.endsWith(QLatin1Char('/'))) {
        base.chop(1);
    }
    if (base.endsWith(QStringLiteral("/manifest.json"))) {
        base.chop(QStringLiteral("/manifest.json").size());
    }
    while (base.endsWith(QLatin1Char('/'))) {
        base.chop(1);
    }
    m_addonUrl = base;
    m_searchCatalogId.clear();
    if (m_addonUrl.isEmpty()) {
        // Cinemeta's searchable movie/series catalogs are stable and named
        // "top". Seed them immediately so a first search at cold start doesn't
        // depend on the home manifest finishing first.
        m_searchCatalogId.insert(QStringLiteral("movie"), QStringLiteral("top"));
        m_searchCatalogId.insert(QStringLiteral("series"), QStringLiteral("top"));
        m_manifestReady = true;
    } else {
        m_manifestReady = false;
    }
    m_manifestLoading = false;
    m_pendingSearches.clear();
}

void CinemetaClient::discoverCatalogs()
{
    if (m_manifestLoading) {
        return;
    }

    m_manifestLoading = true;
    const QUrl url(QStringLiteral("%1/manifest.json").arg(activeBase()));
    getJson(url, [this](const QJsonObject &manifest) {
        QVariantList sections;
        m_searchCatalogId.clear();

        const QJsonArray catalogs = manifest.value(QStringLiteral("catalogs")).toArray();
        for (const QJsonValue &value : catalogs) {
            const QJsonObject catalog = value.toObject();
            const QString type = catalog.value(QStringLiteral("type")).toString();
            const QString id = catalog.value(QStringLiteral("id")).toString();
            if ((type != QStringLiteral("movie") && type != QStringLiteral("series")) || id.isEmpty()) {
                continue;
            }

            bool hasRequiredExtra = false;
            bool supportsSearch = false;
            const QJsonArray extra = catalog.value(QStringLiteral("extra")).toArray();
            for (const QJsonValue &ev : extra) {
                const QJsonObject extraObj = ev.toObject();
                const QString name = extraObj.value(QStringLiteral("name")).toString();
                const bool required = extraObj.value(QStringLiteral("isRequired")).toBool();
                if (name == QStringLiteral("search")) {
                    supportsSearch = true;
                }
                if (required && name != QStringLiteral("skip")) {
                    hasRequiredExtra = true;
                }
            }

            if (supportsSearch && !m_searchCatalogId.contains(type)) {
                m_searchCatalogId.insert(type, id);
            }

            // Plain home rails: catalogs that load without a required extra.
            if (!hasRequiredExtra) {
                const QString name = catalog.value(QStringLiteral("name")).toString();
                const QString suffix = type == QStringLiteral("movie")
                    ? QStringLiteral("Movies") : QStringLiteral("Shows");
                QVariantMap section;
                section.insert(QStringLiteral("id"), id);
                section.insert(QStringLiteral("type"), type);
                section.insert(QStringLiteral("title"),
                               name.isEmpty() ? suffix : QStringLiteral("%1 · %2").arg(name, suffix));
                sections.append(section);
            }
        }

        if (sections.isEmpty()) {
            fallbackCatalogs();
            return;
        }

        m_manifestLoading = false;
        m_manifestReady = true;
        emit catalogsDiscovered(sections);
        drainPendingSearches();
    }, [this]() {
        fallbackCatalogs();
    });
}

void CinemetaClient::fetchMeta(const QString &type, const QString &id)
{
    const QUrl url(QStringLiteral("%1/meta/%2/%3.json").arg(activeBase(), type, id));
    getJson(url, [this, type, id](const QJsonObject &object) {
        emit metaReady(type, id, normalizeMeta(object.value(QStringLiteral("meta")).toObject()));
    });
}

void CinemetaClient::search(const QString &type, const QString &query)
{
    if (!m_manifestReady) {
        m_pendingSearches.append({type, query});
        if (!m_manifestLoading) {
            discoverCatalogs();
        }
        return;
    }

    const QString encodedQuery = QString::fromUtf8(QUrl::toPercentEncoding(query));
    const QString catalogId = m_searchCatalogId.value(type, QStringLiteral("top"));
    const QUrl url(QStringLiteral("%1/catalog/%2/%3/search=%4.json")
                       .arg(activeBase(), type, catalogId, encodedQuery));
    getJson(url, [this, type, query](const QJsonObject &object) {
        QVariantList items;
        const QJsonArray metas = object.value(QStringLiteral("metas")).toArray();
        for (const QJsonValue &value : metas) {
            items.append(normalizeMeta(value.toObject()));
        }
        emit searchReady(type, query, items);
    }, [this, type, query]() {
        emit searchReady(type, query, QVariantList{});
    });
}

void CinemetaClient::getJson(const QUrl &url, const std::function<void(const QJsonObject &)> &handler,
                             const std::function<void()> &errorHandler)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("AIOStreamsLinux/0.1"));

    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, handler, errorHandler]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(QStringLiteral("Network error: %1").arg(reply->errorString()));
            if (errorHandler) {
                errorHandler();
            }
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            emit errorOccurred(QStringLiteral("Invalid JSON from Cinemeta"));
            if (errorHandler) {
                errorHandler();
            }
            return;
        }

        handler(document.object());
    });
}

void CinemetaClient::drainPendingSearches()
{
    const QVector<PendingSearch> searches = m_pendingSearches;
    m_pendingSearches.clear();
    for (const PendingSearch &searchRequest : searches) {
        search(searchRequest.type, searchRequest.query);
    }
}

void CinemetaClient::fallbackCatalogs()
{
    QVariantList sections;
    m_searchCatalogId.clear();
    for (const QString &type : {QStringLiteral("movie"), QStringLiteral("series")}) {
        QVariantMap section;
        section.insert(QStringLiteral("id"), QStringLiteral("top"));
        section.insert(QStringLiteral("type"), type);
        section.insert(QStringLiteral("title"),
                       type == QStringLiteral("movie") ? QStringLiteral("Popular Movies")
                                                       : QStringLiteral("Popular Shows"));
        sections.append(section);
        m_searchCatalogId.insert(type, QStringLiteral("top"));
    }

    m_manifestLoading = false;
    m_manifestReady = true;
    emit catalogsDiscovered(sections);
    drainPendingSearches();
}

#ifdef MIRU_TESTING
QVariantMap CinemetaClient::normalizeMetaForTesting(const QJsonObject &meta) const
{
    return normalizeMeta(meta);
}

QVariantList CinemetaClient::normalizeVideosForTesting(const QJsonArray &videos) const
{
    return normalizeVideos(videos);
}
#endif

QVariantMap CinemetaClient::normalizeMeta(const QJsonObject &meta) const
{
    QVariantMap item;
    item.insert(QStringLiteral("id"), meta.value(QStringLiteral("id")).toString());
    item.insert(QStringLiteral("type"), meta.value(QStringLiteral("type")).toString());
    item.insert(QStringLiteral("name"), meta.value(QStringLiteral("name")).toString());
    item.insert(QStringLiteral("poster"), meta.value(QStringLiteral("poster")).toString());
    item.insert(QStringLiteral("background"), meta.value(QStringLiteral("background")).toString());
    item.insert(QStringLiteral("logo"), meta.value(QStringLiteral("logo")).toString());
    item.insert(QStringLiteral("description"), meta.value(QStringLiteral("description")).toString());
    item.insert(QStringLiteral("releaseInfo"), meta.value(QStringLiteral("releaseInfo")).toString());
    item.insert(QStringLiteral("imdbRating"), sanitizeRating(meta.value(QStringLiteral("imdbRating")).toString()));
    item.insert(QStringLiteral("runtime"), meta.value(QStringLiteral("runtime")).toString());
    if (meta.contains(QStringLiteral("popularity"))) {
        item.insert(QStringLiteral("popularity"), meta.value(QStringLiteral("popularity")).toDouble());
    }
    if (meta.contains(QStringLiteral("score"))) {
        item.insert(QStringLiteral("score"), meta.value(QStringLiteral("score")).toDouble());
    }

    QStringList genres;
    const QJsonArray genreArray = meta.value(QStringLiteral("genres")).toArray();
    for (const QJsonValue &genre : genreArray) {
        genres.append(genre.toString());
    }
    item.insert(QStringLiteral("genres"), genres);
    item.insert(QStringLiteral("videos"), normalizeVideos(meta.value(QStringLiteral("videos")).toArray()));
    return item;
}

QVariantList CinemetaClient::normalizeVideos(const QJsonArray &videos) const
{
    QVariantList normalized;
    for (const QJsonValue &value : videos) {
        const QJsonObject video = value.toObject();

        // Cinemeta episode titles live in "name" (with "title" as a fallback).
        QString title = video.value(QStringLiteral("name")).toString();
        if (title.isEmpty()) {
            title = video.value(QStringLiteral("title")).toString();
        }

        // Per-episode IMDb rating. Cinemeta returns it as a string (and "0"
        // for the many episodes it has no rating for).
        const QString rating = sanitizeRating(video.value(QStringLiteral("rating")).toVariant().toString());

        QVariantMap item;
        item.insert(QStringLiteral("id"), video.value(QStringLiteral("id")).toString());
        item.insert(QStringLiteral("title"), title);
        item.insert(QStringLiteral("season"), video.value(QStringLiteral("season")).toInt());
        item.insert(QStringLiteral("episode"), video.value(QStringLiteral("episode")).toInt());
        item.insert(QStringLiteral("released"), video.value(QStringLiteral("released")).toString());
        item.insert(QStringLiteral("thumbnail"), video.value(QStringLiteral("thumbnail")).toString());
        item.insert(QStringLiteral("overview"), video.value(QStringLiteral("overview")).toString());
        item.insert(QStringLiteral("rating"), rating);
        normalized.append(item);
    }
    return normalized;
}
