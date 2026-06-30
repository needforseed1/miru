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
    void persistsSeriesProgress();
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

QTEST_MAIN(WatchHistoryTest)
#include "tst_watch_history.moc"
