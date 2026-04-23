# Implementation Plan: CEF-Qt Window Embedding

## Overview

Implement the Lilypad application's CEF browser embedding inside a Qt6 `QMainWindow`. The implementation follows a specific startup sequence: Qt builds the window and layout, a deferred `QTimer::singleShot(0, ...)` call triggers browser creation after geometry is valid, and a 10 ms `QTimer` drives `CefDoMessageLoopWork()` to bridge the two event loops. All code is C++ using Qt6 and the CEF API.

## Tasks

- [x] 1. Implement `MainWindow` constructor with native container widget
  - [x] 1.1 Set up `MainWindow` constructor with URL bar and container widget
    - Create `QVBoxLayout` on the central widget with `m_urlBar` (`QLineEdit`) on top and `m_container` (`QWidget`) below, filling remaining space
    - Set `Qt::WA_NativeWindow` attribute on `m_container` so `winId()` returns a valid native handle before `createBrowser()` is called
    - Set `Qt::StrongFocus` focus policy on `m_container` so CEF can receive keyboard events
    - _Requirements: 1.2, 2.1, 2.4_

  - [x] 1.2 Write unit test for `WA_NativeWindow` attribute and layout structure
    - Static analysis of `mainwindow.cpp`: assert `WA_NativeWindow` is set on `m_container` in the constructor
    - Assert `m_urlBar` and `m_container` are both added to a vertical layout
    - _Requirements: 2.1, 2.4_

- [x] 2. Implement `MainWindow::createBrowser()` with `SetAsChild()` embedding
  - [x] 2.1 Implement `createBrowser()` to embed the CEF browser as a native child
    - Instantiate `CefHandler` and store it in `m_handler`
    - Read `m_container->geometry()` to obtain current pixel dimensions
    - Build a `cef_rect_t{0, 0, width, height}` from those dimensions
    - Call `window_info.SetAsChild(static_cast<CefWindowHandle>(m_container->winId()), rect)` to reparent the CEF window under the container
    - Call `CefBrowserHost::CreateBrowser()` with the configured `window_info`, `m_handler`, the URL, and default `CefBrowserSettings`
    - _Requirements: 2.2, 2.3, 3.1, 3.2_

  - [x] 2.2 Write property test for geometry rect matching container dimensions (Property 2)
    - **Property 2: Geometry rect matches container dimensions**
    - **Validates: Requirements 3.1, 3.2**
    - Generate 100+ random `(width, height)` pairs with `w > 0` and `h > 0`
    - For each pair, verify structurally (via static analysis of `mainwindow.cpp`) that `cef_rect_t` is built from `m_container->geometry().width()` and `m_container->geometry().height()` directly
    - Simulate rect construction in Python and assert `rect.width == w` and `rect.height == h`

  - [x] 2.3 Write unit test for `SetAsChild()` call with `winId()`
    - Static analysis of `mainwindow.cpp`: assert `SetAsChild()` is called in `createBrowser()` with `m_container->winId()` as the first argument
    - Assert a `cef_rect_t` is constructed from `m_container->geometry()` and passed to `SetAsChild()`
    - _Requirements: 2.2, 2.3_

- [x] 3. Implement URL bar navigation with null-guard
  - [x] 3.1 Connect `returnPressed` signal to navigation lambda in `MainWindow` constructor
    - Connect `m_urlBar->returnPressed` to a lambda that calls `m_handler->GetBrowser()->GetMainFrame()->LoadURL(m_urlBar->text().toStdString())`
    - Guard the call: only invoke `LoadURL()` when `m_handler->GetBrowser()` returns a non-null pointer
    - _Requirements: 4.1, 4.2, 4.3, 6.3_

  - [x] 3.2 Write property test for URL pass-through losslessness (Property 3)
    - **Property 3: URL pass-through is lossless**
    - **Validates: Requirements 4.1, 4.2**
    - Generate 100+ random URL strings (empty, ASCII, Unicode, very long, special characters, non-URL strings)
    - Verify structurally that `LoadURL` receives `m_urlBar->text().toStdString()` without intermediate mutation
    - Verify that `QString::toStdString()` (simulated as Python UTF-8 encode/decode) is a lossless round-trip for each sample

  - [x] 3.3 Write unit test for null-guard before `LoadURL()`
    - Static analysis of `mainwindow.cpp`: assert that `GetBrowser()` is checked for null before `LoadURL()` is called in the `returnPressed` lambda
    - _Requirements: 4.3, 6.3_

- [x] 4. Implement `CefHandler` browser lifecycle management
  - [x] 4.1 Implement `OnAfterCreated()` and `OnBeforeClose()` in `CefHandler`
    - In `OnAfterCreated()`, store the `CefBrowser` reference in `m_browser`
    - In `OnBeforeClose()`, clear `m_browser` to null when the browser being closed matches the stored instance
    - Expose `GetBrowser()` accessor returning the current `m_browser` value (null if no browser is live)
    - _Requirements: 6.1, 6.2, 6.4_

  - [x] 4.2 Write property test for browser lifecycle round-trip (Property 4)
    - **Property 4: Browser lifecycle round-trip**
    - **Validates: Requirements 6.1, 6.2, 6.4**
    - Generate 100+ simulated sequences of `OnAfterCreated` / `OnBeforeClose` calls
    - After each `OnAfterCreated`: assert `GetBrowser()` is non-null
    - After each `OnBeforeClose`: assert `GetBrowser()` is null
    - Verify via static analysis of `cef_handler.cpp` that `m_browser` is assigned in `OnAfterCreated` and cleared in `OnBeforeClose`

  - [x] 4.3 Write unit test for `CefHandler` lifecycle methods
    - Static analysis of `cef_handler.cpp`: assert `OnAfterCreated` stores the browser reference and `OnBeforeClose` clears it
    - Assert `GetBrowser()` accessor is present and returns `m_browser`
    - _Requirements: 6.1, 6.2, 6.4_

