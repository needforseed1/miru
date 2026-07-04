#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "../src/app/WatchHistory.h"

class WatchHistoryTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void recordsAndResumesMovies();
    void ignoresEarlyProgressAndRemovesCompletedItems();
    void appliesLandscapeMetadataToExistingEntries();
    void persistsSeriesProgress();
    void savesWatchHistoryOwnerOnly();
    void keepsWatchedEpisodesForNextUp();
    void watchedFlagSurvivesBarelyStartedReplay();
};

namespace {
QVariantMap movieMedia()
{
    return {
        {QStringLiteral("type"), QStringLiteral("movie")},
        {QStringLiteral("id"), QStringLiteral("tt0111161")},
        {QStringLiteral("name"), QStringLiteral("The Shawshank Redemption")},
    };
}

QVariantMap seriesMedia()
{
    return {
        {QStringLiteral("type"), QStringLiteral("series")},
        {QStringLiteral("id"), QStringLiteral("tt0944947:1:1")},
        {QStringLiteral("baseId"), QStringLiteral("tt0944947")},
        {QStringLiteral("season"), 1},
        {QStringLiteral("episode"), 1},
        {QStringLiteral("name"), QStringLiteral("Game of Thrones")},
    };
}

void removeAppData()
{
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!path.isEmpty()) {
        QDir(path).removeRecursively();
    }
}
}

void WatchHistoryTest::initTestCase()
{
    static QTemporaryDir dataHome;
    QVERIFY(dataHome.isValid());
    qputenv("XDG_DATA_HOME", dataHome.path().toUtf8());
    QCoreApplication::setOrganizationName(QStringLiteral("MiruTests"));
    QCoreApplication::setApplicationName(QStringLiteral("WatchHistoryTest"));
}

void WatchHistoryTest::init()
{
    removeAppData();
}

void WatchHistoryTest::cleanup()
{
    removeAppData();
}

void WatchHistoryTest::recordsAndResumesMovies()
{
    WatchHistory history;
    QSignalSpy changed(&history, &WatchHistory::changed);

    history.record(movieMedia(), 60.0, 600.0);

    QCOMPARE(changed.count(), 1);
    const QVariantList items = history.inProgress();
    QCOMPARE(items.size(), 1);

    const QVariantMap item = items.first().toMap();
    QCOMPARE(item.value(QStringLiteral("key")).toString(), QStringLiteral("movie:tt0111161"));
    QCOMPARE(item.value(QStringLiteral("position")).toDouble(), 60.0);
    QCOMPARE(history.positionFor(movieMedia()), 55.0);
    QVERIFY(!history.entry(QStringLiteral("movie:tt0111161")).isEmpty());

    history.remove(QStringLiteral("movie:tt0111161"));
    QVERIFY(history.inProgress().isEmpty());
    QCOMPARE(changed.count(), 2);
}

void WatchHistoryTest::ignoresEarlyProgressAndRemovesCompletedItems()
{
    WatchHistory history;

    history.record(movieMedia(), 2.0, 100.0);
    QVERIFY(history.inProgress().isEmpty());

    history.record(movieMedia(), 20.0, 100.0);
    QCOMPARE(history.inProgress().size(), 1);

    history.record(movieMedia(), 95.0, 100.0);
    QVERIFY(history.inProgress().isEmpty());
    QCOMPARE(history.positionFor(movieMedia()), 0.0);
}

void WatchHistoryTest::appliesLandscapeMetadataToExistingEntries()
{
    WatchHistory history;
    history.record(seriesMedia(), 120.0, 1200.0);

    const QVariantMap meta{
        {QStringLiteral("id"), QStringLiteral("tt0944947")},
        {QStringLiteral("name"), QStringLiteral("Game of Thrones")},
        {QStringLiteral("poster"), QStringLiteral("poster.webp")},
        {QStringLiteral("background"), QStringLiteral("backdrop.webp")},
        {QStringLiteral("videos"), QVariantList{
            QVariantMap{
                {QStringLiteral("season"), 1},
                {QStringLiteral("episode"), 1},
                {QStringLiteral("thumbnail"), QStringLiteral("still.webp")},
                {QStringLiteral("title"), QStringLiteral("Winter Is Coming")},
            },
        }},
    };

    history.applyMetadata(meta);

    const QVariantMap item = history.entry(QStringLiteral("series:tt0944947:1:1"));
    QCOMPARE(item.value(QStringLiteral("poster")).toString(), QStringLiteral("poster.webp"));
    QCOMPARE(item.value(QStringLiteral("background")).toString(), QStringLiteral("backdrop.webp"));
    QCOMPARE(item.value(QStringLiteral("thumbnail")).toString(), QStringLiteral("still.webp"));
    QCOMPARE(item.value(QStringLiteral("episodeTitle")).toString(), QStringLiteral("Winter Is Coming"));
}

