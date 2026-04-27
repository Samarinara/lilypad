# Implementation Plan: Vertical Tab Panel

## Overview

Implement multi-tab support for the Lilypad browser by introducing `TabEntry`, `TabManager`, `TabPanel`, and `TabEntryWidget` classes, extending `CefHandler` and `CefOSRWidget`, and refactoring `MainWindow` to delegate tab management. The implementation follows the CEF multi-threaded lifecycle rules and uses Qt6 with rapidcheck for property-based tests.

## Tasks

- [x] 1. Extend `CefOSRWidget` with input and rendering control flags
  - Add `m_inputEnabled` and `m_renderEnabled` bool members (default `true`)
  - Implement `setInputEnabled(bool)` and `setRenderEnabled(bool)` methods
  - Gate all mouse and keyboard event handlers with `if (m_inputEnabled)` checks
  - Gate `paintEvent` with `if (m_renderEnabled)` check
  - Update `CMakeLists.txt` if any new source files are added
  - _Requirements: 9.1, 9.2, 9.3, 9.4_

  - [x] 1.1 Write unit tests for `CefOSRWidget` input/render flags
    - Test that with `setInputEnabled(false)`, mouse events are not forwarded to CEF
    - Test that with `setRenderEnabled(false)`, `paintEvent` does not draw
    - _Requirements: 9.1, 9.2, 9.3_

- [x] 2. Implement `TabEntry` struct
  - Create `src/tab_entry.h` with the `TabEntry` struct as specified in the design
  - Implement `activate()`: show widget, enable input, call `WasHidden(false)`, set focus
  - Implement `deactivate()`: hide widget, disable input, call `WasHidden(true)`
  - Guard all `GetBrowser()` accesses with null checks
  - _Requirements: 4.2, 4.4, 9.1, 9.2, 9.3, 9.4, 10.1, 10.2_

- [x] 3. Implement `TabManager`
  - Create `src/tab_manager.h` and `src/tab_manager.cpp`
  - Implement `createTab(url)`: allocate `TabEntry`, create `CefHandler` and `CefOSRWidget`, call `CefBrowserHost::CreateBrowser`, enforce 50-tab cap, return tabId or -1
  - Implement `closeTab(tabId)`: call `CloseBrowser(true)`, mark entry as closing, select successor tab per Property 11 logic
  - Implement `switchToTab(tabId)`: deactivate old active tab, activate new tab, emit `activeTabChanged`
  - Implement `onBrowserCreated`, `onBeforeClose`, `onTitleChanged`, `onFaviconChanged`, `onLoadingStateChanged`, `onUrlChanged` callback methods
  - Implement `closeAllTabs()` for application exit using a `QEventLoop` to pump CEF messages
  - Add a 10-second `QTimer` per tab creation to handle `OnAfterCreated` timeout (Requirement 8.6)
  - Add `closeTab` last-tab guard: if closing the last tab, open a new tab first
  - Add all Qt signals: `tabCreated`, `tabClosed`, `activeTabChanged`, `titleChanged`, `faviconChanged`, `loadingStateChanged`, `urlChanged`
  - _Requirements: 3.2, 3.3, 3.4, 3.5, 3.6, 4.1, 4.2, 4.5, 5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 8.1, 8.2, 8.3, 8.4, 8.5, 8.6_

  - [x] 3.1 Write property test for tab count invariant after creation (Property 1)
    - **Property 1: Tab count invariant after creation**
    - Generator: random initial tab count in [0, 49], random URL string
    - Verify: `tabCount() == initial + 1`, `entry->handler != null`, `entry->osrWidget != null`
    - **Validates: Requirements 3.2, 3.3, 3.5**

  - [x] 3.2 Write property test for tab count cap enforcement (Property 2)
    - **Property 2: Tab count cap enforcement**
    - Generator: fill manager to 50 tabs, attempt one more `createTab` with random URL
    - Verify: return value == -1, `tabCount() == 50`
    - **Validates: Requirements 3.5, 3.6**

  - [x] 3.3 Write property test for tab ID uniqueness (Property 3)
    - **Property 3: Tab ID uniqueness across a session**
    - Generator: random sequence of `createTab`/`closeTab` operations (length 1..100)
    - Verify: set of all ever-assigned IDs has no duplicates
    - **Validates: Requirements 3.2, 5.6**

  - [x] 3.4 Write property test for new tab becomes active (Property 4)
    - **Property 4: New tab becomes active**
    - Generator: random initial tab count [0, 49], random URL
    - Verify: `activeTab()->tabId == returned tabId`
    - **Validates: Requirements 3.4**

  - [x] 3.5 Write property test for active tab and OSR visibility invariant (Property 5)
    - **Property 5: Active tab and OSR visibility invariant after switch**
    - Generator: random number of tabs [1, 10], random valid tab ID to switch to
    - Verify: `activeTab()->tabId == target`, exactly one OSR widget visible, all others hidden with `inputEnabled == false`
    - **Validates: Requirements 4.1, 4.2, 9.1, 9.2, 9.3, 9.4**

  - [x] 3.6 Write property test for previously active browser preserved after switch (Property 6)
    - **Property 6: Previously active browser preserved after switch**
    - Generator: random tab list [2, 10], random switch target
    - Verify: previously active tab's `handler->GetBrowser()` is non-null after switch
    - **Validates: Requirements 4.5**

  - [x] 3.7 Write property test for last-tab close opens a new tab (Property 10)
    - **Property 10: Last-tab close opens a new tab**
    - Generator: single-tab manager
    - Verify: after `closeTab`, `tabCount() == 1`
    - **Validates: Requirements 5.4**

  - [x] 3.8 Write property test for active tab selection after close (Property 11)
    - **Property 11: Active tab selection after close**
    - Generator: random tab list [2, 50], random active tab index i
    - Verify: new active tab is tab at index i-1, or index 0 if i was 0
    - **Validates: Requirements 5.3**

  - [x] 3.9 Write property test for close removes tab from manager (Property 12)
    - **Property 12: Close removes tab from manager and panel**
    - Generator: random tab list [1, 10], random tab to close
    - Verify: `tabById(id) == null`, `tabCount` decreased by 1
    - **Validates: Requirements 5.1, 5.5**

