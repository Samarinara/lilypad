// ============================================================================
// TAB_MANAGER.CPP
// ============================================================================
// This file contains the implementation of TabManager's methods.
// This is where the tab lifecycle logic lives: creating tabs,
// closing tabs, switching tabs, handling callbacks, etc.
// ============================================================================

// ============================================================================
// INCLUDES
// ============================================================================
#include "tab_manager.h"
#include "tab_entry.h"
#include <QEventLoop>  // For waiting during closeAllTabs
#include <QTimer>     // For creation timeout
#include <QDebug>    // For qWarning() debugging output
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QVBoxLayout>

// ============================================================================
// CONSTRUCTOR
// ============================================================================
// Initialize a new TabManager.
// ============================================================================
TabManager::TabManager(QObject* parent)
    : QObject(parent)
{
    // m_tabs starts empty, other members have defaults in header
}

// ============================================================================
// DESTRUCTOR
// ============================================================================
// Clean up when TabManager is destroyed.
// We need to delete all TabEntry objects we created.
// ============================================================================
TabManager::~TabManager()
{
    for (TabEntry* entry : m_tabs) {
        delete entry;
    }
    m_tabs.clear();
}

// ============================================================================
// ============================================================================
// SECTION: PUBLIC API - Tab Operations
// ============================================================================
// ============================================================================

// ============================================================================
// METHOD: createTab
// ============================================================================
// Create a new browser tab with the given URL.
//
// STEPS:
// 1. Check tab limit (can't have more than 50 tabs)
// 2. Create TabEntry with unique ID
// 3. Start a creation timeout timer
// 4. Create QWebEngineView
// 5. Make this the active tab
// 6. Notify listeners
// ============================================================================
int TabManager::createTab(const QString& url)
{
    // Check the limit first - can't have more than 50 tabs
    if (m_tabs.size() >= kMaxTabs)
        return -1;

    // Create the tab entry (data structure for this tab)
    TabEntry* entry = new TabEntry();

    // Assign a unique ID and store the URL
    int tabId = m_nextTabId++;
    entry->tabId = tabId;
    entry->url   = url;

    // Add to our list of tabs
    m_tabs.append(entry);

    // ============================================================================
    // CREATION TIMEOUT TIMER
    // ============================================================================
    QTimer* timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(kCreationTimeoutMs);
    m_creationTimers[tabId] = timer;

    connect(timer, &QTimer::timeout, this, [this, tabId]() {
        qWarning() << "TabManager: browser creation timed out for tab" << tabId;
        m_creationTimers.remove(tabId);
        removeTabEntry(tabId);
        emit tabClosed(tabId);
    });
    timer->start();

    // ============================================================================
    // CREATE THE QWebEngineView
    // ============================================================================
    createBrowserImpl(entry);

    // ============================================================================
    // MAKE THIS THE ACTIVE TAB
    // ============================================================================
    switchToTab(tabId);

    // Notify UI that we created a new tab
    emit tabCreated(tabId);
    return tabId;
}

// ============================================================================
// METHOD: closeTab
// ============================================================================
// Close a specific tab.
// ============================================================================
void TabManager::closeTab(int tabId)
{
    int idx = indexOfTab(tabId);
    if (idx < 0)
        return;

    // ============================================================================
    // LAST TAB GUARD
    // ============================================================================
    if (m_tabs.size() == 1) {
        createTab("about:blank");
    }

    // ============================================================================
    // DETERMINE SUCCESSOR BEFORE REMOVING
    // ============================================================================
    if (tabId == m_activeTabId) {
        int currentIdx = indexOfTab(tabId);
        int successorIdx = (currentIdx > 0) ? currentIdx - 1 : 0;

        TabEntry* successor = m_tabs[successorIdx];
        if (successor->tabId == tabId) {
            successorIdx = (successorIdx + 1 < m_tabs.size()) ? successorIdx + 1 : successorIdx - 1;
            successor = m_tabs[successorIdx];
        }
        switchToTab(successor->tabId);
    }

    // Mark as closing so callbacks are ignored
    m_closingTabs.insert(tabId);

    // Get the entry
    TabEntry* entry = tabById(tabId);
    if (!entry)
        return;

    // Close the view
    onBeforeClose(tabId);
}

