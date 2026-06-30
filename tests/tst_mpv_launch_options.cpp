#include <QtTest/QtTest>

#include "../src/player/MpvLaunchOptions.h"

class MpvLaunchOptionsTest : public QObject
{
    Q_OBJECT

private slots:
    void buildsNetworkPlaybackArguments();
    void buildsPercentStartArguments();
    void suppressesBuiltInOscWhenModernzResourceIsMissing();
    void appendsCustomArgsAfterDefaults();
};

void MpvLaunchOptionsTest::buildsNetworkPlaybackArguments()
{
    MpvLaunchOptions options;
    options.socketPath = QStringLiteral("/tmp/miru.sock");
    options.url = QStringLiteral(" https://example.invalid/movie.mkv ");
    options.title = QStringLiteral(" Movie Title ");
    options.headers = {
        {QStringLiteral("Authorization"), QStringLiteral(" Bearer token ")},
        {QStringLiteral("Range"), QStringLiteral("bytes=0-")},
        {QStringLiteral("X-Empty"), QString()},
    };
    options.subtitleLanguage = QStringLiteral("eng");
    options.enableGpuNext = true;
    options.enableHdrHint = true;
    options.startSeconds = 42.5;
    options.modernzScriptPath = QStringLiteral("/opt/miru/modernz.lua");
    options.modernzFontsPath = QStringLiteral("/opt/miru/fonts");

    const QStringList args = buildMpvArguments(options);

    QCOMPARE(args.first(), QStringLiteral("--input-ipc-server=/tmp/miru.sock"));
    QVERIFY(args.contains(QStringLiteral("--save-position-on-quit=no")));
    QVERIFY(args.contains(QStringLiteral("--fullscreen=yes")));
    QVERIFY(args.contains(QStringLiteral("--start=42.5")));
    QVERIFY(args.contains(QStringLiteral("--ytdl=no")));
    QVERIFY(args.contains(QStringLiteral("--cache=yes")));
    QVERIFY(args.contains(QStringLiteral("--network-timeout=60")));
    QVERIFY(args.contains(QStringLiteral("--audio-buffer=1.0")));
    QVERIFY(args.contains(QStringLiteral("--hwdec=auto-safe")));
    QVERIFY(args.contains(QStringLiteral("--vo=gpu-next")));
    QVERIFY(args.contains(QStringLiteral("--gpu-api=vulkan")));
    QVERIFY(args.contains(QStringLiteral("--target-colorspace-hint=yes")));
    QVERIFY(args.contains(QStringLiteral("--force-media-title=Movie Title")));
    QVERIFY(args.contains(QStringLiteral("--script=/opt/miru/modernz.lua")));
    QVERIFY(args.contains(QStringLiteral("--osd-fonts-dir=/opt/miru/fonts")));
    QVERIFY(args.contains(QStringLiteral("--slang=eng")));
    QVERIFY(args.contains(QStringLiteral("--http-header-fields-append=Authorization: Bearer token")));
    QVERIFY(!args.contains(QStringLiteral("--http-header-fields-append=Range: bytes=0-")));
    QVERIFY(!args.contains(QStringLiteral("--http-header-fields-append=X-Empty: ")));
    QCOMPARE(args.last(), QStringLiteral("https://example.invalid/movie.mkv"));
}

void MpvLaunchOptionsTest::buildsPercentStartArguments()
{
    MpvLaunchOptions options;
    options.socketPath = QStringLiteral("/tmp/miru.sock");
    options.url = QStringLiteral("https://example.invalid/show.mkv");
    options.startPercent = 12.5;
    options.enableHwdec = false;
    options.enableModernz = false;
    options.subtitleLanguage = QStringLiteral("off");
    options.startFullscreen = false;

    const QStringList args = buildMpvArguments(options);

    QVERIFY(args.contains(QStringLiteral("--start=12.5%")));
    QVERIFY(!args.contains(QStringLiteral("--fullscreen=yes")));
    QVERIFY(!args.contains(QStringLiteral("--hwdec=auto-safe")));
    QVERIFY(!args.contains(QStringLiteral("--slang=off")));
    QVERIFY(args.last() == QStringLiteral("https://example.invalid/show.mkv"));
}

void MpvLaunchOptionsTest::suppressesBuiltInOscWhenModernzResourceIsMissing()
{
    MpvLaunchOptions options;
    options.socketPath = QStringLiteral("/tmp/miru.sock");
    options.url = QStringLiteral("https://example.invalid/movie.mkv");
    options.enableModernz = true;

    const QStringList args = buildMpvArguments(options);

    QVERIFY(args.contains(QStringLiteral("--osc=no")));
    QVERIFY(args.contains(QStringLiteral("--osd-bar=no")));
    QVERIFY(args.contains(QStringLiteral("--script-opts=modernz-download_button=no,modernz-persistent_progress=no,modernz-persistent_buffer=no")));
    QVERIFY(!args.contains(QStringLiteral("--script=")));
    QVERIFY(!args.contains(QStringLiteral("--osd-fonts-dir=")));
}

void MpvLaunchOptionsTest::appendsCustomArgsAfterDefaults()
{
    MpvLaunchOptions options;
    options.socketPath = QStringLiteral("/tmp/miru.sock");
    options.url = QStringLiteral("https://example.invalid/movie.mkv");
    options.asahiLinux = true;
    options.extraArgs = {
        QStringLiteral("--hwdec=auto-safe"),
        QStringLiteral("--network-timeout=10"),
    };

    const QStringList args = buildMpvArguments(options);
    const int defaultHwdec = args.indexOf(QStringLiteral("--hwdec=no"));
    const int customHwdec = args.indexOf(QStringLiteral("--hwdec=auto-safe"));
    const int defaultTimeout = args.indexOf(QStringLiteral("--network-timeout=60"));
    const int customTimeout = args.indexOf(QStringLiteral("--network-timeout=10"));

    QVERIFY(defaultHwdec >= 0);
    QVERIFY(customHwdec > defaultHwdec);
    QVERIFY(defaultTimeout >= 0);
    QVERIFY(customTimeout > defaultTimeout);
    QCOMPARE(args.at(args.size() - 3), QStringLiteral("--hwdec=auto-safe"));
    QCOMPARE(args.at(args.size() - 2), QStringLiteral("--network-timeout=10"));
    QCOMPARE(args.last(), QStringLiteral("https://example.invalid/movie.mkv"));
}

QTEST_MAIN(MpvLaunchOptionsTest)
#include "tst_mpv_launch_options.moc"
