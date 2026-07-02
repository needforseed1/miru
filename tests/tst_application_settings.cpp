#include <QtTest/QtTest>

#include <QDir>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "../src/app/ApplicationController.h"

class ApplicationSettingsTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void loadsDefaults();
    void reportsSourceHealth();
    void persistsUserSettings();
    void ignoresInvalidScale();
    void ignoresInvalidThemeColors();
};

namespace {
QTemporaryDir &configHome()
{
    static QTemporaryDir dir;
    return dir;
}

QTemporaryDir &dataHome()
{
    static QTemporaryDir dir;
    return dir;
}

void clearSettings()
{
    QSettings settings;
    settings.clear();
    settings.sync();
}

void removeAppData()
{
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!path.isEmpty()) {
        QDir(path).removeRecursively();
    }
}

QVariantMap healthByName(const QVariantList &health, const QString &name)
{
    for (const QVariant &entry : health) {
        const QVariantMap item = entry.toMap();
        if (item.value(QStringLiteral("name")).toString() == name) {
            return item;
        }
    }
    return {};
}
}

void ApplicationSettingsTest::initTestCase()
{
    QVERIFY(configHome().isValid());
    QVERIFY(dataHome().isValid());
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, configHome().path());
    qputenv("XDG_CONFIG_HOME", configHome().path().toUtf8());
    qputenv("XDG_DATA_HOME", dataHome().path().toUtf8());
    QCoreApplication::setOrganizationName(QStringLiteral("MiruTests"));
    QCoreApplication::setApplicationName(QStringLiteral("ApplicationSettingsTest"));
}

void ApplicationSettingsTest::init()
{
    clearSettings();
    removeAppData();
}

void ApplicationSettingsTest::cleanup()
{
    clearSettings();
    removeAppData();
}

void ApplicationSettingsTest::loadsDefaults()
{
    ApplicationController controller;

    QCOMPARE(controller.aioStreamsUrl(), QString());
    QCOMPARE(controller.metadataUrl(), QString());
    QCOMPARE(controller.subtitleLanguage(), QStringLiteral("eng"));
    QCOMPARE(controller.uiScale(), 1.0);
    QCOMPARE(controller.showPosterRatings(), true);
    QCOMPARE(controller.uiMainColor(), QStringLiteral("#8460ff"));
    QCOMPARE(controller.uiProgressStartColor(), QStringLiteral("#8460ff"));
    QCOMPARE(controller.uiProgressEndColor(), QStringLiteral("#ff5e9a"));
    QCOMPARE(controller.mpvHardwareDecoding(), true);
    QCOMPARE(controller.mpvGpuNext(), false);
    QCOMPARE(controller.mpvHdrHint(), false);
    QCOMPARE(controller.mpvModernz(), true);
    QCOMPARE(controller.mpvFullscreen(), true);
    QCOMPARE(controller.mpvExtraArgs(), QString());
}

void ApplicationSettingsTest::reportsSourceHealth()
{
    ApplicationController controller;
    const QVariantList initial = controller.sourceHealth();
    QCOMPARE(initial.size(), 6);

    QCOMPARE(healthByName(initial, QStringLiteral("AIOStreams")).value(QStringLiteral("state")).toString(),
             QStringLiteral("warning"));
    QCOMPARE(healthByName(initial, QStringLiteral("Metadata")).value(QStringLiteral("status")).toString(),
             QStringLiteral("Cinemeta"));
    QCOMPARE(healthByName(initial, QStringLiteral("OpenSubtitles")).value(QStringLiteral("status")).toString(),
             QStringLiteral("ENG"));
    QCOMPARE(healthByName(initial, QStringLiteral("IMDb ratings")).value(QStringLiteral("state")).toString(),
             QStringLiteral("warning"));
    QCOMPARE(healthByName(initial, QStringLiteral("Trakt")).value(QStringLiteral("status")).toString(),
             QStringLiteral("Optional"));
    QVERIFY(!healthByName(initial, QStringLiteral("mpv")).isEmpty());

    QSignalSpy healthChanged(&controller, &ApplicationController::sourceHealthChanged);
    controller.setAioStreamsUrl(QStringLiteral("https://aio.example/manifest.json"));
    controller.setMetadataUrl(QStringLiteral("https://meta.example/manifest.json"));
    controller.setSubtitleLanguage(QStringLiteral("off"));

    QVERIFY(healthChanged.count() >= 3);
    const QVariantList updated = controller.sourceHealth();
    QCOMPARE(healthByName(updated, QStringLiteral("AIOStreams")).value(QStringLiteral("status")).toString(),
             QStringLiteral("Configured"));
    QCOMPARE(healthByName(updated, QStringLiteral("Metadata")).value(QStringLiteral("status")).toString(),
             QStringLiteral("Custom addon"));
    QCOMPARE(healthByName(updated, QStringLiteral("OpenSubtitles")).value(QStringLiteral("state")).toString(),
             QStringLiteral("neutral"));
}