- [x] 5. Checkpoint — Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [x] 6. Implement `main()` startup sequence with deferred browser creation and CEF event loop
  - [x] 6.1 Implement `QCefApp` and `CefInitialize` setup in `main()`
    - Define `QCefApp` as a minimal `CefApp` / `CefBrowserProcessHandler` subclass
    - In `OnBeforeCommandLineProcessing()`, append GPU-disabling switches (`disable-gpu`, `disable-software-rasterizer`, etc.)
    - Call `CefExecuteProcess()` first so CEF sub-processes exit early
    - Configure `CefSettings` with `no_sandbox = true` and `root_cache_path` set to `QStandardPaths::CacheLocation + "/lilypad"`
    - Call `CefInitialize()` and return 1 on failure
    - _Requirements: 5.1, 5.3_

  - [x] 6.2 Implement deferred `createBrowser()` call via `QTimer::singleShot(0, ...)`
    - After `w.show()`, call `QTimer::singleShot(0, [&w](){ w.createBrowser("https://polli.page"); })` to defer browser creation by one event-loop tick
    - This guarantees `m_container` has non-zero geometry when `createBrowser()` reads it
    - _Requirements: 3.3, 3.4_

  - [x] 6.3 Implement `QTimer`-driven `CefDoMessageLoopWork()` at 10 ms interval
    - Declare a `QTimer cef_timer` in `main()`
    - Connect `cef_timer`'s `timeout` signal to a lambda that calls `CefDoMessageLoopWork()`
    - Start the timer with `cef_timer.start(10)` (≤ 16 ms, satisfying the 60 fps budget)
    - _Requirements: 5.1, 5.2_

  - [x] 6.4 Implement `CefShutdown()` after `qapp.exec()` returns
    - Call `CefShutdown()` exactly once, unconditionally, after `qapp.exec()` returns and before `main()` returns
    - _Requirements: 5.3_

  - [x] 6.5 Write unit test for `QTimer::singleShot(0, ...)` deferral of `createBrowser()`
    - Static analysis of `main.cpp`: assert `QTimer::singleShot(0, ...)` wraps the `createBrowser()` call
    - Assert the `singleShot` call appears after `w.show()`
    - _Requirements: 3.3, 3.4_

  - [x] 6.6 Write unit test for `CefDoMessageLoopWork()` timer at ≤ 16 ms
    - Static analysis of `main.cpp`: assert a `QTimer` is connected to a lambda calling `CefDoMessageLoopWork()`
    - Assert the timer is started with an interval ≤ 16 ms (expected: 10 ms)
    - Assert `CefDoMessageLoopWork()` is called only inside the timer callback, not as a bare call elsewhere in `main()`
    - _Requirements: 5.1, 5.2_

  - [x] 6.7 Write unit test for `CefShutdown()` called exactly once after `qapp.exec()`
    - Static analysis of `main.cpp`: assert `CefShutdown()` appears exactly once in `main()`
    - Assert `CefShutdown()` appears after `qapp.exec()` in source order
    - Assert `CefShutdown()` is not inside a conditional block that could cause it to be skipped
    - _Requirements: 5.3_

- [x] 7. Checkpoint — Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

- [x] 8. Wire all components together and verify single-window behaviour
  - [x] 8.1 Verify `MainWindow` header exposes the correct interface
    - Confirm `mainwindow.h` declares `m_container` (`QWidget*`), `m_urlBar` (`QLineEdit*`), `m_handler` (`CefHandler*`), and `createBrowser(const QString&)`
    - Confirm `cef_handler.h` declares `GetBrowser()`, `OnAfterCreated()`, and `OnBeforeClose()`
    - _Requirements: 2.1, 6.4_

  - [x] 8.2 Write unit test for single top-level window (Property 1)
    - **Property 1: Browser is always embedded as a native child**
    - **Validates: Requirements 1.1, 1.3, 2.2, 2.3**
    - Static analysis of `mainwindow.cpp` and `main.cpp`: assert no additional `QMainWindow` or top-level `QWidget::show()` calls exist outside of `MainWindow`
    - Assert `SetAsChild()` is used (not `SetAsPopup()` or any other mode that would create a floating window)

- [x] 9. Final checkpoint — Ensure all tests pass
  - Ensure all tests pass, ask the user if questions arise.

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP
- Each task references specific requirements for traceability
- All tests use static source analysis (Python, no external dependencies), matching the existing test suite pattern in `tests/preservation/` and `tests/fix_checking/`
- Property tests use Python's `random` module with a fixed seed for reproducibility (minimum 100 iterations)
- Source file paths are configurable via environment variables (`LILYPAD_SOURCE`, `LILYPAD_MAINWINDOW`, `LILYPAD_HANDLER`)
- Run tests from the repository root: `python tests/.../<test_file>.py`
