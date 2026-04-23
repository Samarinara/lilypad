# CEF Network Crash Fix — Bugfix Design

## Overview

The Lilypad CEF+Qt6 desktop browser app fails to start correctly due to four interrelated bugs in `src/main.cpp` and `src/mainwindow.cpp`. The bugs are:

1. **Missing `CefExecuteProcess()`** — CEF sub-processes re-execute the binary and fall through to `QApplication`, spawning a second blank window.
2. **`in-process-gpu` conflicts with `disable-gpu` family** — contradictory command-line switches destabilise the Chromium network service, causing it to crash and restart in a tight loop.
3. **`createBrowser()` called before widget geometry is valid** — the container widget has not been laid out when `createBrowser` is called immediately after `show()`, so the browser is embedded at zero size and the page never renders.
4. **`CefSettings.root_cache_path` not set** — CEF falls back to a shared default path and emits a repeated warning about potential process-singleton conflicts.

The fix is surgical: add the missing `CefExecuteProcess()` guard, remove the conflicting `in-process-gpu` switch, defer `createBrowser()` until after the event loop has processed the first paint (via `QTimer::singleShot`), and set `root_cache_path` to an application-specific directory.

---

## Glossary

- **Bug_Condition (C)**: The set of runtime conditions that trigger one or more of the four bugs described above.
- **Property (P)**: The desired observable behaviour when the application launches correctly — network service stable, page rendered, single window, no warning.
- **Preservation**: Existing behaviours that must remain unchanged after the fix — URL navigation, clean shutdown, CEF message-loop pumping, and correct browser embedding geometry.
- **`CefExecuteProcess()`**: CEF API that must be called at the very start of `main()` so that CEF sub-process invocations (renderer, GPU, network) exit early without running Qt code.
- **`CefDoMessageLoopWork()`**: CEF API called periodically from the Qt timer to give CEF a timeslice within the Qt event loop.
- **`QTimer::singleShot`**: Qt one-shot timer used to defer `createBrowser()` until after the Qt event loop has processed the initial layout and paint, ensuring valid widget geometry.
- **`in-process-gpu`**: A Chromium command-line switch that runs the GPU process in-process; it directly contradicts `disable-gpu` and related switches, destabilising the network service.
- **`root_cache_path`**: A `CefSettings` field that specifies the root directory for CEF's cache files; leaving it empty triggers a warning and may cause process-singleton conflicts.
- **`m_container`**: The `QWidget` in `MainWindow` that hosts the native CEF browser window as a child.

---

## Bug Details

### Bug Condition

The four bugs all manifest at application launch. They are triggered by the combination of missing API calls, a conflicting command-line switch, a premature geometry read, and a missing settings field.

**Formal Specification:**

```
FUNCTION isBugCondition(launchContext)
  INPUT: launchContext — the state of main() at application startup
  OUTPUT: boolean

  bug1 := CefExecuteProcess() NOT called before QApplication construction
  bug2 := command_line contains "in-process-gpu"
            AND command_line contains any of
              {"disable-gpu", "disable-gpu-compositing",
               "disable-gpu-rasterization", "disable-software-rasterizer"}
  bug3 := createBrowser() called synchronously after show()
            WITHOUT waiting for Qt event loop to process layout
  bug4 := CefSettings.root_cache_path IS empty string

  RETURN bug1 OR bug2 OR bug3 OR bug4
END FUNCTION
```

### Examples

| # | Trigger | Current (buggy) behaviour | Expected (correct) behaviour |
|---|---------|--------------------------|------------------------------|
| 1 | App launches; CEF spawns renderer sub-process | Sub-process re-enters `main()`, constructs `QApplication`, opens a second blank window | Sub-process calls `CefExecuteProcess()`, gets non-zero return, exits immediately |
| 2 | App launches; `OnBeforeCommandLineProcessing` runs | Both `disable-gpu` and `in-process-gpu` are appended; network service crashes every ~1 s | Only `disable-gpu` family switches present; network service starts once and stays up |
| 3 | `w.show()` returns; `createBrowser()` is called | `m_container->geometry()` returns `QRect(0,0,0,0)`; CEF browser embedded at zero size; page blank | `createBrowser()` deferred via `QTimer::singleShot(0, ...)`; geometry is valid; page loads |
| 4 | `CefInitialize()` called with empty `root_cache_path` | Warning logged repeatedly: `Please customize CefSettings.root_cache_path` | `root_cache_path` set to `~/.cache/lilypad`; warning never appears |

