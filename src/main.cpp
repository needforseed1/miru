#include <QByteArray>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QSettings>

#include "app/ApplicationController.h"
#include "app/CachingNetworkFactory.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName("AIOStreams Linux");
    QCoreApplication::setOrganizationName("AIOStreamsLinux");

    // Apply a saved UI zoom before the GUI starts (Qt reads QT_SCALE_FACTOR
    // once at startup). On top of the compositor's own scaling.
    if (!qEnvironmentVariableIsSet("QT_SCALE_FACTOR")) {
        const double scale = QSettings().value(QStringLiteral("ui/scaleFactor"), 1.0).toDouble();
        if (scale > 0.0 && !qFuzzyCompare(scale, 1.0)) {
            qputenv("QT_SCALE_FACTOR", QByteArray::number(scale, 'g', 4));
        }
    }

    // Let fractional desktop scaling (1.25, 1.5, …) pass through instead of
    // being rounded to an integer — important for crisp sizing on Wayland.
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QGuiApplication app(argc, argv);
    app.setApplicationDisplayName(QStringLiteral("Miru"));
    QQuickStyle::setStyle("Basic");

    ApplicationController controller;

    QQmlApplicationEngine engine;
    // Persist poster artwork to disk so it isn't re-downloaded every launch.
    static CachingNetworkFactory networkFactory;
    engine.setNetworkAccessManagerFactory(&networkFactory);
    engine.rootContext()->setContextProperty("appController", &controller);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("StremioLinux", "Main");

    return app.exec();
}
