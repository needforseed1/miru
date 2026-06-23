#pragma once

#include <QObject>
#include <QString>

template <typename T> class QFutureWatcher;
class QNetworkAccessManager;
class QNetworkReply;

// Real IMDb episode ratings sourced from IMDb's free bulk dataset dumps
// (https://datasets.imdbws.com/), the same approach UpdateTool uses.
// Downloads title.episode + title.ratings, joins them into a local SQLite
// cache, and looks up per-episode ratings offline. Refreshed periodically.
class ImdbRatings : public QObject
{
    Q_OBJECT

public:
    explicit ImdbRatings(QObject *parent = nullptr);

    void init();
    void refreshIfStale(int maxAgeDays = 7);
    void refreshNow();

    bool ready() const { return m_ready; }

    // Returns a formatted rating ("8.9") for a series episode, or empty.
    QString ratingFor(const QString &seriesImdbId, int season, int episode) const;

    // Title-level rating ("8.3") for a movie or series IMDb id, or empty.
    QString titleRating(const QString &imdbId) const;

    // Joins title.episode + title.ratings dumps into a SQLite cache.
    // Exposed for testing; normally driven by the refresh pipeline.
    static bool buildDatabase(const QString &dbPath,
                              const QString &episodePath,
                              const QString &ratingsPath);

signals:
    void statusChanged(const QString &message);
    void refreshFinished(bool changed);

private:
    void openRead();
    void downloadDatasets();
    void startBuild();

    QString m_dbPath;
    QString m_episodeTmp;
    QString m_ratingsTmp;
    bool m_ready = false;
    bool m_hasTitleTable = false;
    bool m_busy = false;
    int m_pendingDownloads = 0;
    bool m_downloadFailed = false;

    QNetworkAccessManager *m_network = nullptr;
    QFutureWatcher<bool> *m_watcher = nullptr;
};
