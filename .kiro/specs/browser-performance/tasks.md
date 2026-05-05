# Tasks

## Task List

- [x] 1. Add `faviconUrl` string property to `TabEntry`
  - [x] 1.1 Add `Q_PROPERTY(QString faviconUrl READ faviconUrl NOTIFY faviconUrlChanged)` to `src/tab_entry.h`
  - [x] 1.2 Add `QString m_faviconUrl` member variable with empty-string default to `src/tab_entry.h`
  - [x] 1.3 Add `faviconUrl()` getter and `setFaviconUrl(const QString&)` setter with change guard to `src/tab_entry.h`
  - [x] 1.4 Add `faviconUrlChanged()` signal to `src/tab_entry.h`

- [x] 2. Fix `onFaviconChanged` in `TabManager` to store favicon URL correctly
  - [x] 2.1 Replace `entry->setUrl(entry->url())` with `entry->setFaviconUrl(faviconUrl.toString())` in `src/tab_manager.cpp`
  - [x] 2.2 Add `FaviconUrlRole` to the `Role` enum in `src/tab_manager.h`
  - [x] 2.3 Add `roles[FaviconUrlRole] = "faviconUrl"` to `roleNames()` in `src/tab_manager.cpp`
  - [x] 2.4 Add `case FaviconUrlRole: return entry->faviconUrl()` to `data()` in `src/tab_manager.cpp`

- [x] 3. Add `QHash<int, TabEntry*>` index to `TabManager` for O(1) tab lookup
  - [x] 3.1 Add `QHash<int, TabEntry*> m_tabIndex` member to `src/tab_manager.h`
  - [x] 3.2 Insert `m_tabIndex[tabId] = entry` in `createTab()` after the entry is created in `src/tab_manager.cpp`
  - [x] 3.3 Call `m_tabIndex.remove(tabId)` in `removeTabEntry()` before deleting the entry in `src/tab_manager.cpp`
  - [x] 3.4 Replace the loop body of `tabById()` with `return m_tabIndex.value(tabId, nullptr)` in `src/tab_manager.cpp`

- [x] 4. Remove blocking `QEventLoop` from `closeAllTabs()`
  - [x] 4.1 Remove the `QEventLoop loop`, `QTimer safetyTimer`, and all associated `connect`/`disconnect` and `loop.exec()` calls from `closeAllTabs()` in `src/tab_manager.cpp`
  - [x] 4.2 Replace with a simple iteration: call `removeTabEntry(id)` for each tab ID collected before the loop
  - [x] 4.3 Remove `#include <QEventLoop>` and `#include <QTimer>` from `src/tab_manager.cpp` if no longer used

- [x] 5. Remove hardcoded absolute paths from `initDatabase()` in `UrlProcessor`
  - [x] 5.1 Remove the two `/home/samarinara/...` entries from `possiblePaths` in `src/urlprocessor.cpp`
  - [x] 5.2 Verify the remaining portable paths (`applicationDirPath() + "/bangs.db"`, `"./bangs.db"`, `"../bangs.db"`, `"../../bangs.db"`) are sufficient for normal builds

- [x] 6. Promote `domainRegex` and `bangRegex` to `static const` in `UrlProcessor`
  - [x] 6.1 Change `QRegularExpression domainRegex(...)` to `static const QRegularExpression domainRegex(...)` in `isUrl()` in `src/urlprocessor.cpp`
  - [x] 6.2 Change `QRegularExpression bangRegex(...)` to `static const QRegularExpression bangRegex(...)` in `handleBangRouting()` in `src/urlprocessor.cpp`

- [x] 7. Remove unconditional `qDebug` calls from hot-path handlers
  - [x] 7.1 Remove `qDebug() << "onUrlChanged called for tab"...` from `onUrlChanged()` in `src/tab_manager.cpp`
  - [x] 7.2 Remove `qDebug() << "Loading URL for active tab"...` and `qDebug() << "Active tab ID"...` from `loadUrlForActiveTab()` in `src/tab_manager.cpp`
  - [x] 7.3 Remove `qDebug() << "Favicon changed for tab"...` from `onFaviconChanged()` in `src/tab_manager.cpp`
  - [x] 7.4 Remove `console.log` calls from `onTitleChanged`, `onIconChanged`, `onLoadingChanged`, and `onUrlChanged` handlers inside the `WebEngineView` delegate in `qml/Main.qml`
  - [x] 7.5 Retain the low-frequency `qDebug() << "Creating new tab"...` and `qDebug() << "Tab created with ID"...` logs in `createTab()` as they fire at most once per tab creation

