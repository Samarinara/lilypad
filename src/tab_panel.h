#pragma once
#include <QWidget>
#include <QMap>

class TabManager;
class TabEntryWidget;
class QVBoxLayout;
class QScrollArea;
class QPushButton;

class TabPanel : public QWidget {
    Q_OBJECT
public:
    explicit TabPanel(TabManager* manager, QWidget* parent = nullptr);

    // Accessors for testing
    int widgetCount() const { return m_widgets.size(); }
    TabEntryWidget* widgetForTab(int tabId) const { return m_widgets.value(tabId, nullptr); }
    QList<int> tabOrder() const;  // returns tab IDs in display order (top to bottom)

    static constexpr int kPanelWidth = 220;

signals:
    void newTabRequested();
    void tabCloseRequested(int tabId);
    void tabSwitchRequested(int tabId);

private slots:
    void onTabCreated(int tabId);
    void onTabClosed(int tabId);
    void onActiveTabChanged(int oldId, int newId);
    void onTitleChanged(int tabId, const QString& title);
    void onFaviconChanged(int tabId, const QPixmap& pixmap);
    void onLoadingStateChanged(int tabId, bool isLoading);

private:
    TabManager*                  m_tabManager;
    QVBoxLayout*                 m_listLayout;
    QMap<int, TabEntryWidget*>   m_widgets;
    QPushButton*                 m_newTabBtn;

    void addTabWidget(int tabId);
    void removeTabWidget(int tabId);
    void setActiveTab(int tabId);
    void updateNewTabButton();
};
