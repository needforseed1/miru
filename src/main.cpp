#include <QByteArray>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QSettings>
#include <QSslSocket>

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

#if defined(Q_OS_LINUX)
    // Let fractional desktop scaling (1.25, 1.5, ...) pass through instead of
    // being rounded to an integer. This is primarily a Wayland/Linux concern;
    // keep macOS on Qt's native high-DPI policy to avoid odd pointer mapping
    // when Cocoa and Qt translate between logical and device pixels.
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    QGuiApplication app(argc, argv);
    app.setApplicationDisplayName(QStringLiteral("Miru"));
    QQuickStyle::setStyle("Basic");

#ifdef Q_OS_MACOS
    // Qt defaults to Secure Transport on macOS, which tops out at TLS 1.2 and
    // fails against TLS 1.3-only servers (handshake error -9836). Prefer the
    // OpenSSL backend; fall back if the bundled libssl/libcrypto won't load.
    if (QSslSocket::availableBackends().contains(QStringLiteral("openssl"))) {
        QSslSocket::setActiveBackend(QStringLiteral("openssl"));
        if (!QSslSocket::supportsSsl()) {
            QSslSocket::setActiveBackend(QStringLiteral("securetransport"));
        }
    }
#endif

    // Bundled UI font; named instances (Medium/SemiBold/…) map to QML
    // font.weight. Fall back to the system font if registration fails.
    if (QFontDatabase::addApplicationFont(
            QStringLiteral(":/resources/fonts/InterVariable.ttf")) != -1) {
        QFont uiFont(QStringLiteral("Inter Variable"));
        uiFont.setPixelSize(14);
        app.setFont(uiFont);
    }

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
