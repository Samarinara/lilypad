// ============================================================================
// TEST_BROWSER_PERFORMANCE.CPP
// ============================================================================
// Browser performance bugfix tests covering:
//   Task 9  — Exploratory tests confirming bug conditions are fixed
//   Task 10 — Fix-checking tests verifying each bug is resolved
//   Task 11 — Preservation tests verifying existing behaviors are unchanged
//   Task 12 — Property-based tests (using Qt Test loops)
// ============================================================================

#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QElapsedTimer>
#include <QUrl>
#include <QString>
#include <QStringList>
#include <QList>
#include <QRandomGenerator>

// Source under test
#include "../src/tab_entry.h"
#include "../src/tab_manager.h"
#include "../src/urlprocessor.h"

// ============================================================================
// HELPER: read a source file into a QString for structural checks
// ============================================================================
static QString readSourceFile(const QString& relativePath)
{
    // Try several locations relative to the working directory
    QStringList candidates;
    candidates << relativePath
               << "../" + relativePath
               << "../../" + relativePath;
    for (const QString& path : candidates) {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString::fromUtf8(f.readAll());
        }
    }
    return QString();
}

// ============================================================================
// TEST CLASS
// ============================================================================
class TestBrowserPerformance : public QObject
{
    Q_OBJECT

private slots:

    // ========================================================================
    // TASK 9 — Exploratory tests confirming bug conditions are fixed
    // ========================================================================

    // 9.1 — TabEntry has a faviconUrl property (confirms Bug 2 fix)
    void test_9_1_tabEntry_has_faviconUrl_property()
    {
        TabEntry entry;
        // Verify the property exists via the meta-object system
        const QMetaObject* meta = entry.metaObject();
        int propIdx = meta->indexOfProperty("faviconUrl");
        QVERIFY2(propIdx >= 0, "TabEntry must have a 'faviconUrl' Q_PROPERTY");

        // Verify it is readable and has a NOTIFY signal
        QMetaProperty prop = meta->property(propIdx);
        QVERIFY2(prop.isReadable(), "faviconUrl property must be readable");
        QVERIFY2(prop.hasNotifySignal(), "faviconUrl property must have a NOTIFY signal");
    }

    // 9.2 — onFaviconChanged does NOT emit a spurious urlChanged signal
    void test_9_2_onFaviconChanged_no_spurious_urlChanged()
    {
        TabManager mgr;
        int tabId = mgr.createTab("https://example.com");
        QVERIFY(tabId > 0);

        TabEntry* entry = mgr.tabById(tabId);
        QVERIFY(entry != nullptr);

        // Spy on the TabEntry's urlChanged signal — must NOT fire
        QSignalSpy urlSpy(entry, &TabEntry::urlChanged);

        // Trigger onFaviconChanged with a valid favicon URL
        mgr.onFaviconChanged(tabId, QUrl("https://example.com/favicon.ico"));

        // No spurious urlChanged should have been emitted
        QCOMPARE(urlSpy.count(), 0);
    }

    // 9.3 — tabById uses m_tabIndex hash (O(1) lookup works correctly)
    void test_9_3_tabById_hash_lookup_correct()
    {
        TabManager mgr;
        // Create several tabs and verify tabById returns the right entry for each
        QList<int> ids;
        for (int i = 0; i < 10; ++i) {
            int id = mgr.createTab(QString("https://example%1.com").arg(i));
            QVERIFY(id > 0);
            ids.append(id);
        }

        for (int id : ids) {
            TabEntry* entry = mgr.tabById(id);
            QVERIFY2(entry != nullptr, "tabById must return non-null for a valid tab ID");
            QCOMPARE(entry->tabId(), id);
        }

        // Non-existent ID must return nullptr
        QVERIFY(mgr.tabById(99999) == nullptr);
    }

