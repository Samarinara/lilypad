// ============================================================================
// TAB_MANAGER.H
// ============================================================================
// This file defines the TabManager class — the "brain" of Lilypad's tab system.
// TabManager orchestrates all browser tabs: creating them, closing them, switching
// between them, and handling communication between QWebEngine and our UI.
//
// Think of TabManager as the "manager" you'd report to in an office.
// It's not the browser itself (that's QWebEngineView), but it keeps track of all
// your tabs, knows which one is active, and handles requests to create
// or close tabs.
// ============================================================================

#ifndef LILYPAD_TAB_MANAGER_H
#define LILYPAD_TAB_MANAGER_H

// ============================================================================
// QT INCLUDES
// ============================================================================
// Qt framework includes we need for:
// - QObject: Base for objects with signals/slots
// - QList: Dynamic array container (like std::vector but Qt-style)
// - QMap: Key-value map (like std::unordered_map)
// - QSet: Collection of unique values
// - QPixmap: Image for favicons
// - QTimer: For tracking tab creation
// ============================================================================
#include <QObject>    // Base class with signal/slot support
#include <QList>     // Qt list container
#include <QMap>      // Qt map (key-value store)
#include <QSet>      // Qt set (unique values)
#include <QPixmap>   // Qt image
#include <QTimer>    // For timeouts

// ============================================================================
// LOCAL INCLUDES
// ============================================================================
// TabEntry is our tab data structure. We need its definition
// because TabManager manages lists of TabEntry objects.
#include "tab_entry.h"

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
class QWidget;    // Base widget

// ============================================================================
// CLASS: TabManager
// ============================================================================
// TabManager is a QObject so it can use Qt's signal/slot system.
// This allows it to notify other parts of the application when
// tab state changes (title changed, URL changed, tab closed, etc.).
// ============================================================================
class TabManager : public QObject {
    // Enable Qt's meta-object system
    Q_OBJECT

public:
    // ============================================================================
    // CONSTRUCTOR
    // ============================================================================
    // Create a new TabManager.
    //
    // Parameters:
    //   parent - Parent QObject for cleanup hierarchy
    explicit TabManager(QObject* parent = nullptr);

    // ============================================================================
    // DESTRUCTOR
    // ============================================================================
    // Clean up when TabManager is destroyed.
    // We delete all TabEntry objects we created.
    // ============================================================================
    ~TabManager();

    // ============================================================================
    // PUBLIC API: Tab Operations
    // ============================================================================
    // These methods are called by MainWindow when the user
    // interacts with tabs (new tab, close tab, switch tab).
    // ============================================================================

    // Create a new tab with the given URL.
    //
    // Returns: The new tab's unique ID, or -1 if we hit the limit.
    //
    // The limit is 50 tabs — this prevents a user from
    // accidentally creating thousands of tabs and crashing the app.
    int createTab(const QString& url);

    // Close a specific tab.
    //
    // Parameter: tabId - The ID of the tab to close.
    //
    // Note: Always keeps at least one tab open.
    void closeTab(int tabId);

    // Switch to a specific tab.
    //
    // Parameter: tabId - The ID to switch to.
    //
    // The previously active tab becomes inactive
    // (QWebEngineView is hidden to save resources).
    void switchToTab(int tabId);

    // Close ALL tabs. Called when the app is shutting down.
    void closeAllTabs();

    // ============================================================================
    // ACCESSORS
    // ============================================================================
    // Various ways to get tab information.
    // ============================================================================

    // Get the currently active tab.
    TabEntry* activeTab() const;

    // Find a tab by its ID.
    TabEntry* tabById(int tabId) const;

    // Get total number of tabs.
    int tabCount() const;

    // Get ALL tabs as a list.
    QList<TabEntry*> allTabs() const;

    // ============================================================================
    // CALLBACK METHODS (called by TabEntry/WebView signals)
    // ============================================================================
    void onTitleChanged(int tabId, const QString& title);
    void onFaviconChanged(int tabId, const QPixmap& pixmap);
    void onLoadingStateChanged(int tabId, bool isLoading);
    void onUrlChanged(int tabId, const QString& url);
    void onBrowserCreated(int tabId);
    void onBeforeClose(int tabId);

    // ============================================================================
    // SIGNALS (NOTIFICATIONS TO UI)
    // ============================================================================
    // Qt signals that notify other parts of the app when
    // tab state changes. Any component can connect to these
    // to be notified of updates.
    // ============================================================================
signals:
    // Emitted when a new tab is created.
    // Parameter: tabId of the new tab.
    void tabCreated(int tabId);

    // Emitted when the tab's view container is ready.
    // Parameters: tabId and container widget.
    void tabViewCreated(int tabId, QWidget* container);

    // Emitted when a tab is closed.
    // Parameter: tabId of the closed tab.
    void tabClosed(int tabId);

    // Emitted when active tab changes.
    // Parameters: old tab ID, new tab ID.
    void activeTabChanged(int oldTabId, int newTabId);

    // Emitted when tab title changes.
    void titleChanged(int tabId, const QString& title);

    // Emitted when favicon changes.
    void faviconChanged(int tabId, const QPixmap& pixmap);

    // Emitted when loading state changes.
    void loadingStateChanged(int tabId, bool isLoading);

    // Emitted when URL changes.
    void urlChanged(int tabId, const QString& url);

protected:
    // ============================================================================
    // VIRTUAL METHOD (FOR TESTING)
    // ============================================================================
    // createBrowserImpl is "extracted" so tests can override it.
    //
    // In unit tests, we often don't want to actually create
    // a QWebEngineView (it's slow and complex). By making this
    // virtual, tests can subclass TabManager and provide
    // mock/browser-less implementations.
    // ============================================================================
    virtual void createBrowserImpl(TabEntry* entry);

private:
    // ============================================================================
    // PRIVATE DATA MEMBERS
    // ============================================================================
    // These are the internal state that TabManager tracks.
    // ============================================================================

    // All tabs we manage, in order.
    // QList is like std::vector but with Qt API.
    QList<TabEntry*> m_tabs;

    // The currently active tab's ID (-1 if none).
    int m_activeTabId = -1;

    // Next tab ID to assign.
    // We increment this each time we create a tab.
    // Starts at 1, not 0.
    int m_nextTabId = 1;

    // Creation timeout timers.
    // Key: tabId, Value: QTimer pointer.
    QMap<int, QTimer*> m_creationTimers;

    // Tabs that are currently closing.
    // We add tab IDs here when closeTab is called.
    QSet<int> m_closingTabs;

    // ============================================================================
    // CONSTANTS
    // ============================================================================
    // These define limits and timeouts.
    // ============================================================================

    // Maximum number of tabs allowed.
    static constexpr int kMaxTabs = 50;

    // Browser creation timeout in milliseconds.
    static constexpr int kCreationTimeoutMs = 10000;

    // ============================================================================
    // PRIVATE HELPER METHODS
    // ============================================================================
    // Internal methods that support the public API.
    // ============================================================================

    // Find the index (position) of a tab in our list.
    // Returns -1 if not found.
    int indexOfTab(int tabId) const;

    // Remove a tab entry and clean up resources.
    void removeTabEntry(int tabId);
};

#endif // LILYPAD_TAB_MANAGER_H
