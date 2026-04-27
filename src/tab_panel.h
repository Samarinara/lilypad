// ============================================================================
// TAB_PANEL.H
// ============================================================================
// This file defines the TabPanel class — the sidebar that shows ALL tabs.
// Think of it as Chrome's "tab strip" but on the LEFT side instead of the top.
//
// When you have multiple tabs open, this panel lists them all so you can see
// what's open and click to switch between them. It's like a "table of contents"
// for your browser tabs.
//
// In Lilypad, the sidebar shows:
// - List of all tabs (each is a TabEntryWidget)
// - New Tab button at bottom
// - Scroll if many tabs (overflow)
// ============================================================================

#ifndef LILYPAD_TAB_PANEL_H
#define LILYPAD_TAB_PANEL_H

// ============================================================================
// QT INCLUDES
// ============================================================================
// Qt framework for UI:
// ============================================================================
#include <QWidget>   // Base widget class
#include <QMap>     // Map for tabId → widget lookup

// ============================================================================.
// QT FORWARD DECLARATIONS
// ============================================================================
class TabManager;          // Tab manager (who we connect to)
class TabEntryWidget;     // Individual tab widget
class QVBoxLayout;       // Vertical layout
class QScrollArea;       // Scrollable container
class QPushButton;       // New tab button

// ============================================================================
// CLASS: TabPanel
// ============================================================================
// The sidebar panel showing all tabs.
//
// We connect to TabManager signals to know when tabs are created/closed/changed.
// We create TabEntryWidgets for each tab and manage their display.
//
// ANALOGY: Think of this like a playlist in a music app. When songs are
// added to the playlist, they appear in the list. When you click a song,
// it plays. Same concept - but for browser tabs.
// ============================================================================
class TabPanel : public QWidget {
    Q_OBJECT

public:
    // ============================================================================
    // CONSTRUCTOR
    // ============================================================================
    // Create a side panel managed by the given TabManager.
    //
    // Parameter: manager - The TabManager to connect to for tab events
    // ============================================================================
    explicit TabPanel(TabManager* manager, QWidget* parent = nullptr);

    // ============================================================================
    // ACCESSORS (FOR TESTING)
    // ============================================================================
    // These help unit tests verify the panel state.

    // Get number of tab widgets we created.
    int widgetCount() const { return m_widgets.size(); }

    // Get the widget for a specific tab.
    TabEntryWidget* widgetForTab(int tabId) const { return m_widgets.value(tabId, nullptr); }

    // Get tab IDs in display order (top to bottom).
    QList<int> tabOrder() const;

    // ============================================================================
    // CONSTANTS
    // ============================================================================
    // Fixed width of the sidebar panel.
    static constexpr int kPanelWidth = 220;

    // ============================================================================
    // SIGNALS
    // ============================================================================
    // Signals we emit when user interacts with tabs.

    // User clicked "+ New Tab" button.
    signals:
        void newTabRequested();

    // User clicked close button on a tab.
    void tabCloseRequested(int tabId);

    // User clicked a tab (to switch to it).
    void tabSwitchRequested(int tabId);

private slots:
    // ============================================================================
    // SLOTS (CONNECTED TO TAB MANAGER)
    // ============================================================================
    // These are called when TabManager sends signals.
    // Qt "slots" are methods that receive signal calls.
    // ============================================================================

    // Called when a new tab is created.
    void onTabCreated(int tabId);

    // Called when a tab is closed.
    void onTabClosed(int tabId);

    // Called when active tab changes.
    void onActiveTabChanged(int oldId, int newId);

    // Called when tab title changes.
    void onTitleChanged(int tabId, const QString& title);

    // Called when tab favicon changes.
    void onFaviconChanged(int tabId, const QPixmap& pixmap);

    // Called when loading state changes.
    void onLoadingStateChanged(int tabId, bool isLoading);

private:
    // ============================================================================
    // PRIVATE DATA MEMBERS
    // ============================================================================

    // The TabManager we're connected to.
    TabManager* m_tabManager;

    // Layout for the list of tab widgets.
    QVBoxLayout* m_listLayout;

    // Map of tabId → TabEntryWidget.
    // This lets us find/update widgets quickly.
    QMap<int, TabEntryWidget*> m_widgets;

    // The "+ New Tab" button at the bottom.
    QPushButton* m_newTabBtn;

    // ============================================================================
    // PRIVATE HELPER METHODS
    // ============================================================================

    // Add a new tab widget to our list.
    void addTabWidget(int tabId);

    // Remove a tab widget from our list.
    void removeTabWidget(int tabId);

    // Set which tab is visually active.
    void setActiveTab(int tabId);

    // Update new tab button (enable if under limit).
    void updateNewTabButton();
};

#endif // LILYPAD_TAB_PANEL_H