// ============================================================================
// TAB_PANEL.CPP
// ============================================================================
// This file contains the implementation of TabPanel — the sidebar that
// manages all tab widgets and coordinates with TabManager.
// ============================================================================

// ============================================================================
// INCLUDES
// ============================================================================
#include "tab_panel.h"
#include "tab_manager.h"
#include "tab_entry_widget.h"
#include "tab_entry.h"

#include <QVBoxLayout>    // Vertical layout
#include <QScrollArea>   // Scrollable area
#include <QPushButton>  // New tab button
#include <QWidget>     // Widget base

// ============================================================================
// CONSTRUCTOR
// ============================================================================
// Set up the sidebar panel with scrolling and tab list.
// ============================================================================
TabPanel::TabPanel(TabManager* manager, QWidget* parent)
    : QWidget(parent)
    , m_tabManager(manager)
{
    // ============================================================================
    // SET PANEL WIDTH
    // ============================================================================
    // Fixed width for the sidebar.
    setFixedWidth(kPanelWidth);

    // ============================================================================
    // OUTER LAYOUT
    // ============================================================================
    // The main layout: scroll area + new tab button.
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    // ============================================================================
    // SCROLL AREA
    // ============================================================================
    // If there are many tabs, scroll instead of squishing them.
    // This holds the list of tab widgets.
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);  // Content resizes to fill area
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);  // No horizontal scroll
    scrollArea->setFrameShape(QFrame::NoFrame);  // No border

    // ============================================================================
    // LIST CONTAINER
    // ============================================================================
    // The actual list of tab widgets lives inside the scroll area.
    auto* listContainer = new QWidget(scrollArea);
    m_listLayout = new QVBoxLayout(listContainer);
    m_listLayout->setAlignment(Qt::AlignTop);   // Align to top
    m_listLayout->setSpacing(0);               // No space between tabs
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    listContainer->setLayout(m_listLayout);

    scrollArea->setWidget(listContainer);

    // ============================================================================
    // NEW TAB BUTTON
    // ============================================================================
    // Button at bottom to create new tabs.
    m_newTabBtn = new QPushButton(QStringLiteral("+ New Tab"), this);
    connect(m_newTabBtn, &QPushButton::clicked, this, [this]() {
        emit newTabRequested();
    });

    // ============================================================================
    // ADD TO OUTER LAYOUT
    // ============================================================================
    // Order: scroll area (expands), then new tab button.
    outerLayout->addWidget(scrollArea, 1);  // Stretch factor 1 = expand
    outerLayout->addWidget(m_newTabBtn);
    setLayout(outerLayout);

    // ============================================================================
    // CONNECT TO TAB MANAGER
    // ============================================================================
    // Subscribe to TabManager signals to stay in sync.
    // When tabs are created/closed/changed, we update accordingly.
    connect(m_tabManager, &TabManager::tabCreated,         this, &TabPanel::onTabCreated);
    connect(m_tabManager, &TabManager::tabClosed,          this, &TabPanel::onTabClosed);
    connect(m_tabManager, &TabManager::activeTabChanged,   this, &TabPanel::onActiveTabChanged);
    connect(m_tabManager, &TabManager::titleChanged,     this, &TabPanel::onTitleChanged);
    connect(m_tabManager, &TabManager::faviconChanged,   this, &TabPanel::onFaviconChanged);
    connect(m_tabManager, &TabManager::loadingStateChanged, this, &TabPanel::onLoadingStateChanged);

    // Initialize the button state
    updateNewTabButton();
}

// ============================================================================
// PUBLIC ACCESSOR: tabOrder
// ============================================================================
// Return tab IDs in display order (top to bottom).
// This iterates through the layout to find widgets.
// ============================================================================
QList<int> TabPanel::tabOrder() const
{
    QList<int> order;
    const int count = m_listLayout->count();

    // Iterate through layout items (left to right, top to bottom)
    for (int i = 0; i < count; ++i) {
        QLayoutItem* item = m_listLayout->itemAt(i);
        if (!item)
            continue;

        // Get the widget from the layout item
        QWidget* w = item->widget();
        if (!w)
            continue;

        // Cast to TabEntryWidget to get tabId
        auto* tew = qobject_cast<TabEntryWidget*>(w);
        if (tew)
            order.append(tew->tabId());
    }
    return order;
}

// ============================================================================
// ============================================================================
// SECTION: Private Helper Methods
// ============================================================================
// ============================================================================

