# CEF Network Crash Fix — Tasks

## Task List

- [x] 1. Exploratory testing on unfixed code
  - [x] 1.1 Write a test/script that launches the unfixed binary and counts top-level windows; assert ≥ 2 windows appear (confirms Bug 1 — missing `CefExecuteProcess()`)
  - [x] 1.2 Write a test/script that monitors stderr for `network_service_instance_impl.cc:608` for 5 seconds; assert ≥ 1 occurrence per second (confirms Bug 2 — `in-process-gpu` conflict)
  - [x] 1.3 Write a test/script that checks `m_container->geometry()` at the point `createBrowser()` is called; assert width and height are 0 (confirms Bug 3 — premature geometry read)
  - [x] 1.4 Write a test/script that monitors stderr for `resource_util.cc:83`; assert it appears on launch (confirms Bug 4 — missing `root_cache_path`)

- [x] 2. Fix `src/main.cpp` — Bug 1: Add `CefExecuteProcess()` guard
  - [x] 2.1 Insert `CefExecuteProcess()` call immediately after `CefMainArgs main_args(argc, argv);`, before any Qt or application code, and return its exit code when it is ≥ 0

- [x] 3. Fix `src/main.cpp` — Bug 2: Remove conflicting `in-process-gpu` switch
  - [x] 3.1 Delete `command_line->AppendSwitch("in-process-gpu");` from `OnBeforeCommandLineProcessing`

- [x] 4. Fix `src/main.cpp` — Bug 4: Set `CefSettings.root_cache_path`
  - [x] 4.1 Add `#include <QStandardPaths>` to `main.cpp`
  - [x] 4.2 After `settings.no_sandbox = true;`, set `root_cache_path` to `QStandardPaths::CacheLocation + "/lilypad"` using `CefString::FromString`

- [x] 5. Fix `src/main.cpp` — Bug 3: Defer `createBrowser()` until geometry is valid
  - [x] 5.1 Replace the synchronous `w.createBrowser("https://polli.page")` call with a `QTimer::singleShot(0, ...)` lambda that calls `w.createBrowser("https://polli.page")` after the first event-loop iteration

- [x] 6. Fix checking — verify all four bugs are resolved
  - [x] 6.1 Write a unit test asserting that `OnBeforeCommandLineProcessing` does not append `in-process-gpu` to the command line
  - [x] 6.2 Write a unit test asserting that `CefSettings.root_cache_path` is set to a non-empty, application-specific path
  - [x] 6.3 Write an integration test that builds and runs the fixed binary, asserts exactly one top-level window is created, and asserts zero `network_service_instance_impl.cc:608` occurrences in stderr over 5 seconds
  - [x] 6.4 Write an integration test that waits for `CefHandler::OnAfterCreated` and asserts the container geometry has non-zero width and height
  - [x] 6.5 Write an integration test that asserts zero `resource_util.cc:83` occurrences in stderr after launch

- [x] 7. Preservation checking — verify unchanged behaviours
  - [x] 7.1 Write a property-based test (Property 2) that, for any arbitrary URL string, asserts `LoadURL` is called on the browser's main frame with that exact URL when the user presses Enter in the address bar
  - [x] 7.2 Write a unit test asserting `CefShutdown()` is called exactly once after `qapp.exec()` returns and that the process exits with code 0
  - [x] 7.3 Write a unit test asserting `CefDoMessageLoopWork()` is called on every Qt timer tick at the 10 ms interval
  - [x] 7.4 Write a unit test asserting the browser is embedded as a native child of `m_container` using the correct `winId()` and a non-zero geometry rect

- [x] 8. Build verification
  - [x] 8.1 Run `cmake --build build` (or equivalent) and confirm zero errors and zero new warnings introduced by the changes
