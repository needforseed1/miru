#include "ImdbRatings.h"

#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFutureWatcher>
#include <QHash>
#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QtConcurrent>

#include <zlib.h>

#include <cstdlib>
#include <cstring>

namespace {
constexpr auto kEpisodeUrl = "https://datasets.imdbws.com/title.episode.tsv.gz";
constexpr auto kRatingsUrl = "https://datasets.imdbws.com/title.ratings.tsv.gz";
constexpr auto kReadConnection = "imdb_read";

// Split a tab-separated line in place. Returns the field count (capped).
int splitTabs(char *line, char **fields, int maxFields)
{
    int n = 0;
    fields[n++] = line;
    for (char *p = line; *p && n < maxFields; ++p) {
        if (*p == '\t') {
            *p = '\0';
            fields[n++] = p + 1;
        }
    }
    return n;
}

bool isNumericField(const char *s)
{
    if (!s || !*s || *s == '\\') { // "\N" null marker
        return false;
    }
    for (const char *p = s; *p; ++p) {
        if (*p < '0' || *p > '9') {
            return false;
        }
    }
    return true;
}
} // namespace

ImdbRatings::ImdbRatings(QObject *parent)
    : QObject(parent)
{
}

void ImdbRatings::init()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    m_dbPath = dir + QStringLiteral("/imdb_ratings.sqlite");
    if (QFile::exists(m_dbPath)) {
        openRead();
    }
}

void ImdbRatings::refreshIfStale(int maxAgeDays)
{
    QSettings settings;
    const QDateTime last = settings.value(QStringLiteral("imdb/lastRefresh")).toDateTime();
    const bool stale = !m_ready || !m_hasTitleTable || !last.isValid()
        || last.daysTo(QDateTime::currentDateTime()) >= maxAgeDays;
    if (stale) {
        refreshNow();
    }
}

void ImdbRatings::refreshNow()
{
    if (m_busy) {
        return;
    }
    m_busy = true;
    downloadDatasets();
}

void ImdbRatings::openRead()
{
    QSqlDatabase db = QSqlDatabase::contains(kReadConnection)
        ? QSqlDatabase::database(kReadConnection, false)
        : QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), kReadConnection);
    if (db.isOpen()) {
        db.close();
    }
    db.setDatabaseName(m_dbPath);
    if (!db.open()) {
        m_ready = false;
        return;
    }
    QSqlQuery q(db);
    m_ready = q.exec(QStringLiteral("SELECT 1 FROM episode_rating LIMIT 1")) && q.next();
    QSqlQuery t(db);
    m_hasTitleTable = t.exec(QStringLiteral("SELECT 1 FROM title_rating LIMIT 1")) && t.next();
}

void ImdbRatings::downloadDatasets()
{
    if (!m_network) {
        m_network = new QNetworkAccessManager(this);
    }

    const QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    m_episodeTmp = tmpDir + QStringLiteral("/imdb_title_episode.tsv.gz");
    m_ratingsTmp = tmpDir + QStringLiteral("/imdb_title_ratings.tsv.gz");

    m_pendingDownloads = 2;
    m_downloadFailed = false;
    emit statusChanged(QStringLiteral("Downloading IMDb ratings dataset…"));

    const auto fetch = [this](const QString &url, const QString &dest) {
        QNetworkRequest request{QUrl(url)};
        request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("AIOStreamsLinux/0.1"));
        QNetworkReply *reply = m_network->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, dest]() {
            reply->deleteLater();
            if (reply->error() == QNetworkReply::NoError) {
                QFile file(dest);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(reply->readAll());
                    file.close();
                } else {
                    m_downloadFailed = true;
                }
            } else {
                m_downloadFailed = true;
            }
            if (--m_pendingDownloads == 0) {
                if (m_downloadFailed) {
                    m_busy = false;
                    emit statusChanged(QStringLiteral("IMDb ratings download failed"));
                    emit refreshFinished(false);
                } else {
                    startBuild();
                }
            }
        });
    };

    fetch(QString::fromLatin1(kEpisodeUrl), m_episodeTmp);
    fetch(QString::fromLatin1(kRatingsUrl), m_ratingsTmp);
}

void ImdbRatings::startBuild()
{
    emit statusChanged(QStringLiteral("Building IMDb ratings cache…"));

    const QString buildPath = m_dbPath + QStringLiteral(".building");
    const QString episodePath = m_episodeTmp;
    const QString ratingsPath = m_ratingsTmp;

    if (!m_watcher) {
        m_watcher = new QFutureWatcher<bool>(this);
        connect(m_watcher, &QFutureWatcher<bool>::finished, this, [this, buildPath]() {
            const bool ok = m_watcher->result();
            QFile::remove(m_episodeTmp);
            QFile::remove(m_ratingsTmp);
            m_busy = false;
            if (ok) {
                // Swap the freshly built DB into place.
                if (QSqlDatabase::contains(kReadConnection)) {
                    QSqlDatabase::database(kReadConnection, false).close();
                }
                QFile::remove(m_dbPath);
                if (QFile::rename(buildPath, m_dbPath)) {
                    QSettings().setValue(QStringLiteral("imdb/lastRefresh"), QDateTime::currentDateTime());
                    openRead();
                    emit statusChanged(QStringLiteral("IMDb ratings updated"));
                    emit refreshFinished(true);
                    return;
                }
            }
            QFile::remove(buildPath);
            emit statusChanged(QStringLiteral("IMDb ratings build failed"));
            emit refreshFinished(false);
        });
    }

    m_watcher->setFuture(QtConcurrent::run(&ImdbRatings::buildDatabase, buildPath, episodePath, ratingsPath));
}

