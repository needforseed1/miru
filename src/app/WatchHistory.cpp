#include "WatchHistory.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTimer>

#include <algorithm>
#include <functional>

namespace {
constexpr double kMinProgress = 0.03;
constexpr double kMaxProgress = 0.92;
constexpr double kResumeBackoffSeconds = 5.0;
constexpr int kMaxContinueWatching = 20;
constexpr int kMaxWatchedEntries = 200;
}

WatchHistory::WatchHistory(QObject *parent)
    : QObject(parent)
    , m_saveTimer(new QTimer(this))
{
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(1500);
    connect(m_saveTimer, &QTimer::timeout, this, &WatchHistory::saveNow);
    load();
}

WatchHistory::~WatchHistory()
{
    saveNow();
}

QVariantList WatchHistory::inProgress() const
{
    QVariantList filtered;
    for (const QVariant &entry : m_entries) {
        const QVariantMap map = entry.toMap();
        if (isInProgress(map)) {
            filtered.append(map);
        }
    }

    std::sort(filtered.begin(), filtered.end(), [](const QVariant &a, const QVariant &b) {
        return a.toMap().value(QStringLiteral("updatedAt")).toLongLong()
               > b.toMap().value(QStringLiteral("updatedAt")).toLongLong();
    });

    while (filtered.size() > kMaxContinueWatching) {
        filtered.removeLast();
    }
    return filtered;
}

QVariantMap WatchHistory::entry(const QString &key) const
{
    for (const QVariant &entry : m_entries) {
        const QVariantMap map = entry.toMap();
        if (map.value(QStringLiteral("key")).toString() == key && isInProgress(map)) {
            return map;
        }
    }
    return {};
}

double WatchHistory::positionFor(const QVariantMap &media) const
{
    const QString key = keyForMedia(media);
    if (key.isEmpty()) {
        return 0.0;
    }

    for (const QVariant &entry : m_entries) {
        const QVariantMap map = entry.toMap();
        if (map.value(QStringLiteral("key")).toString() == key && isInProgress(map)) {
            return std::max(0.0, map.value(QStringLiteral("position")).toDouble() - kResumeBackoffSeconds);
        }
    }
    return 0.0;
}

void WatchHistory::record(const QVariantMap &media, double position, double duration)
{
    const QString key = keyForMedia(media);
    if (key.isEmpty() || duration <= 0.0 || position < 0.0) {
        return;
    }

    const double ratio = position / duration;
    // Finished items are kept (flagged watched) instead of dropped: they feed
    // the local Next Up rail and the auto-advance "what did I just finish"
    // lookup. Only barely-started sessions still disappear.
    const bool watched = ratio >= kMaxProgress;
    const bool resumable = ratio >= kMinProgress && !watched;
    bool modified = false;

    for (int i = 0; i < m_entries.size(); ++i) {
        QVariantMap existing = m_entries.at(i).toMap();
        if (existing.value(QStringLiteral("key")).toString() != key) {
            continue;
        }

        if (!watched && !resumable) {
            // Barely started. A watched entry stays watched (poking at a
            // finished episode must not erase Next Up); an in-progress one is
            // dropped as before.
            if (isWatched(existing)) {
                return;
            }
            m_entries.removeAt(i);
        } else {
            QVariantMap updated = media;
            updated.insert(QStringLiteral("key"), key);
            updated.insert(QStringLiteral("position"), position);
            updated.insert(QStringLiteral("duration"), duration);
            updated.insert(QStringLiteral("updatedAt"), QDateTime::currentSecsSinceEpoch());
            if (watched) {
                updated.insert(QStringLiteral("watched"), true);
            }
            m_entries[i] = updated;
        }
        modified = true;
        break;
    }

    if (!modified && (watched || resumable)) {
        QVariantMap entry = media;
        entry.insert(QStringLiteral("key"), key);
        entry.insert(QStringLiteral("position"), position);
        entry.insert(QStringLiteral("duration"), duration);
        entry.insert(QStringLiteral("updatedAt"), QDateTime::currentSecsSinceEpoch());
        if (watched) {
            entry.insert(QStringLiteral("watched"), true);
        }
        m_entries.append(entry);
        modified = true;
    }

    if (modified) {
        pruneWatched();
        scheduleSave();
        emit changed();
    }
}

