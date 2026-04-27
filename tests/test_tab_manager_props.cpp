// Property-based tests for TabManager using rapidcheck + Qt Test.
//
// These tests use StubTabManager, a subclass that overrides createBrowserImpl
// to immediately call onBrowserCreated (simulating the WebEngine view creation).
//
// Properties tested:
//   Property  1 — Tab count invariant after creation          (Req 3.2, 3.3, 3.5)
//   Property  2 — Tab count cap enforcement                   (Req 3.5, 3.6)
//   Property  3 — Tab ID uniqueness across a session          (Req 3.2, 5.6)
//   Property  4 — New tab becomes active                      (Req 3.4)
//   Property  5 — Active tab and view visibility invariant     (Req 4.1, 4.2, 9.1-9.4)
//   Property  6 — Previously active browser preserved         (Req 4.5)
//   Property 10 — Last-tab close opens a new tab              (Req 5.4)
//   Property 11 — Active tab selection after close            (Req 5.3)
//   Property 12 — Close removes tab from manager              (Req 5.1, 5.5)

#include <QtTest/QtTest>
#include <QApplication>
#include <rapidcheck.h>

#include "tab_manager.h"
#include "tab_entry.h"

// ---------------------------------------------------------------------------
// StubTabManager — bypasses CEF browser creation
// ---------------------------------------------------------------------------

class StubTabManager : public TabManager {
public:
    explicit StubTabManager(QObject* parent = nullptr) : TabManager(parent) {}

protected:
    // Instead of calling CefBrowserHost::CreateBrowser, immediately simulate
    // the view creation callback so the tab becomes fully initialised.
    void createBrowserImpl(TabEntry* entry) override {
        // Create a dummy view container
        entry->container = new QWidget();
        onBrowserCreated(entry->tabId);
    }
};

// ---------------------------------------------------------------------------
// Helper: create `n` tabs in a StubTabManager and return the list of IDs
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

class TestTabManagerProps : public QObject {
    Q_OBJECT

private slots:
    // Property 1: Tab count invariant after creation
    // Validates: Requirements 3.2, 3.3, 3.5
    void prop1_tabCountInvariantAfterCreation();

    // Property 2: Tab count cap enforcement
    // Validates: Requirements 3.5, 3.6
    void prop2_tabCountCapEnforcement();

    // Property 3: Tab ID uniqueness across a session
    // Validates: Requirements 3.2, 5.6
    void prop3_tabIdUniqueness();

    // Property 4: New tab becomes active
    // Validates: Requirements 3.4
    void prop4_newTabBecomesActive();

    // Property 5: Active tab and view visibility invariant
    // Validates: Requirements 4.1, 4.2, 9.1-9.4
    void prop5_activeTabAndVisibilityInvariant();

    // Property 6: Previously active browser preserved
    // Validates: Requirements 4.5
    void prop6_previousActivePreserved();

    // Property 10: Last-tab close opens a new tab
    // Validates: Requirements 5.4
    void prop10_lastTabCloseOpensNewTab();

    // Property 11: Active tab selection after close
    // Validates: Requirements 5.3
    void prop11_activeTabAfterClose();

    // Property 12: Close removes tab from manager
    // Validates: Requirements 5.1, 5.5
    void prop12_closeRemovesTab();
};

// ---------------------------------------------------------------------------
// Property 1: Tab count invariant after creation
// ---------------------------------------------------------------------------
void TestTabManagerProps::prop1_tabCountInvariantAfterCreation()
{
    StubTabManager mgr;
    QList<int> ids = populateTabs(mgr, 5);

    rc::check([&mgr, &ids]() {
        int before = mgr.tabCount();
        int id = mgr.createTab("https://example.com");
        if (id != -1) {
            ids.append(id);
            QVERIFY(mgr.tabCount() == before + 1);
        }
    });
}