bool ImdbRatings::buildDatabase(const QString &dbPath,
                                const QString &episodePath,
                                const QString &ratingsPath)
{
    // 1) Load tconst -> rating from title.ratings into memory.
    QHash<QByteArray, float> ratings;
    ratings.reserve(1600000);
    {
        gzFile rt = gzopen(ratingsPath.toLocal8Bit().constData(), "rb");
        if (!rt) {
            return false;
        }
        char buf[512];
        gzgets(rt, buf, sizeof(buf)); // header
        char *fields[3];
        while (gzgets(rt, buf, sizeof(buf))) {
            buf[strcspn(buf, "\r\n")] = '\0';
            if (splitTabs(buf, fields, 3) < 2) {
                continue;
            }
            ratings.insert(QByteArray(fields[0]), strtof(fields[1], nullptr));
        }
        gzclose(rt);
    }
    if (ratings.isEmpty()) {
        return false;
    }

    // 2) Stream title.episode, join with ratings, insert into SQLite.
    const QString connName = QStringLiteral("imdb_build_%1")
        .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
        db.setDatabaseName(dbPath);
        if (db.open()) {
            QSqlQuery q(db);
            q.exec(QStringLiteral("PRAGMA journal_mode=OFF"));
            q.exec(QStringLiteral("PRAGMA synchronous=OFF"));
            q.exec(QStringLiteral("DROP TABLE IF EXISTS episode_rating"));
            q.exec(QStringLiteral("CREATE TABLE episode_rating ("
                                  "parent TEXT, season INTEGER, episode INTEGER, rating REAL, "
                                  "PRIMARY KEY(parent, season, episode)) WITHOUT ROWID"));

            // Title-level ratings (movies + series) for poster badges.
            q.exec(QStringLiteral("DROP TABLE IF EXISTS title_rating"));
            q.exec(QStringLiteral("CREATE TABLE title_rating ("
                                  "tconst TEXT PRIMARY KEY, rating REAL) WITHOUT ROWID"));
            db.transaction();
            QSqlQuery insTitle(db);
            insTitle.prepare(QStringLiteral("INSERT OR REPLACE INTO title_rating VALUES(?,?)"));
            for (auto it = ratings.constBegin(); it != ratings.constEnd(); ++it) {
                insTitle.bindValue(0, QString::fromLatin1(it.key()));
                insTitle.bindValue(1, *it);
                insTitle.exec();
            }
            db.commit();

            gzFile ep = gzopen(episodePath.toLocal8Bit().constData(), "rb");
            if (ep) {
                db.transaction();
                QSqlQuery ins(db);
                ins.prepare(QStringLiteral("INSERT OR REPLACE INTO episode_rating VALUES(?,?,?,?)"));

                char buf[512];
                gzgets(ep, buf, sizeof(buf)); // header
                char *fields[4];
                while (gzgets(ep, buf, sizeof(buf))) {
                    buf[strcspn(buf, "\r\n")] = '\0';
                    if (splitTabs(buf, fields, 4) < 4) {
                        continue;
                    }
                    if (!isNumericField(fields[2]) || !isNumericField(fields[3])) {
                        continue;
                    }
                    const auto it = ratings.constFind(QByteArray::fromRawData(fields[0], qstrlen(fields[0])));
                    if (it == ratings.constEnd()) {
                        continue;
                    }
                    ins.bindValue(0, QString::fromLatin1(fields[1]));
                    ins.bindValue(1, atoi(fields[2]));
                    ins.bindValue(2, atoi(fields[3]));
                    ins.bindValue(3, *it);
                    ins.exec();
                }
                db.commit();
                gzclose(ep);
                ok = true;
            }
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(connName);
    return ok;
}

QString ImdbRatings::ratingFor(const QString &seriesImdbId, int season, int episode) const
{
    if (!m_ready || seriesImdbId.isEmpty()) {
        return QString();
    }
    QSqlDatabase db = QSqlDatabase::database(kReadConnection, false);
    if (!db.isOpen()) {
        return QString();
    }
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT rating FROM episode_rating "
                             "WHERE parent=? AND season=? AND episode=?"));
    q.addBindValue(seriesImdbId);
    q.addBindValue(season);
    q.addBindValue(episode);
    if (q.exec() && q.next()) {
        return QString::number(q.value(0).toDouble(), 'f', 1);
    }
    return QString();
}

QString ImdbRatings::titleRating(const QString &imdbId) const
{
    if (!m_ready || imdbId.isEmpty()) {
        return QString();
    }
    QSqlDatabase db = QSqlDatabase::database(kReadConnection, false);
    if (!db.isOpen()) {
        return QString();
    }
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT rating FROM title_rating WHERE tconst=?"));
    q.addBindValue(imdbId);
    if (q.exec() && q.next()) {
        return QString::number(q.value(0).toDouble(), 'f', 1);
    }
    return QString();
}