- [x] 8. Implement lazy `WebEngineView` instantiation using `Loader` in `Main.qml`
  - [x] 8.1 Replace the direct `WebEngineView` inside the `Repeater` delegate with a `Loader` component
  - [x] 8.2 Set `loader.active: model.tabId === tabManager.activeTabId` so only the active tab's `WebEngineView` is instantiated
  - [x] 8.3 Move all `WebEngineView` signal connections (`onTitleChanged`, `onIconChanged`, `onLoadingChanged`, `onUrlChanged`) inside the `Loader`'s `sourceComponent`
  - [x] 8.4 Update the URL bar sync logic so switching to a tab that already has a loaded `WebEngineView` still updates `urlBar.text` correctly

- [x] 9. Write exploratory tests to confirm bug conditions on unfixed code
  - [x] 9.1 Write a C++ unit test asserting `TabEntry` has a `faviconUrl` property (will fail before Task 1 is complete — confirms Bug 2)
  - [x] 9.2 Write a C++ unit test calling `onFaviconChanged` and asserting no spurious `urlChanged` signal is emitted (confirms Bug 2 root cause)
  - [x] 9.3 Write a C++ unit test asserting `tabById` uses `m_tabIndex` hash after Task 3 (confirms Bug 3 fix)
  - [x] 9.4 Write a C++ unit test asserting `closeAllTabs()` contains no `QEventLoop` (structural check — confirms Bug 4 fix)
  - [x] 9.5 Write a C++ unit test asserting `initDatabase()` path list contains no `/home/` entries (confirms Bug 5 fix)

- [x] 10. Write fix-checking tests (verify each bug condition is resolved)
  - [x] 10.1 Write a C++ unit test: call `setFaviconUrl("https://example.com/favicon.ico")` and assert `faviconUrl()` returns the stored value and `faviconUrlChanged` is emitted (Property 2)
  - [x] 10.2 Write a C++ unit test: create 50 tabs, call `tabById(lastTabId)`, and assert the result is non-null and correct (Property 3 — O(1) lookup)
  - [x] 10.3 Write a C++ unit test: create 3 tabs, call `closeAllTabs()`, and assert it returns without blocking (Property 4)
  - [x] 10.4 Write a C++ unit test: call `UrlProcessor::process("!g hello")` twice and assert both calls return the same URL (Property 7 — static regex)
  - [x] 10.5 Write a C++ unit test: call `UrlProcessor::process("example.com")` twice and assert both calls return the same URL (Property 7 — static domain regex)

- [x] 11. Write preservation tests (verify existing behaviors are unchanged)
  - [x] 11.1 Write a C++ unit test: call `createTab("https://example.com")` and assert the tab is created, URL is set, and `tabCreated` signal is emitted (Requirement 3.2)
  - [x] 11.2 Write a C++ unit test: create 2 tabs, close the active one, and assert the adjacent tab becomes active and `tabClosed` is emitted (Requirement 3.3)
  - [x] 11.3 Write a C++ unit test: call `UrlProcessor::process("!g search term")` and assert the result URL contains the encoded search term routed to the correct engine (Requirement 3.5)
  - [x] 11.4 Write a C++ unit test: call `UrlProcessor::process("example.com")` and assert the result is `QUrl("https://example.com")` (Requirement 3.6)
  - [x] 11.5 Write a C++ unit test: create 50 tabs and assert `createTab()` returns -1 on the 51st attempt (Requirement 3.8)

- [x] 12. Write property-based tests
  - [x] 12.1 Write a property-based test for Property 3: generate random sequences of tab create/close operations and assert `m_tabIndex` is always consistent with `m_tabs` (every tab in `m_tabs` has a matching entry in `m_tabIndex` and vice versa)
  - [x] 12.2 Write a property-based test for Property 7: generate random URL strings and assert `UrlProcessor::process()` returns identical results on repeated calls (verifying static regex produces consistent output)
  - [x] 12.3 Write a property-based test for Property 8: generate random bang queries of the form `!<tag> <query>` and assert the result URL contains the encoded query (preservation of bang routing across all valid inputs)
  - [x] 12.4 Write a property-based test for Property 2: generate random favicon URL strings, call `onFaviconChanged`, and assert `faviconUrl()` equals the input and no `urlChanged` signal is emitted
