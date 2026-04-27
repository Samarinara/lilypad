#pragma once
#include <QMainWindow>

class QLineEdit;
class QWindow;
class TabManager;
class TabPanel;
class QWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    void createInitialTab();

protected:
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onUrlBarSubmit();
    void onActiveTabChanged(int oldId, int newId);
    void onUrlChanged(int tabId, const QString& url);

private:
    QLineEdit*      m_urlBar;
    TabManager*    m_tabManager;
    TabPanel*     m_tabPanel;
    QWindow*     m_cefWindow;
};