QVariantList WatchHistory::lastWatchedEpisodes(int maxItems) const
{
    // Newest watched episode per series, by activity time. Following the
    // user's most recent action (rather than the highest episode number)
    // means a rewatch from S1 naturally surfaces S1's next episode.
    QHash<QString, QVariantMap> newestPerShow;
    for (const QVariant &entry : m_entries) {
        const QVariantMap map = entry.toMap();
        if (!isWatched(map) || map.value(QStringLiteral("type")).toString() != QStringLiteral("series")) {
            continue;
        }
        const QString baseId = map.value(QStringLiteral("baseId")).toString();
        if (baseId.isEmpty() || map.value(QStringLiteral("season")).toInt() <= 0
            || map.value(QStringLiteral("episode")).toInt() <= 0) {
            continue;
        }
        const auto it = newestPerShow.constFind(baseId);
        bool newer = it == newestPerShow.constEnd();
        if (!newer) {
            const qint64 candidateAt = map.value(QStringLiteral("updatedAt")).toLongLong();
            const qint64 currentAt = it->value(QStringLiteral("updatedAt")).toLongLong();
            if (candidateAt != currentAt) {
                newer = candidateAt > currentAt;
            } else {
                // Same-second finishes (batch imports, tests): the higher
                // episode wins so Next Up points past all of them.
                const int candidateSeason = map.value(QStringLiteral("season")).toInt();
                const int currentSeason = it->value(QStringLiteral("season")).toInt();
                newer = candidateSeason > currentSeason
                    || (candidateSeason == currentSeason
                        && map.value(QStringLiteral("episode")).toInt()
                               > it->value(QStringLiteral("episode")).toInt());
            }
        }
        if (newer) {
            newestPerShow.insert(baseId, map);
        }
    }

    QVariantList shows;
    for (const QVariantMap &map : newestPerShow) {
        shows.append(map);
    }
    std::sort(shows.begin(), shows.end(), [](const QVariant &a, const QVariant &b) {
        return a.toMap().value(QStringLiteral("updatedAt")).toLongLong()
               > b.toMap().value(QStringLiteral("updatedAt")).toLongLong();
    });
    while (shows.size() > maxItems) {
        shows.removeLast();
    }
    return shows;
}

void WatchHistory::pruneWatched()
{
    // Watched entries accumulate forever otherwise; drop the oldest beyond a
    // generous cap. In-progress entries are never pruned here.
    QList<QPair<qint64, int>> watchedByAge; // (updatedAt, index)
    for (int i = 0; i < m_entries.size(); ++i) {
        const QVariantMap map = m_entries.at(i).toMap();
        if (isWatched(map)) {
            watchedByAge.append({map.value(QStringLiteral("updatedAt")).toLongLong(), i});
        }
    }
    if (watchedByAge.size() <= kMaxWatchedEntries) {
        return;
    }

    std::sort(watchedByAge.begin(), watchedByAge.end());
    QList<int> drop;
    for (int i = 0; i < watchedByAge.size() - kMaxWatchedEntries; ++i) {
        drop.append(watchedByAge.at(i).second);
    }
    std::sort(drop.begin(), drop.end(), std::greater<int>());
    for (int index : drop) {
        m_entries.removeAt(index);
    }
}