- [x] 4. Checkpoint — Ensure all `TabManager` tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [x] 5. Extend `CefHandler` with tab metadata and display/load callbacks
  - Add `tabId` member and `m_tabManager` non-owning pointer to `CefHandler`
  - Implement `CefDisplayHandler` interface: `GetDisplayHandler()`, `OnTitleChange`, `OnAddressChange`, `OnFaviconURLChange`
  - Implement `CefLoadHandler` interface: `GetLoadHandler()`, `OnLoadingStateChange`
  - Implement favicon download via `CefURLRequest`: on `OnFaviconURLChange`, request the first URL, convert response bytes to `QPixmap`, call `m_tabManager->onFaviconChanged`
  - Forward all callbacks to `TabManager` via direct method calls
  - _Requirements: 7.1, 7.2, 7.3, 7.4, 8.1, 8.2_

- [x] 6. Implement `TabEntryWidget`
  - Create `src/tab_entry_widget.h` and `src/tab_entry_widget.cpp`
  - Build layout: `QLabel` for favicon (16×16), `QLabel` for title (elided), `QPushButton` for close button
  - Implement `setActive(bool)`: apply/remove highlight background using `palette().highlight()`
  - Implement `setTitle(const QString&)`: display URL as fallback when title is empty
  - Implement `setFavicon(const QPixmap&)`: update favicon label; show placeholder if pixmap is null
  - Implement `setLoading(bool)`: show `QMovie` spinner when loading, show favicon/placeholder when not loading
  - Implement `mousePressEvent`, `enterEvent`, `leaveEvent` for click and hover highlighting
  - Emit `clicked(tabId)` and `closeClicked(tabId)` signals
  - _Requirements: 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7_

  - [x] 6.1 Write unit tests for `TabEntryWidget`
    - Test `setActive(true)` applies highlight styling
    - Test `setActive(false)` removes highlight styling
    - Test `setLoading(true)` shows spinner and hides favicon
    - Test `setLoading(false)` shows favicon and hides spinner
    - Test `setTitle("")` with a URL set displays the URL
    - _Requirements: 2.3, 2.4, 2.5, 7.3, 7.4_

  - [x] 6.2 Write property test for title/URL fallback (Property 13)
    - **Property 13: Title/URL fallback**
    - Generator: random URL string, empty title string
    - Verify: `TabEntryWidget` display text == URL
    - **Validates: Requirements 2.3**

  - [x] 6.3 Write property test for loading state display round-trip (Property 14)
    - **Property 14: Loading state display round-trip**
    - Generator: random tab, random favicon pixmap
    - Verify: after `onLoadingStateChanged(true)` → loading indicator visible; after `onLoadingStateChanged(false)` → loading indicator hidden
    - **Validates: Requirements 7.3, 7.4**

