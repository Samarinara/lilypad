// Property-based tests for TabPanel using rapidcheck + Qt Test.
//
// These tests use StubTabManager (same pattern as test_tab_manager_props.cpp)
// to drive the panel without requiring a live CEF instance.
//
// Properties tested:
//   Property  8 — Tab panel order matches creation order     (Req 1.5)
//   Property 12 — Close removes tab from panel               (Req 5.1, 5.5)

#include <QtTest/QtTest>
#include <QApplication>
#include <rapidcheck.h>

#include "tab_manager.h"
#include "tab_entry.h"
#include "tab_panel.h"

// ---------------------------------------------------------------------------
// StubTabManager — bypasses CefBrowserHost::CreateBrowser
// ---------------------------------------------------------------------------

class StubTabManager : public TabManager {
public:
    explicit StubTabManager(QObject* parent = nullptr) : TabManager(parent) {}

protected:
    void createBrowserImpl(TabEntry* entry) override {
        entry->container = new QWidget();
        onBrowserCreated(entry->tabId);
    }
};

// ---------------------------------------------------------------------------
// Helper: create `n` tabs and return the list of IDs
// ---------------------------------------------------------------------------
static QList<int> populateTabs(StubTabManager& mgr, int n)
{
    QList<int> ids;
    for (int i = 0; i < n; ++i) {
        int id = mgr.createTab(QString("https://example.com/%1").arg(i));
        if (id != -1)
            ids.append(id);
    }
    return ids;
}

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------

class TestTabPanelProps : public QObject {
    Q_OBJECT

private slots:
    // Property 8: Tab panel order matches creation order
    // Validates: Requirements 1.5
    void prop8_tabPanelOrderMatchesCreationOrder();

    // Property 12 (panel side): Close removes tab from panel
    // Validates: Requirements 5.1, 5.5
    void prop12_closeRemovesTabFromPanel();
};

// ---------------------------------------------------------------------------
// Property 8: Tab panel order matches creation order
//
// For any sequence of createTab calls [1, 20], the order of TabEntryWidget
// rows in the TabPanel shall match the order in which the tabs were created
// (top to bottom, oldest to newest).
//
// Validates: Requirements 1.5
// ---------------------------------------------------------------------------
void TestTabPanelProps::prop8_tabPanelOrderMatchesCreationOrder()
{
    QVERIFY(rc::check(
        "Property 8: TabPanel row order matches tab creation order",
        []() {
            // Number of tabs to create: [1, 20]
            const int numTabs = *rc::gen::inRange(1, 21);

            StubTabManager mgr;
            TabPanel panel(&mgr);

            QList<int> createdIds = populateTabs(mgr, numTabs);
            RC_ASSERT(createdIds.size() == numTabs);

            // allTabs() returns tabs in creation order
            QList<TabEntry*> allTabs = mgr.allTabs();
            RC_ASSERT(allTabs.size() == numTabs);

            // tabOrder() must match allTabs() order
            QList<int> panelOrder = panel.tabOrder();
            RC_ASSERT(panelOrder.size() == numTabs);

            for (int i = 0; i < numTabs; ++i) {
                RC_ASSERT(panelOrder[i] == allTabs[i]->tabId);
            }
        }
    ));
}

// ---------------------------------------------------------------------------
// Property 12 (panel side): Close removes tab from panel
//
// For any tab list [1, 10] and a random tab to close:
//   - After closeTab(id), TabPanel has no TabEntryWidget for the closed ID
//   - widgetCount() decreased by 1 (accounting for last-tab guard)
//
// Validates: Requirements 5.1, 5.5
// ---------------------------------------------------------------------------
void TestTabPanelProps::prop12_closeRemovesTabFromPanel()
{
    QVERIFY(rc::check(
        "Property 12 (panel): after closeTab, panel has no widget for closed ID",
        []() {
            const int numTabs = *rc::gen::inRange(1, 11);

            StubTabManager mgr;
            TabPanel panel(&mgr);

            QList<int> ids = populateTabs(mgr, numTabs);
            RC_ASSERT(ids.size() == numTabs);
            RC_ASSERT(panel.widgetCount() == numTabs);

            // Pick a random tab to close
            const int closeIdx = *rc::gen::inRange(0, numTabs);
            const int closeId  = ids[closeIdx];

            const int widgetsBefore = panel.widgetCount();
            mgr.closeTab(closeId);

            // The closed tab must have no widget in the panel
            RC_ASSERT(panel.widgetForTab(closeId) == nullptr);

            if (widgetsBefore == 1) {
                // Last-tab guard: a replacement tab was opened, so count stays 1
                RC_ASSERT(panel.widgetCount() == 1);
            } else {
                RC_ASSERT(panel.widgetCount() == widgetsBefore - 1);
            }
        }
    ));
}

QTEST_MAIN(TestTabPanelProps)
#include "test_tab_panel_props.moc"
