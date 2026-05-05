// ============================================================================
// MAIN.CPP - QML VERSION
// ============================================================================
// Entry point for the Lilypad browser application using QML.
//
// This file:
// 1. Initialize Qt application
// 2. Set up Chromium/WebEngine flags
// 3. Create QQmlApplicationEngine
// 4. Expose TabManager to QML
// 5. Load Main.qml
// 6. Run the event loop
// ============================================================================

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml>
#include <QSurfaceFormat>
#include <QCoreApplication>
#include <QtWebEngineQuick/QtWebEngineQuick>
#include <QQuickStyle>
#include "tab_manager.h"
#include "tab_entry.h"
#include "urlprocessor.h"

int main(int argc, char *argv[])
{
    // Set OpenGL format for the application
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    QSurfaceFormat::setDefaultFormat(format);

    // Force GBM disable - fixes flickering on Wayland with Qt WebEngine
    qputenv("QTWEBENGINE_FORCE_USE_GBM", "0");

    // Pass Chromium flags to enable GPU acceleration and fix context loss
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
        "--enable-gpu-rasterization "
        "--ignore-gpu-blocklist "
        "--enable-zero-copy "
        "--enable-features=WebShare "
        "--ozone-platform-hint=auto "
        "--unsafely-treat-insecure-origin-as-secure=http://figma.com,http://www.figma.com "
        "--user-data-dir=/tmp/lilypad-chrome-data"
    );

    // Force OpenGL backend for Qt Quick (used by WebEngine internally)
    qputenv("QSG_RHI_BACKEND", "opengl");

    // Initialize Qt WebEngine FIRST (must be before QGuiApplication)
    QtWebEngineQuick::initialize();

    QGuiApplication app(argc, argv);
    app.setApplicationName("Lilypad");

    // Force Basic style to ignore system and user themes
    QQuickStyle::setStyle("Basic");

    // Create TabManager and UrlProcessor
    TabManager tabManager;
    UrlProcessor urlProcessor;

    QQmlApplicationEngine engine;

    // Add QML import path for theme module
    engine.addImportPath("qrc:/qml");

    // Expose C++ objects to QML
    engine.rootContext()->setContextProperty("tabManager", &tabManager);
    engine.rootContext()->setContextProperty("urlProcessor", &urlProcessor);

    // Load the main QML file
    const QUrl url(QStringLiteral("qrc:/qml/Main.qml"));
    qDebug() << "Loading QML from:" << url.toString();
    engine.load(url);
    qDebug() << "Root objects:" << engine.rootObjects().size();

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