---

## Expected Behavior

### Preservation Requirements

**Unchanged Behaviours:**
- URL navigation via the address bar (`QLineEdit::returnPressed` → `LoadURL`) must continue to work exactly as before.
- CEF must continue to shut down cleanly via `CefShutdown()` when the Qt event loop exits, without crashes or resource leaks.
- The Qt timer must continue to call `CefDoMessageLoopWork()` at the configured 10 ms interval so CEF remains responsive.
- The CEF browser must continue to be embedded as a native child of `m_container` at the correct geometry.

**Scope:**
All runtime behaviour that does NOT involve the four bug conditions above must be completely unaffected by this fix. This includes:
- Mouse and keyboard interaction with the embedded browser after it has loaded.
- The address bar widget and its navigation signal/slot connection.
- The `CefHandler` lifecycle callbacks (`OnAfterCreated`, `OnBeforeClose`).
- The Qt window title, size, and layout structure.

---

## Hypothesized Root Cause

1. **Missing `CefExecuteProcess()` guard** (`main.cpp`, top of `main()`):
   CEF's multi-process architecture re-invokes the host binary for each sub-process (renderer, GPU, network). Without an early `CefExecuteProcess()` call, sub-process invocations fall through to `QApplication` construction and `MainWindow::show()`, producing extra blank windows and corrupting the process model.

2. **Contradictory GPU command-line switches** (`main.cpp`, `OnBeforeCommandLineProcessing`):
   `in-process-gpu` instructs Chromium to run the GPU thread inside the browser process, while `disable-gpu` and its companions tell it not to use GPU at all. The contradiction leaves the network service in an undefined state; Chromium's sandbox/process manager detects the crash and restarts it in a loop.

3. **Premature `createBrowser()` call** (`main.cpp`, after `w.show()`):
   `QWidget::show()` posts a paint event but does not process it synchronously. `m_container->geometry()` therefore returns `QRect(0,0,0,0)` at the point `createBrowser()` reads it. CEF embeds the browser at zero size; the page is never rendered visibly.

4. **Empty `CefSettings.root_cache_path`** (`main.cpp`, settings block):
   CEF's default cache path is a shared system location. When multiple CEF-based applications use it, process-singleton detection can misfire. CEF warns about this at startup and repeats the warning on every relevant operation.

---

## Correctness Properties

Property 1: Bug Condition — CEF Initialisation Sequence Is Correct

_For any_ application launch where all four bug conditions hold (isBugCondition returns true for the current `main.cpp`), the fixed `main()` SHALL:
- call `CefExecuteProcess()` before constructing `QApplication`, causing sub-processes to exit early and preventing extra windows;
- omit `in-process-gpu` from the command line, allowing the network service to start once and remain stable;
- defer `createBrowser()` until after the Qt event loop has processed the initial layout, ensuring valid container geometry and a rendered page;
- set `CefSettings.root_cache_path` to an application-specific directory, suppressing the repeated warning.

**Validates: Requirements 2.1, 2.2, 2.3, 2.4**

Property 2: Preservation — Non-Buggy Runtime Behaviour Is Unchanged

_For any_ runtime interaction that does NOT involve the four bug conditions (isBugCondition returns false — i.e., normal post-launch usage), the fixed code SHALL produce exactly the same behaviour as the original code, preserving URL navigation, clean shutdown, CEF message-loop pumping, and correct browser embedding.

**Validates: Requirements 3.1, 3.2, 3.3, 3.4**

---

## Fix Implementation

### Changes Required

**File: `src/main.cpp`**