// ============================================================================
// METHOD: switchToTab
// ============================================================================
// Switch to a different tab.
// ============================================================================
void TabManager::switchToTab(int tabId)
{
    TabEntry* entry = tabById(tabId);
    if (!entry)
        return;

    if (tabId == m_activeTabId)
        return;

    // Save old tab ID for signal
    int oldTabId = m_activeTabId;

    // ============================================================================
    // DEACTIVATE OLD TAB
    // ============================================================================
    if (oldTabId != -1) {
        TabEntry* oldEntry = tabById(oldTabId);
        if (oldEntry)
            oldEntry->deactivate();
    }

    // ============================================================================
    // ACTIVATE NEW TAB
    // ============================================================================
    m_activeTabId = tabId;
    entry->activate();

    emit activeTabChanged(oldTabId, tabId);
}

// ============================================================================
// METHOD: closeAllTabs
// ============================================================================
void TabManager::closeAllTabs()
{
    QList<int> ids;
    for (TabEntry* e : m_tabs)
        ids.append(e->tabId);

    for (int id : ids) {
        TabEntry* entry = tabById(id);
        if (!entry)
            continue;

        m_closingTabs.insert(id);
        onBeforeClose(id);
    }

    // Wait for all tabs to close
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

// ============================================================================
// ACCESSOR METHODS
// ============================================================================
TabEntry* TabManager::activeTab() const {
    return tabById(m_activeTabId);
}

TabEntry* TabManager::tabById(int tabId) const {
    for (TabEntry* e : m_tabs) {
        if (e->tabId == tabId)
            return e;
    }
    return nullptr;
}

int TabManager::tabCount() const {
    return m_tabs.size();
}

QList<TabEntry*> TabManager::allTabs() const {
    return m_tabs;
}

// ============================================================================
// ============================================================================
// SECTION: CALLBACK METHODS
// ============================================================================
// ============================================================================

void TabManager::onBrowserCreated(int tabId)
{
    TabEntry* entry = tabById(tabId);
    if (!entry)
        return;

    if (m_creationTimers.contains(tabId)) {
        QTimer* timer = m_creationTimers.take(tabId);
        timer->stop();
        timer->deleteLater();
    }

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

// ============================================================================
// ============================================================================
// SECTION: Private Helpers
// ============================================================================
// ============================================================================

int TabManager::indexOfTab(int tabId) const {
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

    if (m_creationTimers.contains(tabId)) {
        QTimer* timer = m_creationTimers.take(tabId);
        timer->stop();
        timer->deleteLater();
    }

    // Properly clean up Qt WebEngine resources
    // Delete container first (which owns the QWebEngineView via layout)
    if (entry->container) {
        entry->container->deleteLater();
        entry->container = nullptr;
    }
    entry->view = nullptr;  // view is deleted when container is deleted

    delete entry;
    emit tabClosed(tabId);
}

// ============================================================================
// METHOD: createBrowserImpl
// ============================================================================
void TabManager::createBrowserImpl(TabEntry* entry)
{
    // Create the QWebEngineView
    entry->view = new QWebEngineView();

    // Create container widget
    entry->container = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(entry->container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(entry->view);
    entry->container->setLayout(layout);

    // Load the URL
    entry->view->load(QUrl(entry->url));

    // Connect signals
    QWebEnginePage* page = entry->view->page();

    QObject::connect(page, &QWebEnginePage::titleChanged, [this, entry](const QString& title) {
        onTitleChanged(entry->tabId, title);
    });

    QObject::connect(page, &QWebEnginePage::urlChanged, [this, entry](const QUrl& url) {
        onUrlChanged(entry->tabId, url.toString());
    });

    QObject::connect(page, &QWebEnginePage::loadStarted, [this, entry]() {
        onLoadingStateChanged(entry->tabId, true);
    });

    QObject::connect(page, &QWebEnginePage::loadFinished, [this, entry](bool) {
        onLoadingStateChanged(entry->tabId, false);
    });

    // Notify MainWindow that the view container is ready
    emit tabViewCreated(entry->tabId, entry->container);

    onBrowserCreated(entry->tabId);
}