void WatchHistoryTest::persistsSeriesProgress()
{
    {
        WatchHistory history;
        history.record(seriesMedia(), 120.0, 1200.0);
        QCOMPARE(history.inProgress().size(), 1);
        QTest::qWait(1600);
    }

    const QString store = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
        .filePath(QStringLiteral("watch_history.json"));
    QVERIFY2(QFile::exists(store), qPrintable(store));

    WatchHistory reloaded;
    const QVariantList items = reloaded.inProgress();
    QCOMPARE(items.size(), 1);

    const QVariantMap item = items.first().toMap();
    QCOMPARE(item.value(QStringLiteral("key")).toString(), QStringLiteral("series:tt0944947:1:1"));
    QCOMPARE(item.value(QStringLiteral("position")).toDouble(), 120.0);
    QCOMPARE(reloaded.positionFor(seriesMedia()), 115.0);
}

void WatchHistoryTest::savesWatchHistoryOwnerOnly()
{
    QString store;
    {
        WatchHistory history;
        history.record(seriesMedia(), 120.0, 1200.0);
        store = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
            .filePath(QStringLiteral("watch_history.json"));
    }

    QVERIFY2(QFile::exists(store), qPrintable(store));
    const QFileDevice::Permissions expected = QFileDevice::ReadOwner | QFileDevice::WriteOwner
        | QFileDevice::ReadUser | QFileDevice::WriteUser;
    QCOMPARE(static_cast<int>(QFile::permissions(store)), static_cast<int>(expected));
}

void WatchHistoryTest::keepsWatchedEpisodesForNextUp()
{
    WatchHistory history;

    QVariantMap episode1 = seriesMedia();
    history.record(episode1, 95.0, 100.0);

    QVariantMap episode2 = seriesMedia();
    episode2.insert(QStringLiteral("id"), QStringLiteral("tt0944947:1:2"));
    episode2.insert(QStringLiteral("episode"), 2);
    history.record(episode2, 95.0, 100.0);

    // Finished episodes leave Continue Watching but stay recorded as watched.
    QVERIFY(history.inProgress().isEmpty());

    const QVariantList lastWatched = history.lastWatchedEpisodes();
    QCOMPARE(lastWatched.size(), 1); // one card per show
    const QVariantMap newest = lastWatched.first().toMap();
    QCOMPARE(newest.value(QStringLiteral("baseId")).toString(), QStringLiteral("tt0944947"));
    QCOMPARE(newest.value(QStringLiteral("season")).toInt(), 1);
    QCOMPARE(newest.value(QStringLiteral("episode")).toInt(), 2);

    // An in-progress rewatch replaces the watched flag for that episode.
    history.record(episode2, 20.0, 100.0);
    QCOMPARE(history.inProgress().size(), 1);
    QCOMPARE(history.lastWatchedEpisodes().first().toMap()
                 .value(QStringLiteral("episode")).toInt(), 1);
}

void WatchHistoryTest::watchedFlagSurvivesBarelyStartedReplay()
{
    WatchHistory history;
    history.record(seriesMedia(), 95.0, 100.0);
    QCOMPARE(history.lastWatchedEpisodes().size(), 1);

    // Poking at a finished episode for a few seconds must not erase Next Up.
    history.record(seriesMedia(), 1.0, 100.0);
    QCOMPARE(history.lastWatchedEpisodes().size(), 1);
}

QTEST_MAIN(WatchHistoryTest)
#include "tst_watch_history.moc"