**Change 1 — Add `CefExecuteProcess()` guard at the very top of `main()`**

Insert immediately after `CefMainArgs main_args(argc, argv);`:

```cpp
// Let CEF sub-processes (renderer, GPU, network) exit early.
int exit_code = CefExecuteProcess(main_args, nullptr, nullptr);
if (exit_code >= 0)
    return exit_code;
```

This must appear before any Qt or application code so that sub-process invocations never reach `QApplication`.

**Change 2 — Remove `in-process-gpu` switch from `OnBeforeCommandLineProcessing`**

Delete the line:
```cpp
command_line->AppendSwitch("in-process-gpu");
```

The remaining `disable-gpu` family switches are consistent with each other and with headless/offscreen rendering; removing the contradictory switch stabilises the network service.

**Change 3 — Set `CefSettings.root_cache_path` to an application-specific directory**

After `settings.no_sandbox = true;`, add:

```cpp
// Use an app-specific cache directory to avoid process-singleton warnings.
const std::string cache_dir =
    QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
        .toStdString() + "/lilypad";
CefString(&settings.root_cache_path).FromString(cache_dir);
```

Requires `#include <QStandardPaths>`.

**Change 4 — Defer `createBrowser()` until after the first Qt event-loop iteration**

Replace:
```cpp
w.show();
w.createBrowser("https://polli.page");
```

With:
```cpp
w.show();
QTimer::singleShot(0, [&w](){
    w.createBrowser("https://polli.page");
});
```

`QTimer::singleShot(0, ...)` posts a zero-delay callback that fires after the event loop has processed the pending show/layout/paint events, guaranteeing that `m_container->geometry()` returns a non-zero rect.

No changes are required to `src/mainwindow.cpp` or `src/cef_handler.cpp`.

---

## Testing Strategy

### Validation Approach

Testing follows a two-phase approach:

1. **Exploratory / bug-condition checking** — run tests against the *unfixed* code to confirm the bugs are reproducible and to validate the root-cause hypotheses.
2. **Fix checking + preservation checking** — run the same tests (and additional preservation tests) against the *fixed* code to confirm all four bugs are resolved and no regressions are introduced.

Because the bugs are startup-time initialisation issues, the primary test vehicle is a set of unit/integration tests that mock or inspect the CEF and Qt initialisation sequence. Property-based tests are used for preservation checking of the URL-navigation and message-loop-pumping paths.

---

### Exploratory Bug Condition Checking

**Goal**: Surface counterexamples that demonstrate each bug on the *unfixed* code. Confirm or refute the root-cause hypotheses. If refuted, re-hypothesize before implementing the fix.

**Test Plan**: Compile and run the application with the current (unfixed) `main.cpp`. Capture stderr output and observe window count.

**Test Cases**:

1. **Sub-process window test** (Bug 1): Launch the binary and count the number of top-level windows created. On unfixed code, expect ≥ 2 windows. *(will fail on unfixed code)*
2. **Network service stability test** (Bug 2): Launch the binary and monitor stderr for `network_service_instance_impl.cc:608` for 5 seconds. On unfixed code, expect ≥ 1 occurrence per second. *(will fail on unfixed code)*
3. **Page render test** (Bug 3): Launch the binary, wait 3 seconds, and check whether the CEF container has non-zero size and a rendered page. On unfixed code, expect blank/zero-size container. *(will fail on unfixed code)*
4. **Cache path warning test** (Bug 4): Launch the binary and monitor stderr for `resource_util.cc:83`. On unfixed code, expect repeated occurrences. *(will fail on unfixed code)*

**Expected Counterexamples**:
- Extra windows appear because sub-processes reach `QApplication` construction.
- Network service crash log repeats because `in-process-gpu` contradicts `disable-gpu`.
- Container geometry is `(0,0,0,0)` at the time `createBrowser()` reads it.
- `root_cache_path` warning appears in stderr on every launch.

---

### Fix Checking

**Goal**: Verify that for all inputs where the bug condition holds, the fixed code produces the expected correct behaviour.