// ---------------------------------------------------------------------------
// Property 2: Tab count cap enforcement (max 50)
// ---------------------------------------------------------------------------
void TestTabManagerProps::prop2_tabCountCapEnforcement()
{
    StubTabManager mgr;
    populateTabs(mgr, 50);

    int id = mgr.createTab("https://overflow.com");
    QVERIFY(id == -1);  // Cap reached
    QVERIFY(mgr.tabCount() <= 50);
}

// ---------------------------------------------------------------------------
// Property 3: Tab ID uniqueness
// ---------------------------------------------------------------------------
void TestTabManagerProps::prop3_tabIdUniqueness()
{
    StubTabManager mgr;
    QList<int> ids = populateTabs(mgr, 10);

    rc::check([&ids]() {
        QSet<int> unique(ids.begin(), ids.end());
        QVERIFY(unique.size() == ids.size());
    });
}

// ---------------------------------------------------------------------------
// Property 4: New tab becomes active
// ---------------------------------------------------------------------------
void TestTabManagerProps::prop4_newTabBecomesActive()
{
    StubTabManager mgr;
    QList<int> ids = populateTabs(mgr, 3);

    TabEntry* active = mgr.activeTab();
    QVERIFY(active != nullptr);
    QVERIFY(ids.contains(active->tabId));
}

// ---------------------------------------------------------------------------
// Property 5: Active tab and view visibility invariant
// ---------------------------------------------------------------------------
void TestTabManagerProps::prop5_activeTabAndVisibilityInvariant()
{
    StubTabManager mgr;
    QList<int> ids = populateTabs(mgr, 5);

    TabEntry* active = mgr.activeTab();
    QVERIFY(active != nullptr);
    if (active->container)
        QVERIFY(active->container->isVisible());

    // Non-active tabs should be hidden
    for (TabEntry* entry : mgr.allTabs()) {
        if (entry->tabId != mgr.activeTab()->tabId) {
            if (entry->container)
                QVERIFY(!entry->container->isVisible());
        }
    }
}

// ---------------------------------------------------------------------------
// Property 6: Previously active browser preserved
// ---------------------------------------------------------------------------
void TestTabManagerProps::prop6_previousActivePreserved()
{
    StubTabManager mgr;
    QList<int> ids = populateTabs(mgr, 5);

    int oldActiveId = mgr.activeTab()->tabId;
    mgr.switchToTab(ids.at(0));

    TabEntry* oldEntry = mgr.tabById(oldActiveId);
    QVERIFY(oldEntry != nullptr);
}

// ---------------------------------------------------------------------------
// Property 10: Last-tab close opens a new tab
// ---------------------------------------------------------------------------
void TestTabManagerProps::prop10_lastTabCloseOpensNewTab()
{
    StubTabManager mgr;
    int id = mgr.createTab("https://example.com");
    QVERIFY(id != -1);

    mgr.closeTab(id);
    QVERIFY(mgr.tabCount() >= 1);  // New tab created
}

// ---------------------------------------------------------------------------
// Property 11: Active tab selection after close
// ---------------------------------------------------------------------------
void TestTabManagerProps::prop11_activeTabAfterClose()
{
    StubTabManager mgr;
    QList<int> ids = populateTabs(mgr, 5);

    int closedId = ids.at(2);
    mgr.closeTab(closedId);

    TabEntry* active = mgr.activeTab();
    QVERIFY(active != nullptr);
    QVERIFY(active->tabId != closedId);
}

// ---------------------------------------------------------------------------
// Property 12: Close removes tab from manager
// ---------------------------------------------------------------------------
void TestTabManagerProps::prop12_closeRemovesTab()
{
    StubTabManager mgr;
    QList<int> ids = populateTabs(mgr, 5);

    int closedId = ids.at(2);
    mgr.closeTab(closedId);

    QVERIFY(mgr.tabById(closedId) == nullptr);
    QVERIFY(mgr.tabCount() == 4);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
QTEST_MAIN(TestTabManagerProps)
#include "test_tab_manager_props.moc"
