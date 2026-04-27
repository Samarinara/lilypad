# Requirements Document

## Introduction

This feature adds a vertical tab panel to the left side of the Lilypad browser window. The panel displays all open tabs as a vertical list with favicons, titles, and close buttons. The existing URL bar remains unchanged at the top. The feature also introduces the underlying tab management architecture: a `TabManager` that owns multiple `TabEntry` objects, each pairing a `CefHandler` with a `CefOSRWidget`, so that browser instances are created, switched, and destroyed safely and efficiently.

## Glossary

- **Tab_Panel**: The vertical sidebar widget rendered on the left side of the main window, containing the list of tab entries.
- **Tab_Entry**: A single row in the Tab_Panel representing one open browser tab. Contains a favicon, a title label, and a close button.
- **Tab_Manager**: The C++ object that owns all `TabEntry` instances, enforces tab lifecycle rules, and coordinates switching between tabs.
- **Active_Tab**: The `TabEntry` whose associated `CefOSRWidget` is currently visible and receiving input.
- **CEF_Browser**: A `CefRefPtr<CefBrowser>` instance managed by a `CefHandler`, representing one Chromium browser process.
- **CEF_Handler**: A `CefHandler` instance that implements `CefClient`, `CefLifeSpanHandler`, and `CefRenderHandler` for one `CEF_Browser`.
- **OSR_Widget**: A `CefOSRWidget` instance that renders the off-screen bitmap produced by one `CEF_Handler` and forwards Qt input events to the associated `CEF_Browser`.
- **URL_Bar**: The existing `QLineEdit` widget at the top of the main window that displays and accepts URLs.
- **Favicon**: A small icon (16×16 or 32×32 pixels) associated with a web page, displayed in the Tab_Entry.
- **Tab_ID**: A unique integer identifier assigned to each `TabEntry` at creation time and never reused within a session.

---

## Requirements

### Requirement 1: Vertical Tab Panel Layout

**User Story:** As a user, I want to see all my open tabs listed vertically on the left side of the browser, so that I can identify and switch between tabs without losing horizontal screen space.

#### Acceptance Criteria

1. THE Tab_Panel SHALL be rendered as a fixed-width vertical sidebar on the left side of the main window, to the left of the OSR_Widget.
2. THE Tab_Panel SHALL remain visible at all times while the browser window is open.
3. THE URL_Bar SHALL remain positioned at the top of the window, spanning the full width above both the Tab_Panel and the OSR_Widget.
4. WHEN the main window is resized, THE Tab_Panel SHALL maintain its fixed width and THE OSR_Widget SHALL expand or contract to fill the remaining horizontal space.
5. THE Tab_Panel SHALL display Tab_Entry rows in the order the tabs were opened, from top to bottom.

---

### Requirement 2: Tab Entry Display

**User Story:** As a user, I want each tab entry to show the page favicon and title, so that I can quickly identify which tab is which.

#### Acceptance Criteria

1. EACH Tab_Entry SHALL display the Favicon of the associated page at 16×16 pixels on the left side of the row.
2. EACH Tab_Entry SHALL display the page title as a single-line truncated string to the right of the Favicon.
3. WHEN a page title is not yet available, THE Tab_Entry SHALL display the URL of the page as the title.
4. WHEN a Favicon is not yet available or fails to load, THE Tab_Entry SHALL display a default placeholder icon.
5. THE Active_Tab's Tab_Entry SHALL be visually distinguished from inactive Tab_Entry rows using a highlighted background color.
6. EACH Tab_Entry SHALL display a close button on the right side of the row.
7. WHEN the cursor hovers over a Tab_Entry, THE Tab_Panel SHALL highlight that Tab_Entry to indicate it is interactive.

---

### Requirement 3: Tab Creation

**User Story:** As a user, I want to open new tabs, so that I can browse multiple pages simultaneously.

#### Acceptance Criteria

1. THE Tab_Panel SHALL display a "New Tab" button at the bottom of the Tab_Panel.
2. WHEN the "New Tab" button is activated, THE Tab_Manager SHALL create a new TabEntry with a unique Tab_ID and open the browser's default new-tab URL.
3. WHEN a new TabEntry is created, THE Tab_Manager SHALL instantiate a new CEF_Handler and a new OSR_Widget for that TabEntry.
4. WHEN a new TabEntry is created, THE Tab_Manager SHALL set the new tab as the Active_Tab.
5. THE Tab_Manager SHALL support a minimum of 1 and a maximum of 50 simultaneously open tabs.
6. IF the number of open tabs equals 50, THEN THE Tab_Manager SHALL ignore further tab creation requests.

---

### Requirement 4: Tab Switching

**User Story:** As a user, I want to click a tab entry to switch to that tab, so that I can move between open pages.

#### Acceptance Criteria

1. WHEN a Tab_Entry is clicked, THE Tab_Manager SHALL set the corresponding tab as the Active_Tab.
2. WHEN the Active_Tab changes, THE Tab_Manager SHALL hide the OSR_Widget of the previously active tab and show the OSR_Widget of the newly active tab.
3. WHEN the Active_Tab changes, THE URL_Bar SHALL display the current URL of the newly active tab's CEF_Browser.
4. WHEN the Active_Tab changes, THE OSR_Widget of the newly active tab SHALL receive keyboard and mouse focus.
5. WHEN the Active_Tab changes, THE Tab_Manager SHALL NOT destroy or reload the CEF_Browser of the previously active tab.

