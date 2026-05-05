# Browser Performance Bugfix Design

## Overview

The Lilypad browser has seven distinct performance defects spanning three layers: the QML tab rendering layer (`Main.qml`), the C++ tab management layer (`tab_manager.cpp` / `tab_entry.h`), and the URL processing layer (`urlprocessor.cpp`). The defects cause excessive memory consumption (all Chromium renderer processes alive simultaneously), unnecessary CPU work (O(n) lookups on every event, regex recompilation on every URL keystroke, unconditional debug logging), UI freezes on shutdown (blocking event loop in `closeAllTabs`), and portability failures (hardcoded developer-machine paths for `bangs.db`).

The fix strategy is targeted and minimal: each defect is addressed in isolation with no architectural rewrites. The QML `Repeater`-based approach is replaced with a `Loader`-based lazy instantiation pattern; `TabManager` gains a `QHash<int, TabEntry*>` index and a `faviconUrl` string property on `TabEntry`; `closeAllTabs` drops its nested `QEventLoop`; `UrlProcessor` promotes two local `QRegularExpression` variables to `static const`; hardcoded paths are removed; and hot-path `qDebug` calls are removed or gated.

---

## Glossary

- **Bug_Condition (C)**: The set of conditions that trigger one or more of the seven performance defects — evaluated per-issue as `isBugCondition_N(input)`.
- **Property (P)**: The desired correct behavior when the bug condition holds — the fixed code must satisfy `P(result)` for all inputs in `C`.
- **Preservation**: All existing user-visible behaviors (tab switching, navigation, bang routing, dark mode, etc.) that must remain identical after the fix.
- **TabManager**: The C++ class in `src/tab_manager.cpp` that owns all `TabEntry` objects and acts as a `QAbstractListModel` for QML.
- **TabEntry**: The C++ class in `src/tab_entry.h` / `src/tab_entry.cpp` that holds per-tab state (title, URL, favicon, loading state).
- **WebEngineView**: The QML component (from `QtWebEngine`) that embeds a full Chromium renderer process per instance.
- **Loader**: A QML component that defers creation of its child until `active: true`, enabling lazy instantiation.
- **onFaviconChanged**: The `TabManager` slot called when a `WebEngineView` emits `onIconChanged`; currently broken — calls `setUrl` instead of storing the favicon URL.
- **closeAllTabs**: The `TabManager` method called on application shutdown; currently blocks the UI thread via a nested `QEventLoop`.
- **bangTable / suffixTable**: `QHash<QString, QString>` maps in `UrlProcessor` loaded from `bangs.db` at startup.
- **domainRegex / bangRegex**: `QRegularExpression` objects in `UrlProcessor` currently re-constructed on every call to `isUrl()` and `handleBangRouting()`.

---

## Bug Details

### Bug Condition

Seven independent bug conditions exist. Each is specified formally below.

---

**Bug 1 — All WebEngineViews always alive**

The bug manifests when more than one tab is open. The `Repeater` in `Main.qml` instantiates a full `WebEngineView` (and its Chromium renderer process) for every tab in the model simultaneously. Background tabs are never suspended or lazily loaded.

**Formal Specification:**
```
FUNCTION isBugCondition_1(state)
  INPUT: state.tabCount — number of open tabs
  OUTPUT: boolean

  RETURN state.tabCount > 1
         AND allWebEngineViewsInstantiated(state)
END FUNCTION
```