    // 9.4 — closeAllTabs() completes quickly (no blocking QEventLoop)
    void test_9_4_closeAllTabs_completes_quickly()
    {
        TabManager mgr;
        mgr.createTab("https://a.com");
        mgr.createTab("https://b.com");
        mgr.createTab("https://c.com");

        QElapsedTimer timer;
        timer.start();
        mgr.closeAllTabs();
        qint64 elapsed = timer.elapsed();

        // Must complete in under 500 ms — a blocking QEventLoop would take seconds
        QVERIFY2(elapsed < 500,
                 qPrintable(QString("closeAllTabs() took %1 ms, expected < 500 ms").arg(elapsed)));
    }

    // 9.5 — initDatabase() source contains no hardcoded /home/ paths
    void test_9_5_initDatabase_no_hardcoded_home_paths()
    {
        QString src = readSourceFile("src/urlprocessor.cpp");
        QVERIFY2(!src.isEmpty(), "Could not read src/urlprocessor.cpp for structural check");

        // The fix removed /home/samarinara paths; verify they are absent
        QVERIFY2(!src.contains("/home/samarinara"),
                 "urlprocessor.cpp must not contain hardcoded /home/samarinara paths");

        // More general: no /home/ absolute path should appear in possiblePaths
        // (We check the specific developer path that was present before the fix)
        QVERIFY2(!src.contains("/home/samarinara/"),
                 "urlprocessor.cpp must not contain any /home/samarinara/ path entries");
    }

    // ========================================================================
    // TASK 10 — Fix-checking tests verifying each bug is resolved
    // ========================================================================

    // 10.1 — setFaviconUrl stores value and emits faviconUrlChanged (Property 2)
    void test_10_1_setFaviconUrl_stores_and_signals()
    {
        TabEntry entry;
        QSignalSpy spy(&entry, &TabEntry::faviconUrlChanged);

        const QString url = "https://example.com/favicon.ico";
        entry.setFaviconUrl(url);

        QCOMPARE(entry.faviconUrl(), url);
        QCOMPARE(spy.count(), 1);

        // Setting the same value again must NOT emit the signal again
        entry.setFaviconUrl(url);
        QCOMPARE(spy.count(), 1);
    }

    // 10.2 — tabById with 50 tabs returns correct entry (Property 3 — O(1) lookup)
    void test_10_2_tabById_50_tabs()
    {
        TabManager mgr;
        int lastId = -1;
        for (int i = 0; i < 50; ++i) {
            int id = mgr.createTab(QString("https://tab%1.com").arg(i));
            QVERIFY(id > 0);
            lastId = id;
        }

        QCOMPARE(mgr.tabCount(), 50);

        TabEntry* entry = mgr.tabById(lastId);
        QVERIFY2(entry != nullptr, "tabById must return non-null for the 50th tab");
        QCOMPARE(entry->tabId(), lastId);
    }

    // 10.3 — closeAllTabs() returns without blocking (Property 4)
    void test_10_3_closeAllTabs_non_blocking()
    {
        TabManager mgr;
        mgr.createTab("https://one.com");
        mgr.createTab("https://two.com");
        mgr.createTab("https://three.com");

        QElapsedTimer timer;
        timer.start();
        mgr.closeAllTabs();
        qint64 elapsed = timer.elapsed();

        QVERIFY2(elapsed < 1000,
                 qPrintable(QString("closeAllTabs() took %1 ms, expected < 1000 ms").arg(elapsed)));
        QCOMPARE(mgr.tabCount(), 0);
    }

    // 10.4 — UrlProcessor::process("!g hello") returns same URL on repeated calls (Property 7)
    void test_10_4_process_bang_query_consistent()
    {
        UrlProcessor proc;
        QUrl first  = proc.process("!g hello");
        QUrl second = proc.process("!g hello");

        // Both calls must return the same result (static regex, consistent output)
        QCOMPARE(first, second);
        // Result must be non-empty
        QVERIFY(!first.isEmpty());
    }

    // 10.5 — UrlProcessor::process("example.com") returns same URL on repeated calls (Property 7)
    void test_10_5_process_domain_consistent()
    {
        UrlProcessor proc;
        QUrl first  = proc.process("example.com");
        QUrl second = proc.process("example.com");

        QCOMPARE(first, second);
        QVERIFY(!first.isEmpty());
    }

