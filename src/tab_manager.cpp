#include "tab_manager.h"
#include "cef_handler.h"
#include "include/cef_browser.h"
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

TabManager::TabManager(QWindow* window, QObject* parent)
    : QObject(parent), m_window(window)
{
}

TabManager::~TabManager()
{
    for (TabEntry* entry : m_tabs) {
        delete entry;
    }
    m_tabs.clear();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

int TabManager::createTab(const QString& url)
{
    if (m_tabs.size() >= kMaxTabs)
        return -1;

    TabEntry* entry = new TabEntry();
    int tabId = m_nextTabId++;
    entry->tabId = tabId;
    entry->url   = url;

    entry->handler = new CefHandler(m_window, this);
    entry->handler->tabId        = tabId;
    entry->handler->m_tabManager = this;

    m_tabs.append(entry);

    // Start creation timeout timer
    QTimer* timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(kCreationTimeoutMs);
    m_creationTimers[tabId] = timer;

    connect(timer, &QTimer::timeout, this, [this, tabId]() {
        // onBrowserCreated has not fired — treat as initialization failure
        qWarning() << "TabManager: browser creation timed out for tab" << tabId;
        m_creationTimers.remove(tabId);
        removeTabEntry(tabId);
        emit tabClosed(tabId);
    });
    timer->start();

    // Create the CEF browser (native embedding)
    createBrowserImpl(entry);

    // Make this the active tab
    switchToTab(tabId);

    emit tabCreated(tabId);
    return tabId;
}

void TabManager::closeTab(int tabId)
{
    int idx = indexOfTab(tabId);
    if (idx < 0)
        return;

    // Last-tab guard: ensure there is always at least one tab
    if (m_tabs.size() == 1) {
        createTab("about:blank");
    }

    // Determine successor before removing
    if (tabId == m_activeTabId) {
        // Re-find index after potential createTab above
        int currentIdx = indexOfTab(tabId);
        int successorIdx = (currentIdx > 0) ? currentIdx - 1 : 0;
        // Make sure we don't pick the tab being closed
        TabEntry* successor = m_tabs[successorIdx];
        if (successor->tabId == tabId) {
            // Edge case: pick the other one
            successorIdx = (successorIdx + 1 < m_tabs.size()) ? successorIdx + 1 : successorIdx - 1;
            successor = m_tabs[successorIdx];
        }
        switchToTab(successor->tabId);
    }

    // Mark as closing so callbacks are ignored
    m_closingTabs.insert(tabId);

    TabEntry* entry = tabById(tabId);
    if (!entry)
        return;

    if (entry->handler && entry->handler->GetBrowser()) {
        entry->handler->GetBrowser()->GetHost()->CloseBrowser(true);
    } else {
        // Browser was never initialized — clean up directly
        onBeforeClose(tabId);
    }
}

void TabManager::switchToTab(int tabId)
{
    TabEntry* entry = tabById(tabId);
    if (!entry)
        return;

    if (tabId == m_activeTabId)
        return;

    int oldTabId = m_activeTabId;

    // Deactivate old active tab
    if (oldTabId != -1) {
        TabEntry* oldEntry = tabById(oldTabId);
        if (oldEntry)
            oldEntry->deactivate();
    }

    m_activeTabId = tabId;
    entry->activate();

    emit activeTabChanged(oldTabId, tabId);
}

void TabManager::closeAllTabs()
{
    // Initiate close on all tabs
    // Collect IDs first since m_tabs will be modified during callbacks
    QList<int> ids;
    for (TabEntry* e : m_tabs)
        ids.append(e->tabId);

    for (int id : ids) {
        TabEntry* entry = tabById(id);
        if (!entry)
            continue;
        m_closingTabs.insert(id);
        if (entry->handler && entry->handler->GetBrowser()) {
            entry->handler->GetBrowser()->GetHost()->CloseBrowser(true);
        } else {
            onBeforeClose(id);
        }
    }

    // Pump the event loop until all OnBeforeClose callbacks have fired,
    // or until the safety timeout expires.
    if (!m_tabs.isEmpty()) {
        QEventLoop loop;
        QTimer safetyTimer;
        safetyTimer.setSingleShot(true);
        safetyTimer.setInterval(5000);

        connect(&safetyTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
        QMetaObject::Connection conn = connect(this, &TabManager::tabClosed, this, [&]() {
            if (m_tabs.isEmpty())
                loop.quit();
        });

        safetyTimer.start();
        loop.exec();

        disconnect(conn);
    }
}

TabEntry* TabManager::activeTab() const
{
    return tabById(m_activeTabId);
}

TabEntry* TabManager::tabById(int tabId) const
{
    for (TabEntry* e : m_tabs) {
        if (e->tabId == tabId)
            return e;
    }
    return nullptr;
}

int TabManager::tabCount() const
{
    return m_tabs.size();
}

QList<TabEntry*> TabManager::allTabs() const
{
    return m_tabs;
}

// ---------------------------------------------------------------------------
// CefHandler callbacks
// ---------------------------------------------------------------------------

void TabManager::onBrowserCreated(int tabId, CefRefPtr<CefBrowser> /*browser*/)
{
    TabEntry* entry = tabById(tabId);
    if (!entry)
        return;

    // Cancel and delete the creation timeout timer
    if (m_creationTimers.contains(tabId)) {
        QTimer* timer = m_creationTimers.take(tabId);
        timer->stop();
        timer->deleteLater();
    }

    // If this tab is currently active, activate it now that the browser is ready
    if (tabId == m_activeTabId) {
        entry->activate();
    }
}

void TabManager::onBeforeClose(int tabId)
{
    TabEntry* entry = tabById(tabId);
    if (!entry)
        return;

    m_closingTabs.remove(tabId);

    removeTabEntry(tabId);
}

void TabManager::onTitleChanged(int tabId, const QString& title)
{
    if (m_closingTabs.contains(tabId))
        return;

    TabEntry* entry = tabById(tabId);
    if (!entry)
        return;

    entry->title = title;
    emit titleChanged(tabId, title);
}

void TabManager::onFaviconChanged(int tabId, const QPixmap& pixmap)
{
    if (m_closingTabs.contains(tabId))
        return;

    TabEntry* entry = tabById(tabId);
    if (!entry)
        return;

    entry->favicon = pixmap;
    emit faviconChanged(tabId, pixmap);
}

void TabManager::onLoadingStateChanged(int tabId, bool isLoading)
{
    if (m_closingTabs.contains(tabId))
        return;

    TabEntry* entry = tabById(tabId);
    if (!entry)
        return;

    entry->isLoading = isLoading;
    emit loadingStateChanged(tabId, isLoading);
}

void TabManager::onUrlChanged(int tabId, const QString& url)
{
    if (m_closingTabs.contains(tabId))
        return;

    TabEntry* entry = tabById(tabId);
    if (!entry)
        return;

    entry->url = url;
    emit urlChanged(tabId, url);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

int TabManager::indexOfTab(int tabId) const
{
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs[i]->tabId == tabId)
            return i;
    }
    return -1;
}

void TabManager::removeTabEntry(int tabId)
{
    int idx = indexOfTab(tabId);
    if (idx < 0)
        return;

    TabEntry* entry = m_tabs.takeAt(idx);

    // Cancel any pending creation timer
    if (m_creationTimers.contains(tabId)) {
        QTimer* timer = m_creationTimers.take(tabId);
        timer->stop();
        timer->deleteLater();
    }

    // The handler is ref-counted by CEF and will be released when the
    // CefRefPtr inside CefHandler goes out of scope.
    delete entry;

    emit tabClosed(tabId);
}

void TabManager::createBrowserImpl(TabEntry* entry)
{
    CefWindowInfo windowInfo;

    QRect rect = m_window->geometry();
    cef_rect_t cefRect(0, 0, rect.width(), rect.height());
    windowInfo.SetAsChild(static_cast<CefWindowHandle>(m_window->winId()), cefRect);

    CefBrowserSettings settings;

    CefBrowserHost::CreateBrowser(windowInfo, entry->handler,
                                  entry->url.toStdString(), settings,
                                  nullptr, nullptr);
}
