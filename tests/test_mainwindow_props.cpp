// Property-based tests for MainWindow-level properties using rapidcheck + Qt Test.
//
// Properties tested:
//   Property  7 — URL bar synchronization                    (Req 4.3, 6.1, 6.3, 6.4)
//   Property  9 — Tab panel fixed width invariant            (Req 1.1, 1.4)
//   Property 15 — Rendering suspension for inactive tabs     (Req 10.1, 10.2, 10.3)
//
// Strategy:
//   Property 7  — Wire a QLineEdit + StubTabManager manually, mirroring what
//                 MainWindow does, and verify URL bar text after switch/navigation.
//   Property 9  — Instantiate a real MainWindow (no tabs created, so no browser calls),
//                 resize it, and verify TabPanel width via findChild<TabPanel*>().
//   Property 15 — With view creation, verify container visibility after each switchToTab.

#include <QtTest/QtTest>
#include <QApplication>
#include <QLineEdit>
#include <rapidcheck.h>

#include "tab_manager.h"
#include "tab_entry.h"
#include "tab_panel.h"
#include "mainwindow.h"

// ---------------------------------------------------------------------------
// StubTabManager — bypasses browser creation
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
        int id = mgr.createTab(QString("https://example%1.com").arg(i));
        if (id != -1)
            ids.append(id);
    }
    return ids;
}

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------

class TestMainWindowProps : public QObject {
    Q_OBJECT

private slots:
    // Property 7: URL bar synchronization
    // Validates: Requirements 4.3, 6.1, 6.3, 6.4
    void prop7_urlBarSynchronization();

    // Property 9: Tab panel fixed width invariant
    // Validates: Requirements 1.1, 1.4
    void prop9_tabPanelFixedWidthInvariant();

    // Property 15: Rendering suspension for inactive tabs
    // Validates: Requirements 10.1, 10.2, 10.3
    void prop15_renderingSuspensionForInactiveTabs();
};

// ---------------------------------------------------------------------------
// Property 7: URL bar synchronization
// ---------------------------------------------------------------------------
void TestMainWindowProps::prop7_urlBarSynchronization()
{
    QVERIFY(rc::check(
        "Property 7: URL bar text equals active tab URL",
        []() {
            const int numTabs = *rc::gen::inRange(1, 11);

            StubTabManager mgr;
            QLineEdit urlBar;

            // Wire up the same connections MainWindow uses
            QObject::connect(&mgr, &TabManager::activeTabChanged,
                [&](int /*oldId*/, int newId) {
                    TabEntry* entry = mgr.tabById(newId);
                    if (entry)
                        urlBar.setText(entry->url);
                });

            QObject::connect(&mgr, &TabManager::urlChanged,
                [&](int tabId, const QString& url) {
                    TabEntry* active = mgr.activeTab();
                    if (active && active->tabId == tabId)
                        urlBar.setText(url);
                });

            QList<int> ids = populateTabs(mgr, numTabs);
            RC_ASSERT(ids.size() == numTabs);

            // --- Sub-test A: after switchToTab, URL bar == entry->url ---
            const int switchIdx = *rc::gen::inRange(0, (int)ids.size() - 1);
            const int targetId = ids.at(switchIdx);

            mgr.switchToTab(targetId);

            TabEntry* active = mgr.activeTab();
            RC_ASSERT(active != nullptr);
            RC_ASSERT(active->tabId == targetId);
            RC_ASSERT(urlBar.text() == active->url);

            // --- Sub-test B: urlChanged for active tab updates URL bar ---
            QString newUrl = QString("https://newsite%1.com").arg(*rc::gen::inRange(1, 1000));
            mgr.onUrlChanged(targetId, newUrl);
            RC_ASSERT(urlBar.text() == newUrl);

            // --- Sub-test C: urlChanged for inactive tab does NOT update URL bar ---
            if (numTabs > 1) {
                // Find an inactive tab
                int inactiveId = -1;
                for (int id : ids) {
                    if (id != targetId) {
                        inactiveId = id;
                        break;
                    }
                }
                RC_ASSERT(inactiveId != -1);

                const QString urlBeforeInactiveNav = urlBar.text();
                QString otherUrl = QString("https://other%1.com").arg(*rc::gen::inRange(1, 1000));
                mgr.onUrlChanged(inactiveId, otherUrl);

                // URL bar must not have changed
                RC_ASSERT(urlBar.text() == urlBeforeInactiveNav);
            }
        }));
}

// ---------------------------------------------------------------------------
// Property 9: Tab panel fixed width invariant
// ---------------------------------------------------------------------------
void TestMainWindowProps::prop9_tabPanelFixedWidthInvariant()
{
    QVERIFY(rc::check(
        "Property 9: TabPanel width is constant across window resize",
        []() {
            const int windowWidth = *rc::gen::inRange(400, 2001);

            MainWindow w;
            w.show();
            w.resize(windowWidth, 600);
            QApplication::processEvents();

            // Find the TabPanel child
            TabPanel* panel = w.findChild<TabPanel*>();
            RC_ASSERT(panel != nullptr);

            // TabPanel must have fixed width == kPanelWidth
            RC_ASSERT(panel->width() == TabPanel::kPanelWidth);

            // The OSR container (content area) should fill remaining width
            QWidget* central = w.centralWidget();
            RC_ASSERT(central != nullptr);

            // Find the content area (not TabPanel, not QLineEdit)
            QWidget* contentContainer = nullptr;
            const auto children = central->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
            for (QWidget* child : children) {
                if (child != panel && qobject_cast<QLineEdit*>(child) == nullptr) {
                    contentContainer = child;
                    break;
                }
            }
            RC_ASSERT(contentContainer != nullptr);

            // Content container width should be window width minus panel width
            const int expectedWidth = central->width() - TabPanel::kPanelWidth;
            RC_ASSERT(contentContainer->width() == expectedWidth);
        }));
}

// ---------------------------------------------------------------------------
// Property 15: Rendering suspension for inactive tabs
// ---------------------------------------------------------------------------
void TestMainWindowProps::prop15_renderingSuspensionForInactiveTabs()
{
    QVERIFY(rc::check(
        "Property 15: inactive tabs have container==hidden, active tab has container==visible",
        []() {
            const int numTabs   = *rc::gen::inRange(2, 11);
            const int numSwitches = *rc::gen::inRange(1, 11);

            StubTabManager mgr;
            QList<int> ids = populateTabs(mgr, numTabs);
            RC_ASSERT(ids.size() == numTabs);

            for (int s = 0; s < numSwitches; ++s) {
                const int switchIdx = *rc::gen::inRange(0, (int)ids.size() - 1);
                const int targetId  = ids.at(switchIdx);

                mgr.switchToTab(targetId);

                TabEntry* active = mgr.activeTab();
                RC_ASSERT(active != nullptr);
                RC_ASSERT(active->tabId == targetId);

                // Verify container visibility
                if (active->container)
                    RC_ASSERT(active->container->isVisible());

                // Non-active tabs should be hidden
                for (TabEntry* entry : mgr.allTabs()) {
                    if (entry->tabId != mgr.activeTab()->tabId) {
                        if (entry->container)
                            RC_ASSERT(!entry->container->isVisible());
                    }
                }
            }
        }));
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
QTEST_MAIN(TestMainWindowProps)
#include "test_mainwindow_props.moc"
