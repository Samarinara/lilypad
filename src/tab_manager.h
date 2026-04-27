#pragma once
#include <QObject>
#include <QList>
#include <QMap>
#include <QSet>
#include <QPixmap>
#include <QTimer>
#include "tab_entry.h"

class QWindow;

class TabManager : public QObject {
    Q_OBJECT
public:
    explicit TabManager(QWindow* window, QObject* parent = nullptr);
    ~TabManager();

    // Returns new tabId, or -1 if at limit (50 tabs)
    int  createTab(const QString& url);
    void closeTab(int tabId);
    void switchToTab(int tabId);
    void closeAllTabs();

    TabEntry* activeTab() const;
    TabEntry* tabById(int tabId) const;
    int       tabCount() const;
    QList<TabEntry*> allTabs() const;

    // Called by CefHandler callbacks (run on Qt main thread)
    void onBrowserCreated(int tabId, CefRefPtr<CefBrowser> browser);
    void onBeforeClose(int tabId);
    void onTitleChanged(int tabId, const QString& title);
    void onFaviconChanged(int tabId, const QPixmap& pixmap);
    void onLoadingStateChanged(int tabId, bool isLoading);
    void onUrlChanged(int tabId, const QString& url);

signals:
    void tabCreated(int tabId);
    void tabClosed(int tabId);
    void activeTabChanged(int oldTabId, int newTabId);
    void titleChanged(int tabId, const QString& title);
    void faviconChanged(int tabId, const QPixmap& pixmap);
    void loadingStateChanged(int tabId, bool isLoading);
    void urlChanged(int tabId, const QString& url);

protected:
    // Overridable hook for CEF browser creation — extracted so tests can
    // subclass TabManager and bypass CefBrowserHost::CreateBrowser.
    virtual void createBrowserImpl(TabEntry* entry);

private:
    QList<TabEntry*>   m_tabs;
    int                m_activeTabId = -1;
    int                m_nextTabId   = 1;
    QWindow*          m_window;
    QMap<int, QTimer*> m_creationTimers;  // tabId -> timeout timer
    QSet<int>          m_closingTabs;     // tabs waiting for OnBeforeClose

    static constexpr int kMaxTabs           = 50;
    static constexpr int kCreationTimeoutMs = 10000;

    int  indexOfTab(int tabId) const;
    void removeTabEntry(int tabId);
};
