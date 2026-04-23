#include <QApplication>
#include <QStandardPaths>
#include <QTimer>
#include "include/cef_app.h"
#include "include/cef_browser_process_handler.h"
#include "mainwindow.h"

class QCefApp : public CefApp, public CefBrowserProcessHandler
{
public:
    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }

    void OnBeforeCommandLineProcessing(
        const CefString& process_type,
        CefRefPtr<CefCommandLine> command_line) override {
        command_line->AppendSwitch("disable-gpu");
        command_line->AppendSwitch("disable-software-rasterizer");
        command_line->AppendSwitch("disable-gpu-compositing");
        command_line->AppendSwitch("disable-gpu-rasterization");
        command_line->AppendSwitch("disable-software-webgl");
        command_line->AppendSwitch("ignore-gpu-blocklist");
        command_line->AppendSwitch("single-process");
    }

private:
    IMPLEMENT_REFCOUNTING(QCefApp);
};

int main(int argc, char *argv[]){
    CefMainArgs main_args(argc, argv);

    int exit_code = CefExecuteProcess(main_args, nullptr, nullptr);
    if (exit_code >= 0)
        return exit_code;

    CefSettings settings;
    settings.no_sandbox = true;

    const std::string cache_dir =
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
            .toStdString() + "/lilypad";
    CefString(&settings.root_cache_path).FromString(cache_dir);

    CefRefPtr<QCefApp> app(new QCefApp);
    if (!CefInitialize(main_args, settings, app, nullptr))
        return 1;

    QApplication qapp(argc, argv);

    MainWindow w;
    w.show();
    QTimer::singleShot(0, [&w](){
        w.createBrowser("https://polli.page");
    });

    QTimer cef_timer;
    QObject::connect(&cef_timer, &QTimer::timeout, [](){
        CefDoMessageLoopWork();
    });
    cef_timer.start(10);

    int ret = qapp.exec();

    CefShutdown();
    return ret;
}