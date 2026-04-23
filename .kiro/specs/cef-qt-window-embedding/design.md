# Design Document: CEF-Qt Window Embedding

## Overview

The Lilypad application embeds a Chromium Embedded Framework (CEF) browser view directly inside a Qt6 `QMainWindow`. The goal is a single unified window where the URL bar and the browser viewport coexist in the same frame — no second top-level window ever appears.

The embedding mechanism is CEF's `SetAsChild()` API, which reparents the CEF native window under a Qt widget's native window handle (`winId()`). The two event loops — Qt's and CEF's — are bridged by calling `CefDoMessageLoopWork()` on a periodic `QTimer`, letting Qt drive both.

The current codebase already implements this design. This document formalises the architecture, data models, and correctness properties so that future changes can be verified against them.

---

## Architecture

The application has three layers:

```
┌─────────────────────────────────────────────────────┐
│                    Qt Layer                         │
│  QApplication  ──►  QTimer (10 ms)                 │
│                         │                           │
│  MainWindow (QMainWindow)                           │
│    ├── QLineEdit  (URL bar)                         │
│    └── QWidget    (m_container, WA_NativeWindow)    │
└─────────────────────────────────────────────────────┘
                          │ winId()
                          ▼
┌─────────────────────────────────────────────────────┐
│                   CEF Layer                         │
│  CefBrowserHost::CreateBrowser()                    │
│    └── CefBrowser  (native child of m_container)   │
│  CefHandler  (CefClient + CefLifeSpanHandler)       │
└─────────────────────────────────────────────────────┘
                          │ CefDoMessageLoopWork()
                          ▼
┌─────────────────────────────────────────────────────┐
│               Chromium Sub-processes                │
│  renderer  │  GPU  │  network  (spawned by CEF)     │
└─────────────────────────────────────────────────────┘
```

### Startup Sequence

```mermaid
sequenceDiagram
    participant OS
    participant main
    participant Qt
    participant CEF

    main->>CEF: CefExecuteProcess() — sub-processes exit here
    main->>CEF: CefInitialize(settings, QCefApp)
    main->>Qt: QApplication qapp
    main->>Qt: MainWindow w; w.show()
    main->>Qt: QTimer::singleShot(0, createBrowser)
    main->>Qt: QTimer cef_timer.start(10)
    main->>Qt: qapp.exec()  ← event loop starts
    Qt-->>main: singleShot fires
    main->>CEF: CefBrowserHost::CreateBrowser(SetAsChild(winId))
    CEF-->>main: OnAfterCreated(browser)
    loop every 10 ms
        Qt-->>CEF: CefDoMessageLoopWork()
    end
    Qt-->>main: qapp.exec() returns
    main->>CEF: CefShutdown()
```

### Key Design Decisions

**QTimer-driven CEF loop** — CEF supports an external message loop via `CefDoMessageLoopWork()`. This avoids running two blocking event loops and lets Qt remain the single owner of the thread. The 10 ms interval (100 Hz) is well within the 16 ms budget for 60 fps rendering.

**Deferred browser creation** — `createBrowser()` is called via `QTimer::singleShot(0, ...)` rather than directly after `w.show()`. This ensures Qt has processed the initial layout pass and `m_container` has non-zero geometry before CEF reads it.

**`WA_NativeWindow` attribute** — Without this attribute, Qt may not allocate a native OS window handle for `m_container` until it is actually painted. Setting it eagerly guarantees `winId()` returns a valid handle at `createBrowser()` time.

---

## Components and Interfaces

### `QCefApp` (`main.cpp`)

Minimal `CefApp` / `CefBrowserProcessHandler` subclass. Its only responsibility is to append GPU-disabling command-line switches before CEF launches sub-processes.

```cpp
class QCefApp : public CefApp, public CefBrowserProcessHandler {
    void OnBeforeCommandLineProcessing(...) override;
    // appends: disable-gpu, disable-software-rasterizer, etc.
};
```

### `MainWindow` (`mainwindow.h / .cpp`)

Top-level `QMainWindow`. Owns the URL bar, the container widget, and the `CefHandler`.

| Member | Type | Purpose |
|---|---|---|
| `m_container` | `QWidget*` | Native parent for the CEF browser window |
| `m_urlBar` | `QLineEdit*` | Address bar; drives navigation via `returnPressed` |
| `m_handler` | `CefHandler*` | Holds the live `CefBrowser` reference |

**`createBrowser(url)`** — Called once, after the window is shown. Reads `m_container->geometry()`, builds a `cef_rect_t`, calls `window_info.SetAsChild(m_container->winId(), rect)`, then calls `CefBrowserHost::CreateBrowser()`.

