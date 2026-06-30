#include <QtTest/QtTest>

#include <QJsonArray>
#include <QJsonObject>
#include <QVariantMap>

#include "../src/services/AIOStreamsClient.h"

class AIOStreamsClientTest : public QObject
{
    Q_OBJECT

private slots:
    void filtersUnplayableStreamsAndPreservesPlaybackHeaders();
    void marksSeriesSeasonPacks();
};

namespace {
QJsonObject stream(const QString &url, const QString &filename)
{
    return {
        {QStringLiteral("name"), QStringLiteral("AIOStreams")},
        {QStringLiteral("description"), QStringLiteral("Cached Usenet 2160p HDR")},
        {QStringLiteral("url"), url},
        {QStringLiteral("behaviorHints"), QJsonObject{
            {QStringLiteral("filename"), filename},
            {QStringLiteral("videoSize"), 5.0 * 1024.0 * 1024.0 * 1024.0},
            {QStringLiteral("proxyHeaders"), QJsonObject{
                {QStringLiteral("request"), QJsonObject{
                    {QStringLiteral("Authorization"), QStringLiteral(" Bearer token ")},
                    {QStringLiteral("Range"), QStringLiteral("bytes=0-")},
                    {QStringLiteral(" User-Agent "), QStringLiteral(" MiruTest ")},
                    {QStringLiteral("X-Blank"), QString()},
                }},
            }},
        }},
    };
}
}

void AIOStreamsClientTest::filtersUnplayableStreamsAndPreservesPlaybackHeaders()
{
    QJsonObject torrent = stream(QStringLiteral("https://example.invalid/torrent"), QStringLiteral("Torrent"));
    torrent.insert(QStringLiteral("infoHash"), QStringLiteral("abc123"));

    const QJsonArray streams = {
        stream(QStringLiteral("https://example.invalid/movie.mkv"), QStringLiteral("Movie.2160p.WEB-DL.HDR.mkv")),
        stream(QStringLiteral("magnet:?xt=urn:btih:abc123"), QStringLiteral("Magnet")),
        stream(QStringLiteral("ftp://example.invalid/movie.mkv"), QStringLiteral("FTP")),
        torrent,
    };

    AIOStreamsClient client;
    const QVariantList normalized = client.normalizeStreamsForTesting(streams);
    QCOMPARE(normalized.size(), 1);

    const QVariantMap item = normalized.first().toMap();
    QCOMPARE(item.value(QStringLiteral("title")).toString(), QStringLiteral("Movie.2160p.WEB-DL.HDR.mkv"));
    QCOMPARE(item.value(QStringLiteral("url")).toString(), QStringLiteral("https://example.invalid/movie.mkv"));
    QCOMPARE(item.value(QStringLiteral("quality")).toString(), QStringLiteral("2160p"));
    QCOMPARE(item.value(QStringLiteral("size")).toString(), QStringLiteral("5.0 GB"));
    QCOMPARE(item.value(QStringLiteral("hdr")).toBool(), true);
    QCOMPARE(item.value(QStringLiteral("usenet")).toBool(), true);

    const QVariantMap headers = item.value(QStringLiteral("headers")).toMap();
    QCOMPARE(headers.size(), 2);
    QCOMPARE(headers.value(QStringLiteral("Authorization")).toString(), QStringLiteral("Bearer token"));
    QCOMPARE(headers.value(QStringLiteral("User-Agent")).toString(), QStringLiteral("MiruTest"));
    QVERIFY(!headers.contains(QStringLiteral("Range")));
    QVERIFY(!headers.contains(QStringLiteral("X-Blank")));
}

void AIOStreamsClientTest::marksSeriesSeasonPacks()
{
    const QJsonArray streams = {
        stream(QStringLiteral("https://example.invalid/episode.mkv"), QStringLiteral("Show.S01E02.1080p.WEB-DL.mkv")),
        stream(QStringLiteral("https://example.invalid/pack.mkv"), QStringLiteral("Show.S01.1080p.WEB-DL.mkv")),
    };

    AIOStreamsClient client;
    const QVariantList normalized = client.normalizeStreamsForTesting(streams, QStringLiteral("series"));
    QCOMPARE(normalized.size(), 2);

    const QVariantMap episode = normalized.at(0).toMap();
    const QVariantMap pack = normalized.at(1).toMap();
    QCOMPARE(episode.value(QStringLiteral("seasonPack")).toBool(), false);
    QCOMPARE(pack.value(QStringLiteral("seasonPack")).toBool(), true);
}

QTEST_MAIN(AIOStreamsClientTest)
#include "tst_aiostreams_client.moc"