// ============================================================================
// METHOD: addTabWidget
// ============================================================================
// Create and add a new tab widget for a tab.
// ============================================================================
void TabPanel::addTabWidget(int tabId)
{
    // Look up the tab entry from TabManager
    TabEntry* entry = m_tabManager->tabById(tabId);
    if (!entry)
        return;

    // Create a new widget for this tab
    auto* w = new TabEntryWidget(tabId, this);

    // Initialize with current state
    w->setTitle(entry->title);
    w->setUrl(entry->url);
    w->setFavicon(entry->favicon);
    w->setLoading(entry->isLoading);

    // Connect widget signals to our panel signals
    // When widget is clicked, we switch to that tab
    connect(w, &TabEntryWidget::clicked, this, [this, tabId]() {
        emit tabSwitchRequested(tabId);
    });

    // When close button clicked, we close that tab
    connect(w, &TabEntryWidget::closeClicked, this, [this, tabId]() {
        emit tabCloseRequested(tabId);
    });

    // Add to our layout
    m_listLayout->addWidget(w);

    // Track in our map
    m_widgets.insert(tabId, w);

    // Update new tab button (might have hit limit)
    updateNewTabButton();
}

// ============================================================================
// METHOD: removeTabWidget
// ============================================================================
// Remove and delete a tab widget.
// ============================================================================
void TabPanel::removeTabWidget(int tabId)
{
    // Find in our map
    auto it = m_widgets.find(tabId);
    if (it == m_widgets.end())
        return;

    // Get the widget
    TabEntryWidget* w = it.value();

    // Remove from layout and map
    m_listLayout->removeWidget(w);
    m_widgets.erase(it);

    // Delete the widget
    delete w;

    // Update new tab button
    updateNewTabButton();
}

// ============================================================================
// METHOD: setActiveTab
// ============================================================================
// Mark one tab as visually active.
// ============================================================================
void TabPanel::setActiveTab(int tabId)
{
    // First, set ALL to inactive
    for (auto* w : m_widgets)
        w->setActive(false);

    // Then, set target to active
    auto it = m_widgets.find(tabId);
    if (it != m_widgets.end())
        it.value()->setActive(true);
}

// ============================================================================
// METHOD: updateNewTabButton
// ============================================================================
// Enable/disable new tab button based on limit.
// ============================================================================
void TabPanel::updateNewTabButton()
{
    // Disable if at 50 tabs (limit)
    m_newTabBtn->setEnabled(m_tabManager->tabCount() < 50);
}

// ============================================================================
// ============================================================================
// SECTION: Slots (connected to TabManager signals)
// ============================================================================
// These methods run when TabManager emits signals.
// ============================================================================

// ============================================================================
// SLOT: onTabCreated
// ============================================================================
// New tab was created, add a widget for it.
// ============================================================================
void TabPanel::onTabCreated(int tabId)
{
    addTabWidget(tabId);
}

// ============================================================================
// SLOT: onTabClosed
// ============================================================================
// Tab was closed, remove its widget.
// ============================================================================
void TabPanel::onTabClosed(int tabId)
{
    removeTabWidget(tabId);
}

// ============================================================================
// SLOT: onActiveTabChanged
// ============================================================================
// Active tab changed, update visual active state.
// ============================================================================
void TabPanel::onActiveTabChanged(int /*oldId*/, int newId)
{
    setActiveTab(newId);
}

// ============================================================================
// SLOT: onTitleChanged
// ============================================================================
// Tab title changed, update its widget.
// ============================================================================
void TabPanel::onTitleChanged(int tabId, const QString& title)
{
    auto it = m_widgets.find(tabId);
    if (it != m_widgets.end())
        it.value()->setTitle(title);
}

// ============================================================================
// SLOT: onFaviconChanged
// ============================================================================
// Tab favicon changed, update its widget.
// ============================================================================
void TabPanel::onFaviconChanged(int tabId, const QPixmap& pixmap)
{
    auto it = m_widgets.find(tabId);
    if (it != m_widgets.end())
        it.value()->setFavicon(pixmap);
}

// ============================================================================
// SLOT: onLoadingStateChanged
// ============================================================================
// Loading state changed, update its widget.
// ============================================================================
void TabPanel::onLoadingStateChanged(int tabId, bool isLoading)
{
    auto it = m_widgets.find(tabId);
    if (it != m_widgets.end())
        it.value()->setLoading(isLoading);
}