- [x] 7. Implement `TabPanel`
  - Create `src/tab_panel.h` and `src/tab_panel.cpp`
  - Build layout: fixed width 220 px, `QScrollArea` containing `QVBoxLayout` for tab rows, "New Tab" `QPushButton` at the bottom
  - Connect to `TabManager` signals: `tabCreated` → `onTabCreated`, `tabClosed` → `onTabClosed`, `activeTabChanged` → `onActiveTabChanged`, `titleChanged` → `onTitleChanged`, `faviconChanged` → `onFaviconChanged`, `loadingStateChanged` → `onLoadingStateChanged`
  - Implement `addTabWidget(tabId)`: create `TabEntryWidget`, add to layout and `m_widgets` map, connect its `clicked` and `closeClicked` signals
  - Implement `removeTabWidget(tabId)`: remove from layout and `m_widgets` map, delete widget
  - Implement `setActiveTab(tabId)`: call `setActive(true)` on new widget, `setActive(false)` on old widget
  - Disable "New Tab" button when `tabCount() == 50`
  - Emit `newTabRequested`, `tabCloseRequested(tabId)`, `tabSwitchRequested(tabId)` signals
  - _Requirements: 1.1, 1.2, 1.5, 2.5, 2.7, 3.1, 5.1_

  - [x] 7.1 Write property test for tab panel order matches creation order (Property 8)
    - **Property 8: Tab panel order matches creation order**
    - Generator: random sequence of `createTab` calls [1, 20]
    - Verify: `TabPanel` widget order matches `m_tabs` list order (top to bottom, oldest to newest)
    - **Validates: Requirements 1.5**

  - [x] 7.2 Write property test for close removes tab from panel (Property 12 — panel side)
    - **Property 12: Close removes tab from manager and panel**
    - Generator: random tab list [1, 10], random tab to close
    - Verify: `TabPanel` has no `TabEntryWidget` for the closed ID
    - **Validates: Requirements 5.1, 5.5**

- [x] 8. Checkpoint — Ensure all `TabPanel` and `TabEntryWidget` tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [x] 9. Refactor `MainWindow` to use `TabManager` and `TabPanel`
  - Remove direct ownership of `CefHandler` and `CefOSRWidget` from `MainWindow`
  - Add `TabManager* m_tabManager`, `TabPanel* m_tabPanel`, and `QWidget* m_osrContainer` members
  - Build layout: `QVBoxLayout` with URL bar on top; `QHBoxLayout` below with `TabPanel` on the left and `m_osrContainer` filling the rest
  - Implement `createInitialTab()`: call `m_tabManager->createTab(defaultUrl)`
  - Connect `TabPanel::newTabRequested` → `m_tabManager->createTab`
  - Connect `TabPanel::tabCloseRequested` → `m_tabManager->closeTab`
  - Connect `TabPanel::tabSwitchRequested` → `m_tabManager->switchToTab`
  - Implement `onActiveTabChanged`: update URL bar with new active tab's URL
  - Implement `onUrlChanged`: update URL bar when active tab navigates
  - Implement `onUrlBarSubmit`: load URL in active tab's browser
  - Override `closeEvent` to call `m_tabManager->closeAllTabs()` before accepting
  - Update `CMakeLists.txt` to include all new source files (`tab_entry.h`, `tab_manager.cpp`, `tab_panel.cpp`, `tab_entry_widget.cpp`)
  - _Requirements: 1.1, 1.2, 1.3, 1.4, 4.3, 4.4, 6.1, 6.2, 6.3, 6.4, 8.4_

  - [x] 9.1 Write property test for URL bar synchronization (Property 7)
    - **Property 7: URL bar synchronization**
    - Generator: random tab list [1, 10], random URL strings, random switch or navigation event
    - Verify: URL bar text == active tab's current URL
    - **Validates: Requirements 4.3, 6.1, 6.3, 6.4**

  - [x] 9.2 Write property test for tab panel fixed width invariant (Property 9)
    - **Property 9: Tab panel fixed width invariant**
    - Generator: random window widths [400, 2000]
    - Verify: `TabPanel` width == `kPanelWidth` (220), OSR container width == window width - `kPanelWidth`
    - **Validates: Requirements 1.1, 1.4**

  - [x] 9.3 Write property test for rendering suspension for inactive tabs (Property 15)
    - **Property 15: Rendering suspension for inactive tabs**
    - Generator: random tab list [2, 10], random sequence of `switchToTab` calls
    - Verify: for each switch, `WasHidden(true)` called on old active, `WasHidden(false)` called on new active; all inactive tabs have `renderEnabled == false`
    - **Validates: Requirements 10.1, 10.2, 10.3**

- [x] 10. Final checkpoint — Ensure all tests pass and the application builds
  - Ensure all tests pass, ask the user if questions arise.
  - Verify the application builds cleanly with `cmake --build`.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- Property-based tests use [rapidcheck](https://github.com/emil-e/rapidcheck) integrated with Qt Test; each property runs a minimum of 100 iterations
- CEF types that cannot be instantiated in unit tests should be abstracted behind thin interfaces or mocked
- All `GetBrowser()` accesses must be null-guarded before use
- The 50-tab cap and tab ID non-reuse invariants are enforced entirely within `TabManager`
- `closeAllTabs()` uses a `QEventLoop` to pump CEF messages while waiting for all `OnBeforeClose` callbacks