void WatchHistory::applyMetadata(const QVariantMap &meta)
{
    const QString metaId = meta.value(QStringLiteral("id")).toString();
    if (metaId.isEmpty()) {
        return;
    }

    bool modified = false;
    for (QVariant &entry : m_entries) {
        QVariantMap item = entry.toMap();
        const QString type = item.value(QStringLiteral("type")).toString();
        const QString itemId = type == QStringLiteral("series")
            ? item.value(QStringLiteral("baseId")).toString()
            : item.value(QStringLiteral("id")).toString();
        if (itemId != metaId) {
            continue;
        }

        for (const QString &key : {QStringLiteral("poster"), QStringLiteral("background"),
                                   QStringLiteral("description"), QStringLiteral("releaseInfo")}) {
            const QString value = meta.value(key).toString();
            if (!value.isEmpty() && item.value(key).toString().isEmpty()) {
                item.insert(key, value);
                modified = true;
            }
        }

        const QString name = meta.value(QStringLiteral("name")).toString();
        if (!name.isEmpty() && item.value(QStringLiteral("name")).toString().isEmpty()) {
            item.insert(QStringLiteral("name"), name);
            modified = true;
        }

        if (type == QStringLiteral("series")) {
            const int season = item.value(QStringLiteral("season")).toInt();
            const int episode = item.value(QStringLiteral("episode")).toInt();
            for (const QVariant &videoEntry : meta.value(QStringLiteral("videos")).toList()) {
                const QVariantMap video = videoEntry.toMap();
                if (video.value(QStringLiteral("season")).toInt() != season
                    || video.value(QStringLiteral("episode")).toInt() != episode) {
                    continue;
                }
                const QString thumbnail = video.value(QStringLiteral("thumbnail")).toString();
                if (!thumbnail.isEmpty() && item.value(QStringLiteral("thumbnail")).toString().isEmpty()) {
                    item.insert(QStringLiteral("thumbnail"), thumbnail);
                    modified = true;
                }
                const QString title = video.value(QStringLiteral("title")).toString();
                if (!title.isEmpty() && item.value(QStringLiteral("episodeTitle")).toString().isEmpty()) {
                    item.insert(QStringLiteral("episodeTitle"), title);
                    modified = true;
                }
                break;
            }
        }

        entry = item;
    }

    if (modified) {
        scheduleSave();
        emit changed();
    }
}

void WatchHistory::remove(const QString &key)
{
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).toMap().value(QStringLiteral("key")).toString() == key) {
            m_entries.removeAt(i);
            scheduleSave();
            emit changed();
            return;
        }
    }
}

QString WatchHistory::keyForMedia(const QVariantMap &media)
{
    const QString type = media.value(QStringLiteral("type")).toString();
    if (type == QStringLiteral("movie")) {
        const QString id = media.value(QStringLiteral("id")).toString();
        return id.isEmpty() ? QString() : QStringLiteral("movie:%1").arg(id);
    }

    if (type == QStringLiteral("series")) {
        const QString baseId = media.value(QStringLiteral("baseId")).toString();
        const int season = media.value(QStringLiteral("season")).toInt();
        const int episode = media.value(QStringLiteral("episode")).toInt();
        if (!baseId.isEmpty() && season > 0 && episode > 0) {
            return QStringLiteral("series:%1:%2:%3").arg(baseId).arg(season).arg(episode);
        }
    }

    return {};
}

bool WatchHistory::isInProgress(const QVariantMap &entry)
{
    const double duration = entry.value(QStringLiteral("duration")).toDouble();
    const double position = entry.value(QStringLiteral("position")).toDouble();
    if (duration <= 0.0 || position < 0.0) {
        return false;
    }

    const double ratio = position / duration;
    return ratio >= kMinProgress && ratio < kMaxProgress;
}

bool WatchHistory::isWatched(const QVariantMap &entry)
{
    return entry.value(QStringLiteral("watched")).toBool();
}

QString WatchHistory::storePath() const
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty()) {
        base = QDir::home().filePath(QStringLiteral(".local/share/AIOStreams Linux"));
    }
    QDir().mkpath(base);
    return QDir(base).filePath(QStringLiteral("watch_history.json"));
}

void WatchHistory::load()
{
    QFile file(storePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray()) {
        return;
    }

    m_entries.clear();
    for (const QJsonValue &value : doc.array()) {
        if (!value.isObject()) {
            continue;
        }
        const QVariantMap entry = value.toObject().toVariantMap();
        if (!entry.value(QStringLiteral("key")).toString().isEmpty()) {
            m_entries.append(entry);
        }
    }
}

void WatchHistory::scheduleSave()
{
    m_saveTimer->start();
}

void WatchHistory::saveNow()
{
    if (m_saveTimer->isActive()) {
        m_saveTimer->stop();
    }

    QJsonArray array;
    for (const QVariant &entry : m_entries) {
        array.append(QJsonObject::fromVariantMap(entry.toMap()));
    }

    QFile file(storePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }
    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    file.close();
    // Entries embed the stream URL and any auth headers (debrid/WebDAV
    // Authorization) needed to resume playback — keep the file owner-only.
    file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}
