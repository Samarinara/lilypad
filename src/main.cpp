#include <QApplication>
#include <QTimer>
#include <clocale>
#include <cstdlib>
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
    settings.windowless_rendering_enabled = false;

    // Use $HOME/.cache/lilypad as the cache dir. QStandardPaths isn't
    // available yet (QApplication hasn't been constructed), so derive it
    // from the environment directly.
    const char* home = std::getenv("HOME");
    const std::string cache_dir = std::string(home ? home : "/tmp") + "/.cache/lilypad";
    CefString(&settings.root_cache_path).FromString(cache_dir);

    CefRefPtr<QCefApp> app(new QCefApp);
    if (!CefInitialize(main_args, settings, app, nullptr))
        return 1;

    // QApplication must come after CefInitialize. Qt's constructor calls
    // setlocale(LC_ALL, "") which resets the locale; restore "C" immediately
    // so Blink's locale DCHECK passes.
    QApplication qapp(argc, argv);
    std::setlocale(LC_ALL, "C");

    MainWindow w;
    w.show();
    w.createInitialTab();

    QTimer cef_timer;
    QObject::connect(&cef_timer, &QTimer::timeout, [](){
        CefDoMessageLoopWork();
    });
    cef_timer.start(10);

    int ret = qapp.exec();

    CefShutdown();
    return ret;
}