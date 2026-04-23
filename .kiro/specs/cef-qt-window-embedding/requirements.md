# Requirements Document

## Introduction

The Lilypad application is a C++/Qt6 desktop browser built on the Chromium Embedded Framework (CEF). Currently, the CEF-rendered webpage opens as a second, separate top-level desktop window alongside the Qt main window. The goal of this feature is to embed the CEF browser view directly inside the existing Qt main window, so that the URL bar and the browser viewport appear together in a single unified window. No second window should appear at any point during the application lifecycle.

The embedding mechanism relies on CEF's `SetAsChild()` API, which reparents the CEF native window under a Qt widget's native window handle (`winId()`). The Qt widget (`m_container`) must be a native window (via `Qt::WA_NativeWindow`) and must have valid, non-zero geometry before `CefBrowserHost::CreateBrowser()` is called.

## Glossary

- **MainWindow**: The top-level `QMainWindow` subclass that owns the URL bar and the browser container widget.
- **Container**: The `QWidget` (`m_container`) that acts as the native parent window for the CEF browser view. It must carry the `Qt::WA_NativeWindow` attribute.
- **CefHandler**: The `CefClient` / `CefLifeSpanHandler` implementation that receives CEF lifecycle callbacks and holds a reference to the live `CefBrowser` instance.
- **Browser**: The `CefBrowser` object created by `CefBrowserHost::CreateBrowser()`, whose native window is embedded inside the Container.
- **URL Bar**: The `QLineEdit` widget at the top of MainWindow that displays the current URL and accepts navigation input from the user.
- **Embedding**: The act of setting the CEF browser's native window as a child of the Container's native window handle using `window_info.SetAsChild(container->winId(), rect)`.
- **Geometry Rect**: The `cef_rect_t` value derived from the Container's pixel dimensions at the moment `createBrowser()` is called, used to size the initial CEF viewport.
- **CefDoMessageLoopWork**: The CEF API called on a periodic Qt timer to give CEF a processing timeslice within Qt's event loop.

---

## Requirements

### Requirement 1: Single Unified Window

**User Story:** As a user, I want the browser view and the URL bar to appear in a single window, so that I do not have to manage two separate windows when browsing.

#### Acceptance Criteria

1. WHEN the application launches, THE MainWindow SHALL display exactly one top-level window on the desktop.
2. WHEN the CEF browser is created, THE MainWindow SHALL contain both the URL Bar and the Browser viewport within the same window frame.
3. WHEN the application is running, THE MainWindow SHALL NOT spawn any additional top-level windows beyond the initial MainWindow.

---

### Requirement 2: CEF Browser Embedded as Native Child

**User Story:** As a developer, I want the CEF browser window to be embedded as a native child of the Qt container widget, so that the browser renders inside the Qt layout rather than as a floating window.

#### Acceptance Criteria

1. THE Container SHALL carry the `Qt::WA_NativeWindow` attribute so that it possesses a valid native window handle before `createBrowser()` is called.
2. WHEN `createBrowser()` is called, THE CefHandler SHALL invoke `window_info.SetAsChild()` with the Container's native window handle (`winId()`) and a Geometry Rect derived from the Container's current dimensions.
3. WHEN the Browser is created, THE Browser's native window SHALL be a child of the Container's native window, not a child of the desktop root window.
4. WHEN the application window is shown, THE Container SHALL be placed below the URL Bar within the MainWindow's vertical layout, occupying the remaining available space.

---

### Requirement 3: Valid Geometry at Browser Creation Time

**User Story:** As a developer, I want the container widget to have non-zero dimensions when the browser is created, so that the CEF viewport is sized correctly from the start and the page is rendered visibly.

#### Acceptance Criteria

1. WHEN `createBrowser()` is called, THE Container SHALL have a width greater than 0 pixels and a height greater than 0 pixels.
2. WHEN `createBrowser()` is called, THE Geometry Rect passed to `SetAsChild()` SHALL reflect the Container's actual pixel dimensions at that moment.
3. IF `createBrowser()` is called before the MainWindow has processed its first paint event, THEN THE MainWindow SHALL defer the call until after Qt has processed the pending layout and paint events.
4. THE MainWindow SHALL use `QTimer::singleShot(0, ...)` to defer the `createBrowser()` call so that the Qt event loop processes the layout before the Browser is created.

---

### Requirement 4: URL Bar Navigation

**User Story:** As a user, I want to type a URL in the address bar and press Enter to navigate the embedded browser to that URL, so that I can visit different web pages without leaving the application.

#### Acceptance Criteria

1. WHEN the user presses the Return key while the URL Bar has focus, THE MainWindow SHALL call `LoadURL()` on the Browser's main frame with the exact text currently in the URL Bar.
2. WHEN `LoadURL()` is called, THE Browser SHALL begin loading the URL provided by the URL Bar without modification.
3. IF the Browser has not yet been created when the Return key is pressed, THEN THE MainWindow SHALL take no navigation action.

---

### Requirement 5: CEF Event Loop Integration

**User Story:** As a developer, I want CEF to receive regular processing timeslices within the Qt event loop, so that the embedded browser renders and responds to network and input events correctly.

#### Acceptance Criteria

1. THE MainWindow SHALL NOT use CEF's built-in message loop; instead, `CefDoMessageLoopWork()` SHALL be driven by a Qt timer.
2. WHEN the Qt application event loop is running, THE application SHALL call `CefDoMessageLoopWork()` at an interval no greater than 16 milliseconds.
3. WHEN the Qt application event loop exits, THE application SHALL call `CefShutdown()` exactly once before the process terminates.

---

### Requirement 6: Browser Lifecycle Management

**User Story:** As a developer, I want the application to correctly track the browser instance from creation to destruction, so that navigation commands are only sent to a live browser and resources are released cleanly on exit.

#### Acceptance Criteria

1. WHEN `OnAfterCreated()` is called by CEF, THE CefHandler SHALL store a reference to the Browser.
2. WHEN `OnBeforeClose()` is called by CEF, THE CefHandler SHALL release the stored Browser reference.
3. WHEN a navigation command is issued via the URL Bar, THE MainWindow SHALL only call `LoadURL()` if the CefHandler holds a non-null Browser reference.
4. THE CefHandler SHALL expose a `GetBrowser()` accessor that returns the currently stored Browser reference, or null if no Browser is live.