**Navigation slot** — Connected to `m_urlBar->returnPressed`. Calls `m_handler->GetBrowser()->GetMainFrame()->LoadURL(m_urlBar->text().toStdString())` only when `GetBrowser()` is non-null.

### `CefHandler` (`cef_handler.h / .cpp`)

Implements `CefClient` and `CefLifeSpanHandler`. Holds a `CefRefPtr<CefBrowser>` that is set in `OnAfterCreated` and cleared in `OnBeforeClose`.

```cpp
class CefHandler : public QObject, public CefClient, public CefLifeSpanHandler {
public:
    CefRefPtr<CefBrowser> GetBrowser() const;
    void OnAfterCreated(CefRefPtr<CefBrowser>) override;
    void OnBeforeClose(CefRefPtr<CefBrowser>) override;
private:
    CefRefPtr<CefBrowser> m_browser;
};
```

### `main()` (`main.cpp`)

Orchestrates the startup sequence:

1. `CefExecuteProcess` — lets CEF sub-processes exit early.
2. `CefInitialize` — starts CEF with `no_sandbox = true` and an app-specific `root_cache_path`.
3. `QApplication` — creates the Qt application object.
4. `MainWindow::show()` — makes the window visible and triggers layout.
5. `QTimer::singleShot(0, createBrowser)` — defers browser creation by one event-loop tick.
6. `QTimer` at 10 ms — drives `CefDoMessageLoopWork()`.
7. `qapp.exec()` — runs the event loop until the window closes.
8. `CefShutdown()` — orderly CEF teardown.

---

## Data Models

### `CefWindowInfo` (CEF type)

Populated in `createBrowser()` before passing to `CefBrowserHost::CreateBrowser()`.

```
CefWindowInfo {
    parent_window : CefWindowHandle  ← m_container->winId()
    x, y          : int              ← 0, 0 (relative to container)
    width, height : int              ← m_container->geometry().width/height()
}
```

Set via `window_info.SetAsChild(handle, cef_rect_t{0, 0, w, h})`.

### `CefSettings` (CEF type)

```
CefSettings {
    no_sandbox       : true
    root_cache_path  : QStandardPaths::CacheLocation + "/lilypad"
}
```

The `root_cache_path` prevents CEF's process-singleton warning when the app is launched multiple times.

### `CefBrowserSettings` (CEF type)

All defaults — no custom browser settings are required for basic embedding.

### `CefHandler` internal state

```
CefHandler {
    m_browser : CefRefPtr<CefBrowser>   // null until OnAfterCreated; null after OnBeforeClose
}
```

---

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system — essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*

This feature is primarily structural (C++ source code with a specific call sequence) rather than a pure-function data transformation. The most effective testing strategy is static source analysis combined with property-based sampling over the input spaces that do vary (URL strings, container geometries). The properties below are formulated to be verifiable by static analysis of the source code and by property-based tests over the relevant input domains.

### Property 1: Browser is always embedded as a native child

*For any* invocation of `createBrowser()`, the `CefWindowInfo` passed to `CefBrowserHost::CreateBrowser()` SHALL have been configured via `SetAsChild()` with a non-null native window handle derived from `m_container->winId()`, so that the resulting browser window is a child of the container rather than a child of the desktop root window.

**Validates: Requirements 1.3, 2.2, 2.3**

### Property 2: Geometry rect matches container dimensions

*For any* container geometry `(w, h)` with `w > 0` and `h > 0`, the `cef_rect_t` passed to `SetAsChild()` SHALL have `width == w` and `height == h`, so that the CEF viewport is sized to exactly fill the container at creation time.

**Validates: Requirements 3.1, 3.2**

### Property 3: URL pass-through is lossless

*For any* URL string `u` entered in the URL bar, when the user presses Return and `GetBrowser()` is non-null, the string passed to `LoadURL()` SHALL be identical to `u` (modulo the lossless `QString::toStdString()` UTF-8 encoding), with no truncation, transformation, or re-encoding applied by the wiring code.

**Validates: Requirements 4.1, 4.2**

### Property 4: Browser lifecycle round-trip

*For any* `CefBrowser` instance `b`:
- After `OnAfterCreated(b)` is called, `GetBrowser()` SHALL return `b`.
- After `OnBeforeClose(b)` is called on the same handler, `GetBrowser()` SHALL return null.

**Validates: Requirements 6.1, 6.2, 6.4**

---

## Error Handling

### Browser not yet created

If the user presses Enter in the URL bar before `OnAfterCreated` has fired (i.e., `m_handler->GetBrowser()` returns null), the navigation lambda does nothing. No crash, no assertion, no queued navigation. The user must wait for the browser to finish initialising and then re-enter the URL.

### CEF initialisation failure