**Pseudocode:**
```
FOR ALL launchContext WHERE isBugCondition(launchContext) DO
  result := launch_fixed_binary(launchContext)
  ASSERT windowCount(result) == 1
  ASSERT networkServiceCrashCount(result, duration=5s) == 0
  ASSERT containerGeometry(result).width > 0
        AND containerGeometry(result).height > 0
  ASSERT rootCachePathWarningCount(result) == 0
END FOR
```

**Test Cases**:
1. **Single window test**: After fix, launch binary and assert exactly one top-level window is created.
2. **Network service stability test**: After fix, monitor stderr for 5 seconds and assert zero `network_service_instance_impl.cc:608` occurrences.
3. **Page render test**: After fix, wait for `OnAfterCreated` callback and assert container geometry is non-zero.
4. **Cache path warning test**: After fix, assert zero `resource_util.cc:83` occurrences in stderr.

---

### Preservation Checking

**Goal**: Verify that for all inputs where the bug condition does NOT hold (normal post-launch usage), the fixed code produces the same behaviour as the original code.

**Pseudocode:**
```
FOR ALL interaction WHERE NOT isBugCondition(interaction) DO
  ASSERT behaviour_original(interaction) == behaviour_fixed(interaction)
END FOR
```

**Testing Approach**: Property-based testing is used for preservation checking because:
- It generates many random URL strings and interaction sequences automatically.
- It catches edge cases (empty URLs, very long URLs, special characters) that manual tests miss.
- It provides strong guarantees that navigation and shutdown behaviour is unchanged across the full input domain.

**Test Plan**: Observe the behaviour of URL navigation and shutdown on the unfixed code (these paths are not affected by the four bugs), then write property-based tests that assert the same behaviour holds after the fix.

**Test Cases**:
1. **URL navigation preservation**: For any valid URL string entered in the address bar, `LoadURL` is called on the browser's main frame with that URL — behaviour must be identical before and after the fix.
2. **Shutdown preservation**: `CefShutdown()` is called exactly once after `qapp.exec()` returns, with no intervening crashes — identical before and after the fix.
3. **Message-loop pumping preservation**: `CefDoMessageLoopWork()` is called at every timer tick at the 10 ms interval — identical before and after the fix.
4. **Browser embedding preservation**: After `createBrowser()` completes (on fixed code), the browser is embedded as a child of `m_container` with the correct `winId()` — same embedding logic as before.

---

### Unit Tests

- Test that `CefExecuteProcess()` is called before `QApplication` is constructed (inspect call order via mock or instrumented build).
- Test that `OnBeforeCommandLineProcessing` does not append `in-process-gpu` to the command line.
- Test that `CefSettings.root_cache_path` is set to a non-empty, application-specific path.
- Test that `createBrowser()` is not called synchronously in the same stack frame as `show()`.
- Test `CefHandler::OnAfterCreated` stores the browser reference correctly.
- Test `CefHandler::OnBeforeClose` clears the browser reference correctly.

### Property-Based Tests

- **Property 1 (Fix)**: For any simulated launch context where isBugCondition is true, the fixed initialisation sequence produces a single window, a stable network service, a non-zero container geometry, and no cache-path warning.
- **Property 2 (Preservation — navigation)**: For any arbitrary URL string `u`, calling `LoadURL(u)` on the browser's main frame after the fix produces the same call as before the fix (same argument, same frame).
- **Property 3 (Preservation — message loop)**: For any sequence of timer ticks, `CefDoMessageLoopWork()` is called exactly once per tick, unchanged by the fix.

### Integration Tests

- Full launch test: build and run the fixed binary; assert one window, no network-service crash log, page loads within 10 seconds, no cache-path warning.
- Navigation integration test: after launch, type a URL in the address bar, press Enter, and assert the browser navigates to the new URL.
- Shutdown integration test: close the main window and assert the process exits cleanly (exit code 0, no crash dump).
- Context-switch test (if applicable): verify that switching between URLs does not destabilise the network service or produce additional windows.