**Examples:**
- 3 tabs open → 3 Chromium renderer processes running simultaneously (expected: 1 active + 2 deferred)
- Switching to tab 2 → tab 1's renderer stays fully alive (expected: tab 1 suspended or unloaded)
- Opening 10 tabs → 10 renderer processes (expected: only the active tab's renderer is live)

---

**Bug 2 — Redundant `dataChanged` emission / favicon not stored**

The bug manifests whenever a `WebEngineView` emits `onIconChanged`. `TabManager::onFaviconChanged` calls `entry->setUrl(entry->url())` — a no-op that re-emits `urlChanged` spuriously — instead of storing the favicon URL.

**Formal Specification:**
```
FUNCTION isBugCondition_2(event)
  INPUT: event — a favicon-changed event with a non-empty faviconUrl
  OUTPUT: boolean

  RETURN event.type == FAVICON_CHANGED
         AND faviconUrlStoredInTabEntry(event.tabId) == ""
         AND spuriousUrlSignalEmitted(event.tabId) == true
END FUNCTION
```

**Examples:**
- Page loads with favicon → `TabEntry.favicon` remains empty QPixmap, no favicon shown in tab
- Favicon event fires → `urlChanged` signal emitted spuriously, QML re-evaluates URL binding unnecessarily
- After fix: favicon URL stored in `TabEntry.faviconUrl`, `faviconChanged` emitted once, QML loads image

---

**Bug 3 — O(n) tab lookup on every hot-path event**

The bug manifests on every tab event (title change, URL change, loading state change, favicon change). `tabById()` and `indexOfTab()` both perform a full linear scan of `m_tabs`.

**Formal Specification:**
```
FUNCTION isBugCondition_3(event)
  INPUT: event — any tab lifecycle event (title/url/loading/favicon change)
  OUTPUT: boolean

  RETURN lookupComplexity(tabById, event.tabId) == O(n)
         AND lookupComplexity(indexOfTab, event.tabId) == O(n)
END FUNCTION
```

**Examples:**
- 50 tabs open, URL changes on tab 50 → 50 iterations to find the entry (expected: 1 hash lookup)
- Every keystroke during page load triggers O(n) scans across all event handlers

---

**Bug 4 — `closeAllTabs` blocks the UI thread**

The bug manifests when the application closes. `closeAllTabs()` spins a nested `QEventLoop` waiting for `WebEngineView` close signals, blocking the UI thread for up to 5 seconds.

**Formal Specification:**
```
FUNCTION isBugCondition_4(action)
  INPUT: action — application shutdown triggering closeAllTabs()
  OUTPUT: boolean

  RETURN nestedEventLoopSpun(action) == true
         AND uiThreadBlocked(action) == true
END FUNCTION
```

**Examples:**
- User closes app → UI freezes for up to 5 seconds before process exits
- WebEngineViews don't emit close signals promptly → full 5-second timeout always hit

---

**Bug 5 — Hardcoded absolute paths in bang database resolution**

The bug manifests on any machine other than the developer's. `initDatabase()` includes `/home/samarinara/...` paths that don't exist elsewhere.

**Formal Specification:**
```
FUNCTION isBugCondition_5(environment)
  INPUT: environment.machine — the machine running the application
  OUTPUT: boolean

  RETURN environment.machine != "samarinara-dev-machine"
         AND hardcodedPathsInPossiblePaths(initDatabase) == true
END FUNCTION
```

**Examples:**
- Running on any CI machine → `bangs.db` not found, bang routing silently disabled
- Running on another developer's machine → same failure
- After fix: only `applicationDirPath() + "/bangs.db"` and `"./bangs.db"` in the path list

---

**Bug 6 — Unconditional `qDebug` logging in hot paths**

The bug manifests on every page navigation. `onUrlChanged`, `onTitleChanged`, `onLoadingStateChanged`, and the QML `WebEngineView` signal handlers all emit unconditional `qDebug` messages.

**Formal Specification:**
```
FUNCTION isBugCondition_6(event)
  INPUT: event — any navigation event (URL change, title change, loading state change)
  OUTPUT: boolean

  RETURN unconditionalQDebugCallPresent(event.handler) == true
END FUNCTION
```

**Examples:**
- Page load → dozens of `qDebug` messages emitted per navigation
- After fix: debug messages removed or wrapped in `#ifdef QT_DEBUG` / runtime flag

---

**Bug 7 — `QRegularExpression` recompiled on every `process()` call**

The bug manifests on every URL bar submission. `isUrl()` and `handleBangRouting()` each construct a new `QRegularExpression` as a local variable, recompiling the pattern from scratch.

**Formal Specification:**
```
FUNCTION isBugCondition_7(call)
  INPUT: call — an invocation of UrlProcessor::process()
  OUTPUT: boolean

  RETURN localQRegularExpressionConstructed(isUrl) == true
         OR localQRegularExpressionConstructed(handleBangRouting) == true
END FUNCTION
```

**Examples:**
- User types 10 characters in URL bar → `domainRegex` compiled 10 times
- After fix: `static const QRegularExpression` compiled once at first call

---

## Expected Behavior

### Preservation Requirements

**Unchanged Behaviors:**
- Tab switching must continue to display the correct web content and update the URL bar (Requirement 3.1)
- Opening a new tab must continue to create a tab entry, navigate to the initial URL, and activate the tab (Requirement 3.2)
- Closing a tab must continue to remove it from the list, switch to an adjacent tab if it was active, and free resources (Requirement 3.3)
- Page load completion must continue to update the tab's title, URL, and loading indicator (Requirement 3.4)
- Bang query routing (e.g., `!g search term`) must continue to resolve to the correct search engine URL (Requirement 3.5)
- Plain URL and domain navigation must continue to work, prepending `https://` when no scheme is present (Requirement 3.6)
- Search query fallback routing must continue to use the default search engine (Requirement 3.7)
- The 50-tab maximum must continue to be enforced (Requirement 3.8)
- Dark mode toggling must continue to apply correct theme colors across all UI components (Requirement 3.9)
- Application startup must continue to open with a single tab navigated to the default home page (Requirement 3.10)

**Scope:**
All inputs that do NOT trigger one of the seven bug conditions should be completely unaffected by this fix. This includes:
- Mouse clicks on tabs, close buttons, and the new-tab button
- URL bar navigation (plain URLs, bang queries, search queries)
- Dark mode toggle
- All QML theme and layout behavior

---

## Hypothesized Root Cause

**Bug 1 — Eager WebEngineView instantiation:**
The `Repeater` in `Main.qml` creates one delegate per model item unconditionally. `WebEngineView` is placed directly inside the delegate `Item`, so it is instantiated immediately when the model row is added. There is no `Loader` or `active` guard to defer creation.

**Bug 2 — Broken favicon handler:**
`onFaviconChanged` was likely written as a placeholder. The developer called `entry->setUrl(entry->url())` — probably intending to trigger a model refresh — but this re-sets the same URL value, which `setUrl`'s guard (`if (m_url != url)`) should block. However, the `QUrl` parameter `faviconUrl` is never used at all. `TabEntry` has no `faviconUrl` string property; it only has a `QPixmap favicon` property with no way to receive a URL from QML.

**Bug 3 — No hash index:**
`TabManager` was designed with a `QList<TabEntry*> m_tabs` as the primary data structure. Both `tabById` and `indexOfTab` iterate this list. No `QHash` was added alongside it to provide O(1) lookup.

**Bug 4 — Synchronous shutdown assumption:**
`closeAllTabs` was written assuming `WebEngineView` objects emit close signals synchronously or near-synchronously. In practice, Chromium renderer teardown is asynchronous and can take seconds. The `QEventLoop` was added as a workaround but causes the UI freeze it was meant to avoid.

**Bug 5 — Developer-machine paths left in production code:**
The `possiblePaths` list in `initDatabase()` was built incrementally during development and never cleaned up. The hardcoded `/home/samarinara/...` entries were added for convenience and never removed before the code was committed.

**Bug 6 — Debug logging never removed:**
`qDebug` calls were added during development for tracing tab lifecycle events. They were never removed or gated behind a debug flag before the code reached its current state.

**Bug 7 — Local variable regex construction:**
`domainRegex` and `bangRegex` are declared as local variables inside their respective functions. The `static` keyword was not used, so the pattern is recompiled on every function call. This is a common C++ oversight with `QRegularExpression`.

---

## Correctness Properties

Property 1: Bug Condition — Lazy WebEngineView Instantiation

_For any_ application state where multiple tabs are open, the fixed QML code SHALL only have an active (instantiated) `WebEngineView` for the currently active tab; all other tabs SHALL use a `Loader` with `active: false` (or equivalent) so their Chromium renderer processes are not started until the tab is first activated.

**Validates: Requirements 2.1, 2.2**

---

Property 2: Bug Condition — Correct Favicon Storage and Signal Emission

_For any_ favicon-changed event received by `TabManager::onFaviconChanged`, the fixed function SHALL store the favicon URL in `TabEntry` (via a `faviconUrl` string property) and SHALL NOT call `entry->setUrl(entry->url())`, ensuring no spurious `urlChanged` signal is emitted and the favicon URL is propagated to the QML model.

**Validates: Requirements 2.3, 2.4**

---

Property 3: Bug Condition — O(1) Tab Lookup

_For any_ tab lifecycle event (title change, URL change, loading state change, favicon change), the fixed `tabById` and `indexOfTab` functions SHALL locate the `TabEntry` in O(1) time using a `QHash<int, TabEntry*>` index, and the hash SHALL remain consistent with `m_tabs` across all create and remove operations.

**Validates: Requirements 2.5, 2.6**

---

Property 4: Bug Condition — Non-Blocking Application Close

_For any_ application shutdown that triggers `closeAllTabs()`, the fixed function SHALL NOT spin a nested `QEventLoop` or block the UI thread; it SHALL release tab resources and allow the process to exit promptly without waiting for WebEngineView close signals.

**Validates: Requirements 2.7, 2.8**

---

Property 5: Bug Condition — Portable Bang Database Resolution

_For any_ machine running the application, the fixed `initDatabase()` function SHALL resolve `bangs.db` using only portable paths (`QCoreApplication::applicationDirPath()` relative paths or `qrc:/`) and SHALL NOT contain any hardcoded absolute paths referencing a specific developer's home directory.

**Validates: Requirements 2.9, 2.10**

---

Property 6: Bug Condition — No Unconditional Hot-Path Debug Logging

_For any_ navigation event (URL change, title change, loading state change) processed by the fixed handlers, the fixed code SHALL NOT emit unconditional `qDebug` messages; any retained diagnostic logging SHALL be gated behind a compile-time (`#ifdef QT_DEBUG`) or runtime debug flag.

**Validates: Requirement 2.11**

---

Property 7: Bug Condition — Static Regex Compilation

_For any_ number of calls to `UrlProcessor::isUrl()` and `UrlProcessor::handleBangRouting()`, the fixed functions SHALL use `static const QRegularExpression` for `domainRegex` and `bangRegex` respectively, ensuring each pattern is compiled exactly once for the lifetime of the process.

**Validates: Requirements 2.12, 2.13**

---

Property 8: Preservation — All Existing User-Visible Behaviors

_For any_ user interaction that does NOT involve the seven bug conditions (tab switching, new tab creation, tab closing, page load completion, bang routing, plain URL navigation, search fallback, tab limit enforcement, dark mode toggling, application startup), the fixed code SHALL produce exactly the same observable behavior as the original code.

**Validates: Requirements 3.1, 3.2, 3.3, 3.4, 3.5, 3.6, 3.7, 3.8, 3.9, 3.10**

---

## Fix Implementation

### Changes Required

**File: `qml/Main.qml`**

**Change 1 — Replace `Repeater` + direct `WebEngineView` with `Repeater` + `Loader`-based lazy instantiation:**
- Wrap the `WebEngineView` inside a `Loader` component within the delegate
- Set `loader.active: model.tabId === tabManager.activeTabId` (or track "ever activated" with a local `property bool hasBeenActivated`)
- Connect `WebEngineView` signals (`onTitleChanged`, `onIconChanged`, `onLoadingChanged`, `onUrlChanged`) inside the `Loader`'s `sourceComponent`
- Ensure the URL bar is updated when switching to a previously-loaded tab

---

**File: `src/tab_entry.h`**

**Change 2 — Add `faviconUrl` string property to `TabEntry`:**
- Add `Q_PROPERTY(QString faviconUrl READ faviconUrl NOTIFY faviconUrlChanged)`
- Add `QString m_faviconUrl` member variable
- Add `faviconUrl()` getter and `setFaviconUrl(const QString&)` setter with change guard
- Add `faviconUrlChanged()` signal

---

**File: `src/tab_manager.h` and `src/tab_manager.cpp`**

**Change 3 — Add `QHash<int, TabEntry*> m_tabIndex` for O(1) lookup:**
- Add `QHash<int, TabEntry*> m_tabIndex` member to `TabManager`
- In `createTab()`: insert `m_tabIndex[tabId] = entry` after creating the entry
- In `removeTabEntry()`: call `m_tabIndex.remove(tabId)` before deleting the entry
- Replace the loop body of `tabById()` with `return m_tabIndex.value(tabId, nullptr)`
- `indexOfTab()` still needs the list scan (for model index), but `tabById()` becomes O(1)

**Change 4 — Fix `onFaviconChanged` to store favicon URL:**
- Replace `entry->setUrl(entry->url())` with `entry->setFaviconUrl(faviconUrl.toString())`
- Add `FaviconUrlRole` to the role enum and `roleNames()` if QML needs to access it via the model
- Update `data()` to return `entry->faviconUrl()` for `FaviconUrlRole`

**Change 5 — Remove blocking `QEventLoop` from `closeAllTabs()`:**
- Remove the `QEventLoop loop`, `QTimer safetyTimer`, and all associated `connect`/`disconnect` calls
- Replace with a simple loop: iterate `m_tabs`, call `removeTabEntry` for each, clear `m_tabs`
- Remove `#include <QEventLoop>` and `#include <QTimer>` if no longer used elsewhere

**Change 6 — Remove unconditional `qDebug` calls from hot-path handlers:**
- In `onUrlChanged`: remove `qDebug() << "onUrlChanged called for tab"...`
- In `onTitleChanged`: remove any `qDebug` calls (currently none, but verify)
- In `onLoadingStateChanged`: remove any `qDebug` calls
- In `createTab()`: retain the tab-creation log (low frequency) but remove the per-event logs
- In `Main.qml` delegates: remove `console.log` from `onTitleChanged`, `onIconChanged`, `onLoadingChanged`, `onUrlChanged` handlers

---

**File: `src/urlprocessor.cpp`**

**Change 7 — Promote `domainRegex` and `bangRegex` to `static const`:**
- In `isUrl()`: change `QRegularExpression domainRegex(...)` to `static const QRegularExpression domainRegex(...)`
- In `handleBangRouting()`: change `QRegularExpression bangRegex(...)` to `static const QRegularExpression bangRegex(...)`

**Change 8 — Remove hardcoded absolute paths from `initDatabase()`:**
- Remove the two `/home/samarinara/...` entries from `possiblePaths`
- Retain only: `QCoreApplication::applicationDirPath() + "/bangs.db"`, `"./bangs.db"`, `"../bangs.db"`, `"../../bangs.db"`
- Consider adding a `qrc:/bangs.db` path if the database is bundled as a Qt resource

---

## Testing Strategy

### Validation Approach

The testing strategy follows a two-phase approach: first, surface counterexamples that demonstrate each bug on the unfixed code to confirm root cause analysis; then verify the fix works correctly and preserves all existing behavior.

---

### Exploratory Bug Condition Checking

**Goal**: Surface counterexamples that demonstrate each bug BEFORE implementing the fix. Confirm or refute the root cause analysis.

**Test Plan**: Write unit tests and structural checks that exercise each bug condition on the unfixed code. Observe failures to confirm root causes.

**Test Cases:**

1. **WebEngineView Eager Instantiation Test**: Create 3 tabs in the model and verify that 3 `WebEngineView` instances are created (will confirm Bug 1 on unfixed code)
2. **Favicon Storage Test**: Call `onFaviconChanged` with a valid URL and assert `TabEntry::faviconUrl()` is non-empty (will fail on unfixed code — property doesn't exist)
3. **Spurious URL Signal Test**: Call `onFaviconChanged` and count `urlChanged` signal emissions — expect 1 spurious emission on unfixed code
4. **tabById Complexity Test**: Create 50 tabs and measure iterations in `tabById` for the last tab (will show O(n) on unfixed code)
5. **closeAllTabs Blocking Test**: Call `closeAllTabs()` and measure wall-clock time — expect up to 5-second block on unfixed code
6. **Hardcoded Path Test**: Inspect `possiblePaths` in `initDatabase()` and assert no path starts with `/home/` (will fail on unfixed code)
7. **Hot-Path Logging Test**: Call `onUrlChanged` and capture `qDebug` output — expect log messages on unfixed code
8. **Regex Recompilation Test**: Call `UrlProcessor::process()` twice and verify `domainRegex` is a local variable (structural check — will confirm Bug 7)

**Expected Counterexamples:**
- `TabEntry` has no `faviconUrl` property → compilation error or missing property
- `tabById` iterates all tabs → confirmed by loop inspection
- `closeAllTabs` contains `QEventLoop` → confirmed by code inspection
- Hardcoded paths present → confirmed by string search in source

---

### Fix Checking

**Goal**: Verify that for all inputs where each bug condition holds, the fixed code produces the expected behavior.

**Pseudocode:**
```
FOR ALL state WHERE isBugCondition_N(state) DO
  result := fixedFunction(state)
  ASSERT property_N(result)
END FOR
```

**Specific checks per bug:**
- Bug 1: For N > 1 tabs, assert only 1 `Loader` has `active: true`
- Bug 2: For any favicon event, assert `TabEntry::faviconUrl()` equals the received URL and no spurious `urlChanged` emitted
- Bug 3: For any tab event, assert `tabById` uses hash lookup (O(1))
- Bug 4: For `closeAllTabs()`, assert no `QEventLoop` is spun and function returns promptly
- Bug 5: For `initDatabase()`, assert `possiblePaths` contains no `/home/` entries
- Bug 6: For any navigation event, assert no unconditional `qDebug` output
- Bug 7: For repeated `process()` calls, assert `domainRegex` and `bangRegex` are `static const`

---

### Preservation Checking

**Goal**: Verify that for all inputs where the bug conditions do NOT hold, the fixed code produces the same result as the original code.

**Pseudocode:**
```
FOR ALL input WHERE NOT isBugCondition_N(input) DO
  ASSERT originalFunction(input) = fixedFunction(input)
END FOR
```

**Testing Approach**: Property-based testing is recommended for preservation checking because:
- It generates many test cases automatically across the input domain
- It catches edge cases that manual unit tests might miss
- It provides strong guarantees that behavior is unchanged for all non-buggy inputs

**Test Plan**: Observe behavior on unfixed code first for all preserved behaviors, then write property-based tests capturing that behavior.

**Test Cases:**
1. **Tab Switching Preservation**: Verify switching tabs still shows correct content and updates URL bar
2. **New Tab Creation Preservation**: Verify `createTab()` still creates entry, sets URL, activates tab, and emits `tabCreated`
3. **Tab Close Preservation**: Verify `closeTab()` still removes entry, switches to adjacent tab, and emits `tabClosed`
4. **Bang Routing Preservation**: For any valid bang query, verify `UrlProcessor::process()` returns the same URL before and after fix
5. **Plain URL Preservation**: For any plain URL or domain, verify `process()` returns the same result
6. **Search Fallback Preservation**: For any non-URL, non-bang query, verify fallback routing is unchanged
7. **Tab Limit Preservation**: Verify `createTab()` still returns -1 when 50 tabs are open
8. **Dark Mode Preservation**: Verify theme property bindings are unaffected by C++ changes

---

### Unit Tests

- Test `TabEntry::setFaviconUrl()` stores value and emits `faviconUrlChanged` signal
- Test `TabManager::onFaviconChanged()` calls `setFaviconUrl` and does NOT call `setUrl` with same value
- Test `TabManager::tabById()` returns correct entry using hash lookup after fix
- Test `TabManager::createTab()` inserts entry into `m_tabIndex`
- Test `TabManager::closeTab()` removes entry from `m_tabIndex`
- Test `TabManager::closeAllTabs()` completes without blocking (no `QEventLoop`)
- Test `UrlProcessor::process()` returns correct URLs for bang queries, plain URLs, and search terms after regex fix
- Test `initDatabase()` path list contains no hardcoded absolute paths

### Property-Based Tests

- Generate random sets of tab IDs and verify `m_tabIndex` is always consistent with `m_tabs` after any sequence of create/close operations
- Generate random bang queries and verify `process()` returns identical results before and after the static regex change
- Generate random domain strings and verify `isUrl()` returns identical results before and after the static regex change
- Generate random sequences of favicon events and verify no spurious `urlChanged` signals are emitted

### Integration Tests

- Full navigation flow: open tab → navigate to URL → verify title, URL, favicon update correctly
- Multi-tab flow: open 3 tabs → switch between them → verify only active tab's `Loader` is active
- Shutdown flow: open 5 tabs → close application → verify `closeAllTabs()` completes promptly
- Bang routing flow: enter `!g hello world` → verify navigation to correct Google search URL
- Cross-machine portability: run on a machine without `/home/samarinara/` → verify bang database loads from `applicationDirPath()`
