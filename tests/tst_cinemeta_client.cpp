#include <QtTest/QtTest>

#include <QJsonArray>
#include <QJsonObject>
#include <QVariantMap>

#include "../src/services/CinemetaClient.h"

class CinemetaClientTest : public QObject
{
    Q_OBJECT

private slots:
    void normalizesMetadataAndEpisodes();
    void sanitizesMissingRatings();
};

void CinemetaClientTest::normalizesMetadataAndEpisodes()
{
    const QJsonObject meta{
        {QStringLiteral("id"), QStringLiteral("tt0944947")},
        {QStringLiteral("type"), QStringLiteral("series")},
        {QStringLiteral("name"), QStringLiteral("Game of Thrones")},
        {QStringLiteral("poster"), QStringLiteral("https://example.invalid/poster.webp")},
        {QStringLiteral("background"), QStringLiteral("https://example.invalid/background.webp")},
        {QStringLiteral("logo"), QStringLiteral("https://example.invalid/logo.png")},
        {QStringLiteral("description"), QStringLiteral("Nine noble families fight for control.")},
        {QStringLiteral("releaseInfo"), QStringLiteral("2011-2019")},
        {QStringLiteral("imdbRating"), QStringLiteral(" 9.2 ")},
        {QStringLiteral("runtime"), QStringLiteral("55 min")},
        {QStringLiteral("popularity"), 1234.5},
        {QStringLiteral("score"), 98.0},
        {QStringLiteral("genres"), QJsonArray{
            QStringLiteral("Drama"),
            QStringLiteral("Fantasy"),
        }},
        {QStringLiteral("videos"), QJsonArray{
            QJsonObject{
                {QStringLiteral("id"), QStringLiteral("tt0944947:1:1")},
                {QStringLiteral("name"), QStringLiteral("Winter Is Coming")},
                {QStringLiteral("season"), 1},
                {QStringLiteral("episode"), 1},
                {QStringLiteral("released"), QStringLiteral("2011-04-17T00:00:00.000Z")},
                {QStringLiteral("thumbnail"), QStringLiteral("https://example.invalid/e1.webp")},
                {QStringLiteral("overview"), QStringLiteral("Series premiere.")},
                {QStringLiteral("rating"), QStringLiteral("8.9")},
            },
            QJsonObject{
                {QStringLiteral("id"), QStringLiteral("tt0944947:1:2")},
                {QStringLiteral("title"), QStringLiteral("The Kingsroad")},
                {QStringLiteral("season"), 1},
                {QStringLiteral("episode"), 2},
                {QStringLiteral("rating"), QStringLiteral("0")},
            },
        }},
    };

    CinemetaClient client;
    const QVariantMap item = client.normalizeMetaForTesting(meta);

    QCOMPARE(item.value(QStringLiteral("id")).toString(), QStringLiteral("tt0944947"));
    QCOMPARE(item.value(QStringLiteral("type")).toString(), QStringLiteral("series"));
    QCOMPARE(item.value(QStringLiteral("name")).toString(), QStringLiteral("Game of Thrones"));
    QCOMPARE(item.value(QStringLiteral("poster")).toString(), QStringLiteral("https://example.invalid/poster.webp"));
    QCOMPARE(item.value(QStringLiteral("background")).toString(), QStringLiteral("https://example.invalid/background.webp"));
    QCOMPARE(item.value(QStringLiteral("logo")).toString(), QStringLiteral("https://example.invalid/logo.png"));
    QCOMPARE(item.value(QStringLiteral("imdbRating")).toString(), QStringLiteral("9.2"));
    QCOMPARE(item.value(QStringLiteral("runtime")).toString(), QStringLiteral("55 min"));
    QCOMPARE(item.value(QStringLiteral("popularity")).toDouble(), 1234.5);
    QCOMPARE(item.value(QStringLiteral("score")).toDouble(), 98.0);
    QCOMPARE(item.value(QStringLiteral("genres")).toStringList(), QStringList({QStringLiteral("Drama"), QStringLiteral("Fantasy")}));

    const QVariantList videos = item.value(QStringLiteral("videos")).toList();
    QCOMPARE(videos.size(), 2);

    const QVariantMap first = videos.at(0).toMap();
    QCOMPARE(first.value(QStringLiteral("id")).toString(), QStringLiteral("tt0944947:1:1"));
    QCOMPARE(first.value(QStringLiteral("title")).toString(), QStringLiteral("Winter Is Coming"));
    QCOMPARE(first.value(QStringLiteral("season")).toInt(), 1);
    QCOMPARE(first.value(QStringLiteral("episode")).toInt(), 1);
    QCOMPARE(first.value(QStringLiteral("thumbnail")).toString(), QStringLiteral("https://example.invalid/e1.webp"));
    QCOMPARE(first.value(QStringLiteral("rating")).toString(), QStringLiteral("8.9"));

    const QVariantMap second = videos.at(1).toMap();
    QCOMPARE(second.value(QStringLiteral("title")).toString(), QStringLiteral("The Kingsroad"));
    QVERIFY(second.value(QStringLiteral("rating")).toString().isEmpty());
}

void CinemetaClientTest::sanitizesMissingRatings()
{
    CinemetaClient client;
    const QVariantMap unratedMeta = client.normalizeMetaForTesting(QJsonObject{
        {QStringLiteral("imdbRating"), QStringLiteral("0")},
    });
    QVERIFY(unratedMeta.value(QStringLiteral("imdbRating")).toString().isEmpty());

    const QVariantList videos = client.normalizeVideosForTesting(QJsonArray{
        QJsonObject{
            {QStringLiteral("name"), QStringLiteral("Unrated")},
            {QStringLiteral("rating"), QStringLiteral("not-a-number")},
        },
    });
    QCOMPARE(videos.size(), 1);
    QVERIFY(videos.first().toMap().value(QStringLiteral("rating")).toString().isEmpty());
}

QTEST_MAIN(CinemetaClientTest)
#include "tst_cinemeta_client.moc"