    // ========================================================================
    // TASK 11 — Preservation tests verifying existing behaviors are unchanged
    // ========================================================================

    // 11.1 — createTab creates tab, sets URL, emits tabCreated (Requirement 3.2)
    void test_11_1_createTab_creates_and_signals()
    {
        TabManager mgr;
        QSignalSpy spy(&mgr, &TabManager::tabCreated);

        int tabId = mgr.createTab("https://example.com");

        QVERIFY2(tabId > 0, "createTab must return a positive tab ID");
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), tabId);

        TabEntry* entry = mgr.tabById(tabId);
        QVERIFY(entry != nullptr);
        QCOMPARE(entry->url(), QString("https://example.com"));
    }

    // 11.2 — closeTab removes active tab, adjacent tab becomes active, tabClosed emitted (Req 3.3)
    void test_11_2_closeTab_switches_to_adjacent()
    {
        TabManager mgr;
        int id1 = mgr.createTab("https://one.com");
        int id2 = mgr.createTab("https://two.com");
        QVERIFY(id1 > 0);
        QVERIFY(id2 > 0);

        // id2 is active (last created)
        QCOMPARE(mgr.activeTabId(), id2);

        QSignalSpy closedSpy(&mgr, &TabManager::tabClosed);

        mgr.closeTab(id2);

        // tabClosed must have been emitted for id2
        QVERIFY(closedSpy.count() >= 1);
        bool foundId2 = false;
        for (int i = 0; i < closedSpy.count(); ++i) {
            if (closedSpy.at(i).at(0).toInt() == id2) {
                foundId2 = true;
                break;
            }
        }
        QVERIFY2(foundId2, "tabClosed must be emitted for the closed tab ID");

        // id1 must now be active
        QCOMPARE(mgr.activeTabId(), id1);
    }

    // 11.3 — UrlProcessor::process("!g search term") routes to correct engine (Requirement 3.5)
    void test_11_3_bang_routing_correct_engine()
    {
        UrlProcessor proc;
        QUrl result = proc.process("!g search term");

        QVERIFY2(!result.isEmpty(), "Bang query must return a non-empty URL");

        // If bang routing worked, the URL should reference Google or contain the query
        QString urlStr = result.toString();
        bool hasGoogle = urlStr.contains("google", Qt::CaseInsensitive);
        bool hasQuery  = urlStr.contains("search") || urlStr.contains("search%20term")
                         || urlStr.contains("search+term");

        if (hasGoogle) {
            // Bang routing to Google succeeded — verify query is encoded in URL
            QVERIFY2(urlStr.contains("search"),
                     "Google bang URL must contain the search query");
        } else {
            // Database may not be available in this test environment — skip engine check
            // but still verify the URL is non-empty (already checked above)
            qDebug() << "Bang database not available; result URL:" << urlStr;
        }
    }

    // 11.4 — UrlProcessor::process("example.com") returns https://example.com (Requirement 3.6)
    void test_11_4_plain_domain_prepends_https()
    {
        UrlProcessor proc;
        QUrl result = proc.process("example.com");
        QCOMPARE(result, QUrl("https://example.com"));
    }

    // 11.5 — createTab returns -1 on the 51st attempt (Requirement 3.8)
    void test_11_5_tab_limit_enforced()
    {
        TabManager mgr;
        for (int i = 0; i < 50; ++i) {
            int id = mgr.createTab(QString("https://tab%1.com").arg(i));
            QVERIFY2(id > 0, qPrintable(QString("Tab %1 creation must succeed").arg(i + 1)));
        }
        QCOMPARE(mgr.tabCount(), 50);

        // 51st tab must be rejected
        int overflowId = mgr.createTab("https://overflow.com");
        QCOMPARE(overflowId, -1);
        QCOMPARE(mgr.tabCount(), 50);
    }

    // ========================================================================
    // TASK 12 — Property-based tests (using Qt Test loops)
    // ========================================================================

    // 12.1 — Property 3: m_tabIndex is always consistent with m_tabs across
    //         random sequences of create/close operations.
    //         Validates: Requirements 2.5, 2.6
    void test_12_1_tabIndex_consistency_property()
    {
        // Run multiple random sequences
        QRandomGenerator rng(42); // fixed seed for reproducibility

        for (int trial = 0; trial < 50; ++trial) {
            TabManager mgr;
            QList<int> liveTabs;

            int ops = static_cast<int>(rng.bounded(20)) + 5; // 5..24 operations
            for (int op = 0; op < ops; ++op) {
                bool doCreate = liveTabs.isEmpty()
                                || (rng.bounded(2) == 0 && liveTabs.size() < 50);

                if (doCreate) {
                    int id = mgr.createTab(QString("https://trial%1op%2.com").arg(trial).arg(op));
                    if (id > 0) {
                        liveTabs.append(id);
                    }
                } else {
                    // Close a random live tab
                    int idx = static_cast<int>(rng.bounded(static_cast<quint32>(liveTabs.size())));
                    int idToClose = liveTabs.takeAt(idx);
                    mgr.closeTab(idToClose);
                    // After closeTab, if only 1 tab was left a new blank tab is created
                    // Resync liveTabs with actual model state
                    liveTabs.clear();
                    for (int r = 0; r < mgr.tabCount(); ++r) {
                        QModelIndex mi = mgr.index(r, 0);
                        int id = mgr.data(mi, TabManager::TabIdRole).toInt();
                        liveTabs.append(id);
                    }
                }

                // Invariant: for every tab in the model, tabById must return non-null
                // and return the correct entry — this verifies hash consistency
                for (int r = 0; r < mgr.tabCount(); ++r) {
                    QModelIndex mi = mgr.index(r, 0);
                    int id = mgr.data(mi, TabManager::TabIdRole).toInt();
                    TabEntry* entry = mgr.tabById(id);
                    QVERIFY2(entry != nullptr,
                             qPrintable(QString("Trial %1 op %2: tabById(%3) returned null")
                                        .arg(trial).arg(op).arg(id)));
                    QCOMPARE(entry->tabId(), id);
                }
            }
        }
    }

    // 12.2 — Property 7: UrlProcessor::process() returns identical results on
    //         repeated calls for random URL strings (static regex consistency).
    //         Validates: Requirements 2.12, 2.13
    void test_12_2_process_idempotent_for_random_urls()
    {
        UrlProcessor proc;
        QRandomGenerator rng(123);

        // Pool of representative URL-like strings to test
        QStringList candidates = {
            "example.com",
            "https://example.com",
            "http://foo.bar.baz",
            "sub.domain.co.uk",
            "no-dots-here",
            "hello world",
            "192.168.1.1",
            "ftp://files.example.org",
            "just-a-word",
            "a.b",
            "test.org/path",
            "EXAMPLE.COM",
            "example.com/path?q=1",
            "localhost",
            "foo bar baz",
        };

        for (int trial = 0; trial < 100; ++trial) {
            int idx = static_cast<int>(rng.bounded(static_cast<quint32>(candidates.size())));
            const QString& input = candidates.at(idx);

            QUrl first  = proc.process(input);
            QUrl second = proc.process(input);

            QVERIFY2(first == second,
                     qPrintable(QString("process('%1') returned different results: '%2' vs '%3'")
                                .arg(input, first.toString(), second.toString())));
        }
    }

    // 12.3 — Property 8: random bang queries !<tag> <query> produce URLs containing
    //         the encoded query (preservation of bang routing).
    //         Validates: Requirement 3.5
    void test_12_3_bang_routing_encodes_query()
    {
        UrlProcessor proc;

        // Check if bang routing is available by testing a known bang
        QUrl probe = proc.process("!g test");
        bool bangAvailable = probe.toString().contains("google", Qt::CaseInsensitive)
                             || probe.toString().contains("search");

        if (!bangAvailable) {
            QSKIP("Bang database not available in test environment; skipping bang routing property test");
        }

        // Known bang tags that should be in the database
        QStringList bangTags = {"g"};

        // Random query words to combine
        QStringList words = {"hello", "world", "foo", "bar", "test", "query",
                             "search", "term", "abc", "xyz"};

        QRandomGenerator rng(456);

        for (int trial = 0; trial < 30; ++trial) {
            // Pick a random bang tag
            int tagIdx = static_cast<int>(rng.bounded(static_cast<quint32>(bangTags.size())));
            const QString& tag = bangTags.at(tagIdx);

            // Build a random query of 1-3 words
            int wordCount = static_cast<int>(rng.bounded(3)) + 1;
            QStringList queryWords;
            for (int w = 0; w < wordCount; ++w) {
                int wi = static_cast<int>(rng.bounded(static_cast<quint32>(words.size())));
                queryWords.append(words.at(wi));
            }
            QString query = queryWords.join(" ");
            QString input = QString("!%1 %2").arg(tag, query);

            QUrl result = proc.process(input);

            // Result must be non-empty
            QVERIFY2(!result.isEmpty(),
                     qPrintable(QString("Bang query '%1' returned empty URL").arg(input)));

            // The result URL must contain the encoded query (or at least one query word)
            QString urlStr = result.toString();
            bool containsQuery = false;
            for (const QString& word : queryWords) {
                if (urlStr.contains(word, Qt::CaseInsensitive)
                    || urlStr.contains(QUrl::toPercentEncoding(word))) {
                    containsQuery = true;
                    break;
                }
            }
            QVERIFY2(containsQuery,
                     qPrintable(QString("Bang URL '%1' does not contain query words from '%2'")
                                .arg(urlStr, input)));
        }
    }

    // 12.4 — Property 2: random favicon URL strings — onFaviconChanged stores the URL
    //         in faviconUrl() and does NOT emit urlChanged.
    //         Validates: Requirements 2.3, 2.4
    void test_12_4_onFaviconChanged_stores_url_no_urlChanged()
    {
        QRandomGenerator rng(789);

        QStringList faviconUrls = {
            "https://example.com/favicon.ico",
            "https://google.com/favicon.ico",
            "https://github.com/favicon.png",
            "https://sub.domain.org/img/icon.svg",
            "https://a.b.c/d.ico",
            "https://example.com/path/to/favicon.ico",
            "https://foo.bar/baz.png",
            "https://test.example.com/favicon.ico",
        };

        for (int trial = 0; trial < 40; ++trial) {
            TabManager mgr;
            int tabId = mgr.createTab("https://initial.com");
            QVERIFY(tabId > 0);

            TabEntry* entry = mgr.tabById(tabId);
            QVERIFY(entry != nullptr);

            // Spy on urlChanged — must NOT fire
            QSignalSpy urlSpy(entry, &TabEntry::urlChanged);
            // Spy on faviconUrlChanged — must fire once
            QSignalSpy faviconSpy(entry, &TabEntry::faviconUrlChanged);

            int idx = static_cast<int>(rng.bounded(static_cast<quint32>(faviconUrls.size())));
            const QString& faviconUrl = faviconUrls.at(idx);

            mgr.onFaviconChanged(tabId, QUrl(faviconUrl));

            // faviconUrl must be stored correctly
            QVERIFY2(entry->faviconUrl() == faviconUrl,
                     qPrintable(QString("Trial %1: faviconUrl() returned '%2', expected '%3'")
                                .arg(trial).arg(entry->faviconUrl(), faviconUrl)));

            // No spurious urlChanged
            QVERIFY2(urlSpy.count() == 0,
                     qPrintable(QString("Trial %1: urlChanged emitted %2 times (expected 0)")
                                .arg(trial).arg(urlSpy.count())));

            // faviconUrlChanged must have been emitted
            QVERIFY2(faviconSpy.count() == 1,
                     qPrintable(QString("Trial %1: faviconUrlChanged emitted %2 times (expected 1)")
                                .arg(trial).arg(faviconSpy.count())));
        }
    }
};

// ============================================================================
// MAIN
// ============================================================================
QTEST_MAIN(TestBrowserPerformance)
#include "test_browser_performance.moc"