If `CefInitialize()` returns false (missing CEF binaries, sandbox error, etc.), `main()` returns 1 immediately. No Qt window is shown.

### Sub-process early exit

`CefExecuteProcess()` is called before `QApplication` is constructed. CEF renderer, GPU, and network sub-processes call this and exit with the returned code. The browser process receives `exit_code == -1` and continues normally.

### Cache directory unavailable

`QStandardPaths::writableLocation` returns an empty string on some systems. In that case `root_cache_path` is set to `"/lilypad"`, which CEF will attempt to create. If it fails, CEF logs a warning but continues without a cache.

### Zero-size container

If `createBrowser()` is somehow called before the layout has been processed (e.g., `singleShot` fires before `show()`), `m_container->geometry()` may return a zero-size rect. CEF will create the browser at zero size; it will not be visible. The `QTimer::singleShot(0, ...)` deferral is the mitigation — it guarantees at least one event-loop tick separates `show()` from `createBrowser()`.

---

## Testing Strategy

### Approach

The feature is a C++ application with a specific startup sequence and source-level structural requirements. The most practical testing approach is **static source analysis** (Python scripts that parse the C++ source) combined with **property-based sampling** over the input domains that vary (URL strings, geometry values).

This matches the existing test suite pattern in `tests/preservation/` and `tests/fix_checking/`.

### Unit Tests (static analysis)

Each test reads the relevant `.cpp` / `.h` file and asserts structural properties:

| Test | File | What it checks |
|---|---|---|
| `test_unit_wa_native_window` | `mainwindow.cpp` | `WA_NativeWindow` set on `m_container` in constructor |
| `test_unit_set_as_child` | `mainwindow.cpp` | `SetAsChild(m_container->winId(), rect)` called in `createBrowser()` |
| `test_unit_geometry_rect` | `mainwindow.cpp` | `cef_rect_t` built from `m_container->geometry()` |
| `test_unit_single_shot_defer` | `main.cpp` | `QTimer::singleShot(0, ...)` wraps `createBrowser()` call |
| `test_unit_cef_message_loop` | `main.cpp` | `QTimer` at ≤ 16 ms drives `CefDoMessageLoopWork()` |
| `test_unit_cef_shutdown` | `main.cpp` | `CefShutdown()` called exactly once after `qapp.exec()` |
| `test_unit_null_guard` | `mainwindow.cpp` | `GetBrowser()` null-check before `LoadURL()` |
| `test_unit_browser_lifecycle` | `cef_handler.cpp` | `OnAfterCreated` stores, `OnBeforeClose` clears `m_browser` |

### Property-Based Tests

Property-based tests use Python's `random` module (no external PBT library required, matching the existing test style) to generate inputs and verify the properties defined above.

**Property 3 — URL pass-through (100+ samples)**

Generate random URL strings (empty, ASCII, Unicode, very long, special characters, non-URL strings). For each:
- Verify structurally that `LoadURL` receives `m_urlBar->text().toStdString()` without intermediate mutation.
- Verify that `QString::toStdString()` (simulated as Python UTF-8 encode/decode) is a lossless round-trip.

Tag: `Feature: cef-qt-window-embedding, Property 3: URL pass-through is lossless`

**Property 2 — Geometry rect matches container (100+ samples)**

Generate random `(width, height)` pairs with `width > 0` and `height > 0`. For each:
- Verify structurally that the `cef_rect_t` construction uses `m_container->geometry().width()` and `m_container->geometry().height()` directly.
- Simulate the rect construction and assert `rect.width == w` and `rect.height == h`.

Tag: `Feature: cef-qt-window-embedding, Property 2: Geometry rect matches container dimensions`

**Property 4 — Browser lifecycle round-trip (100+ samples)**

Generate random sequences of `OnAfterCreated` / `OnBeforeClose` calls (simulated via static analysis of the handler logic). For each:
- After `OnAfterCreated`: assert `GetBrowser()` is non-null.
- After `OnBeforeClose`: assert `GetBrowser()` is null.

Tag: `Feature: cef-qt-window-embedding, Property 4: Browser lifecycle round-trip`

### Integration Tests

Full integration tests (launching the binary and inspecting the window tree) are out of scope for the automated test suite due to the requirement for a display server and CEF binaries. Manual verification covers:

- Application launches with a single window.
- Browser renders `https://polli.page` inside the Qt window.
- URL bar navigation works.
- Application exits cleanly without crashes.

### Test Configuration

- Minimum 100 iterations per property-based test.
- Tests run from the repository root: `python tests/.../<test_file>.py`.
- Source file paths configurable via environment variables (`LILYPAD_SOURCE`, `LILYPAD_MAINWINDOW`, `LILYPAD_HANDLER`).
- No external Python dependencies beyond the standard library.
