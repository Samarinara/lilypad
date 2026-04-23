#pragma once
#include <QMainWindow>
class QLineEdit;
class CefOSRWidget;
class CefHandler;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    void createBrowser(const QString &url);

private:
    QLineEdit  *m_urlBar;
    CefOSRWidget *m_osrWidget;
    CefHandler *m_handler;
};