void ApplicationSettingsTest::persistsUserSettings()
{
    ApplicationController controller;
    QSignalSpy aioChanged(&controller, &ApplicationController::aioStreamsUrlChanged);
    QSignalSpy metadataChanged(&controller, &ApplicationController::metadataUrlChanged);
    QSignalSpy subtitleChanged(&controller, &ApplicationController::subtitleLanguageChanged);
    QSignalSpy themeColorsChanged(&controller, &ApplicationController::themeColorsChanged);
    QSignalSpy extraArgsChanged(&controller, &ApplicationController::mpvExtraArgsChanged);

    controller.setAioStreamsUrl(QStringLiteral("  https://aio.example/manifest.json  "));
    controller.setMetadataUrl(QStringLiteral("  https://meta.example/manifest.json  "));
    controller.setSubtitleLanguage(QStringLiteral("nor"));
    controller.setUiScale(1.25);
    controller.setShowPosterRatings(false);
    controller.setThemeColors(QStringLiteral("2F9CFF"), QStringLiteral("#18d6a3"), QStringLiteral("#ff4f7b"));
    controller.setMpvHardwareDecoding(false);
    controller.setMpvGpuNext(true);
    controller.setMpvHdrHint(true);
    controller.setMpvModernz(false);
    controller.setMpvFullscreen(false);
    controller.setMpvExtraArgs(QStringLiteral("  --profile=high-quality --deband=yes  "));

    QCOMPARE(aioChanged.count(), 1);
    QCOMPARE(metadataChanged.count(), 1);
    QCOMPARE(subtitleChanged.count(), 1);
    QCOMPARE(themeColorsChanged.count(), 1);
    QCOMPARE(extraArgsChanged.count(), 1);

    QSettings settings;
    settings.sync();
    QCOMPARE(settings.value(QStringLiteral("addons/aioStreamsUrl")).toString(), QStringLiteral("https://aio.example/manifest.json"));
    QCOMPARE(settings.value(QStringLiteral("addons/metadataUrl")).toString(), QStringLiteral("https://meta.example/manifest.json"));
    QCOMPARE(settings.value(QStringLiteral("subtitles/language")).toString(), QStringLiteral("nor"));
    QCOMPARE(settings.value(QStringLiteral("ui/scaleFactor")).toDouble(), 1.25);
    QCOMPARE(settings.value(QStringLiteral("ui/showPosterRatings")).toBool(), false);
    QCOMPARE(settings.value(QStringLiteral("ui/mainColor")).toString(), QStringLiteral("#2f9cff"));
    QCOMPARE(settings.value(QStringLiteral("ui/progressStartColor")).toString(), QStringLiteral("#18d6a3"));
    QCOMPARE(settings.value(QStringLiteral("ui/progressEndColor")).toString(), QStringLiteral("#ff4f7b"));
    QCOMPARE(settings.value(QStringLiteral("mpv/hardwareDecoding")).toBool(), false);
    QCOMPARE(settings.value(QStringLiteral("mpv/gpuNext")).toBool(), true);
    QCOMPARE(settings.value(QStringLiteral("mpv/hdrHint")).toBool(), true);
    QCOMPARE(settings.value(QStringLiteral("mpv/modernz")).toBool(), false);
    QCOMPARE(settings.value(QStringLiteral("mpv/fullscreen")).toBool(), false);
    QCOMPARE(settings.value(QStringLiteral("mpv/extraArgs")).toString(), QStringLiteral("--profile=high-quality --deband=yes"));

    ApplicationController reloaded;
    QCOMPARE(reloaded.aioStreamsUrl(), QStringLiteral("https://aio.example/manifest.json"));
    QCOMPARE(reloaded.metadataUrl(), QStringLiteral("https://meta.example/manifest.json"));
    QCOMPARE(reloaded.subtitleLanguage(), QStringLiteral("nor"));
    QCOMPARE(reloaded.uiScale(), 1.25);
    QCOMPARE(reloaded.showPosterRatings(), false);
    QCOMPARE(reloaded.uiMainColor(), QStringLiteral("#2f9cff"));
    QCOMPARE(reloaded.uiProgressStartColor(), QStringLiteral("#18d6a3"));
    QCOMPARE(reloaded.uiProgressEndColor(), QStringLiteral("#ff4f7b"));
    QCOMPARE(reloaded.mpvHardwareDecoding(), false);
    QCOMPARE(reloaded.mpvGpuNext(), true);
    QCOMPARE(reloaded.mpvHdrHint(), true);
    QCOMPARE(reloaded.mpvModernz(), false);
    QCOMPARE(reloaded.mpvFullscreen(), false);
    QCOMPARE(reloaded.mpvExtraArgs(), QStringLiteral("--profile=high-quality --deband=yes"));
}

void ApplicationSettingsTest::ignoresInvalidScale()
{
    ApplicationController controller;

    QSignalSpy scaleChanged(&controller, &ApplicationController::uiScaleChanged);
    controller.setUiScale(0.0);
    controller.setUiScale(-2.0);
    QCOMPARE(controller.uiScale(), 1.0);
    QCOMPARE(scaleChanged.count(), 0);
}

void ApplicationSettingsTest::ignoresInvalidThemeColors()
{
    ApplicationController controller;

    QSignalSpy themeColorsChanged(&controller, &ApplicationController::themeColorsChanged);
    controller.setUiMainColor(QStringLiteral("#12345"));
    controller.setUiProgressStartColor(QStringLiteral("not-a-color"));
    controller.setUiProgressEndColor(QStringLiteral("#1234567"));
    controller.setThemeColors(QStringLiteral("#123456"), QStringLiteral("invalid"), QStringLiteral("#abcdef"));

    QCOMPARE(controller.uiMainColor(), QStringLiteral("#8460ff"));
    QCOMPARE(controller.uiProgressStartColor(), QStringLiteral("#8460ff"));
    QCOMPARE(controller.uiProgressEndColor(), QStringLiteral("#ff5e9a"));
    QCOMPARE(themeColorsChanged.count(), 0);
}

QTEST_MAIN(ApplicationSettingsTest)
#include "tst_application_settings.moc"