---

### Requirement 5: Tab Closing

**User Story:** As a user, I want to close individual tabs, so that I can free resources and keep my workspace tidy.

#### Acceptance Criteria

1. WHEN the close button of a Tab_Entry is clicked, THE Tab_Manager SHALL close the corresponding tab.
2. WHEN a tab is closed, THE Tab_Manager SHALL call `CloseBrowser(true)` on the associated CEF_Browser before releasing the CEF_Handler and OSR_Widget.
3. WHEN a tab is closed and it was the Active_Tab, THE Tab_Manager SHALL activate the tab immediately preceding it in the list, or the next tab if no preceding tab exists.
4. WHEN the last remaining tab is closed, THE Tab_Manager SHALL open a new tab with the default new-tab URL rather than closing the window.
5. WHEN a tab is closed, THE Tab_Manager SHALL remove the corresponding Tab_Entry from the Tab_Panel.
6. WHEN a tab is closed, THE Tab_ID of the closed tab SHALL NOT be reused for any subsequently created tab within the same session.

---

### Requirement 6: URL Bar Synchronization

**User Story:** As a user, I want the URL bar to always reflect the current page of the active tab, so that I can see and edit the URL I am on.

#### Acceptance Criteria

1. WHEN the Active_Tab's CEF_Browser navigates to a new URL, THE URL_Bar SHALL update to display the new URL.
2. WHEN the user submits a URL in the URL_Bar, THE Tab_Manager SHALL load that URL in the Active_Tab's CEF_Browser.
3. WHEN the Active_Tab changes, THE URL_Bar SHALL immediately display the URL of the newly active tab's CEF_Browser.
4. WHEN the Active_Tab's CEF_Browser is loading a page, THE URL_Bar SHALL display the URL of the page being loaded.

---

### Requirement 7: Tab Title and Favicon Updates

**User Story:** As a user, I want tab titles and favicons to update as pages load, so that I always see accurate information in the tab list.

#### Acceptance Criteria

1. WHEN a page title changes in a CEF_Browser, THE Tab_Manager SHALL update the title displayed in the corresponding Tab_Entry.
2. WHEN a page favicon changes in a CEF_Browser, THE Tab_Manager SHALL update the Favicon displayed in the corresponding Tab_Entry.
3. WHEN a CEF_Browser begins loading a page, THE corresponding Tab_Entry SHALL display a loading indicator in place of the Favicon.
4. WHEN a CEF_Browser finishes loading a page, THE corresponding Tab_Entry SHALL replace the loading indicator with the page Favicon or the default placeholder icon if no Favicon is available.

---

### Requirement 8: Tab Manager Lifecycle and Thread Safety

**User Story:** As a developer, I want the tab management layer to handle CEF's multi-threaded lifecycle correctly, so that browser instances are created and destroyed without crashes or resource leaks.

#### Acceptance Criteria

1. THE Tab_Manager SHALL create each CEF_Browser on the CEF UI thread using `CefBrowserHost::CreateBrowser`.
2. WHEN a tab is closed, THE Tab_Manager SHALL initiate browser shutdown by calling `CloseBrowser(true)` and SHALL wait for `OnBeforeClose` to fire before releasing the CEF_Handler.
3. THE Tab_Manager SHALL NOT access `CefRefPtr<CefBrowser>` members from the Qt main thread without first verifying the pointer is non-null.
4. WHEN the application exits, THE Tab_Manager SHALL close all open CEF_Browser instances before `CefShutdown` is called.
5. THE Tab_Manager SHALL serialize all tab creation and destruction operations through the Qt main thread event loop to prevent concurrent modification of the tab list.
6. IF a CEF_Browser fails to initialize, THEN THE Tab_Manager SHALL remove the corresponding Tab_Entry from the Tab_Panel and log the failure.

---

### Requirement 9: Input Isolation Between Tabs

**User Story:** As a developer, I want each tab's OSR widget to only receive input when it is the active tab, so that background tabs are not accidentally interacted with.

#### Acceptance Criteria

1. WHILE a tab is not the Active_Tab, THE corresponding OSR_Widget SHALL NOT forward mouse events to its CEF_Browser.
2. WHILE a tab is not the Active_Tab, THE corresponding OSR_Widget SHALL NOT forward keyboard events to its CEF_Browser.
3. WHILE a tab is not the Active_Tab, THE corresponding OSR_Widget SHALL be hidden and SHALL NOT be painted.
4. WHEN a tab becomes the Active_Tab, THE corresponding OSR_Widget SHALL become visible and SHALL resume forwarding input events to its CEF_Browser.

---

### Requirement 10: Resource Efficiency for Background Tabs

**User Story:** As a developer, I want background tabs to consume minimal rendering resources, so that the browser remains performant with many open tabs.

#### Acceptance Criteria

1. WHILE a tab is not the Active_Tab, THE corresponding CEF_Browser SHALL have its rendering suspended by calling `WasHidden(true)` on the browser host.
2. WHEN a tab becomes the Active_Tab, THE corresponding CEF_Browser SHALL have its rendering resumed by calling `WasHidden(false)` on the browser host.
3. WHILE a tab is not the Active_Tab, THE Tab_Manager SHALL NOT trigger repaints of the corresponding OSR_Widget in response to `OnPaint` callbacks.
