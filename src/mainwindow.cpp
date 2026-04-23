#include "mainwindow.h"
#include "cef_osr_widget.h"
#include <QLineEdit>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_handler(nullptr)
{
    setWindowTitle("Lilypad");
    resize(1200, 800);

    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *lay = new QVBoxLayout(central);

    m_urlBar = new QLineEdit("https://polli.page");
    lay->addWidget(m_urlBar);

    m_osrWidget = new CefOSRWidget(central);
    lay->addWidget(m_osrWidget, 1);

    connect(m_urlBar, &QLineEdit::returnPressed, [this](){
        if (m_handler && m_handler->GetBrowser())
            m_handler->GetBrowser()->GetMainFrame()->LoadURL(m_urlBar->text().toStdString());
    });
}

void MainWindow::createBrowser(const QString &url){
    m_handler = new CefHandler(m_osrWidget, this);
    m_osrWidget->setHandler(m_handler);

    CefWindowInfo window_info;
    window_info.SetAsWindowless(m_osrWidget->winId());

    CefBrowserSettings settings;
    settings.windowless_frame_rate = 30;

    CefBrowserHost::CreateBrowser(window_info, m_handler,
                                   url.toStdString(), settings,
                                   nullptr,
                                   nullptr);
}