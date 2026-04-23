# Exploratory Bug Condition Tests

This directory contains exploratory test scripts for the CEF+Qt6 Lilypad browser app.
These scripts are designed to **confirm the existence of four known bugs** in the unfixed
source code before any fixes are applied.

## Philosophy

These are **bug-condition exploration tests**. Their purpose is to surface counterexamples
that demonstrate each bug on the *unfixed* binary. A test that **FAILS** (i.e., the assertion
triggers) on the unfixed code is the **SUCCESS case** — it confirms the bug is present and
the root-cause hypothesis is correct.

After the fixes are applied, re-running these scripts should produce **PASS** results,
confirming the bugs are resolved.

## Prerequisites

- A built Lilypad binary at `../../build/lilypad` (relative to this directory), or set
  the `LILYPAD_BINARY` environment variable to the correct path.
- Python 3.8+
- For Bug 1 (window counting): `wmctrl` or `xdotool` installed
  - Ubuntu/Debian: `sudo apt install wmctrl xdotool`
- A running X display (or Xvfb for headless environments):
  - `sudo apt install xvfb`
  - `Xvfb :99 -screen 0 1280x1024x24 &`
  - `export DISPLAY=:99`

## Test Scripts

| Script | Bug | What it checks | Expected result on UNFIXED code |
|--------|-----|----------------|---------------------------------|
| `test_bug1_extra_window.py` | Bug 1 | Counts top-level windows after launch | FAIL — ≥ 2 windows appear (sub-processes open blank windows) |
| `test_bug2_network_crash.py` | Bug 2 | Monitors stderr for network service crash log | FAIL — crash message appears ≥ 1×/second |
| `test_bug3_geometry.py` | Bug 3 | Static source analysis: `createBrowser()` called synchronously after `show()` | FAIL — no `QTimer::singleShot` deferral found |
| `test_bug4_cache_warning.py` | Bug 4 | Monitors stderr for `resource_util.cc:83` warning | FAIL — warning appears on launch |

## Running the Tests

Run all four scripts from the repo root:

```bash
# Optional: set binary path explicitly
export LILYPAD_BINARY=./build/lilypad

# Run each script
python3 tests/exploratory/test_bug1_extra_window.py
python3 tests/exploratory/test_bug2_network_crash.py
python3 tests/exploratory/test_bug3_geometry.py
python3 tests/exploratory/test_bug4_cache_warning.py
```

Or run them all at once:

```bash
for f in tests/exploratory/test_bug*.py; do
    echo "=== $f ==="
    python3 "$f"
    echo ""
done
```

## Interpreting Results

- **EXIT CODE 1 / "BUG CONFIRMED"**: The bug is present in the current code. This is the
  expected outcome on unfixed code and confirms the root-cause hypothesis.
- **EXIT CODE 0 / "PASS"**: The bug is NOT present (either the code has been fixed, or the
  test could not reproduce the condition). After applying all four fixes, all scripts should
  exit with code 0.

## Bug Summary

### Bug 1 — Missing `CefExecuteProcess()`
CEF's multi-process architecture re-invokes the host binary for each sub-process (renderer,
GPU, network). Without an early `CefExecuteProcess()` call, sub-process invocations fall
through to `QApplication` construction and `MainWindow::show()`, producing extra blank windows.

**Fix**: Add `CefExecuteProcess()` guard at the very top of `main()`, before any Qt code.

### Bug 2 — `in-process-gpu` conflicts with `disable-gpu`
`in-process-gpu` instructs Chromium to run the GPU thread inside the browser process, while
`disable-gpu` and its companions tell it not to use GPU at all. The contradiction leaves the
network service in an undefined state; Chromium detects the crash and restarts it in a loop.

**Fix**: Remove `command_line->AppendSwitch("in-process-gpu")` from `OnBeforeCommandLineProcessing`.

### Bug 3 — `createBrowser()` called before widget geometry is valid
`QWidget::show()` posts a paint event but does not process it synchronously.
`m_container->geometry()` therefore returns `QRect(0,0,0,0)` at the point `createBrowser()`
reads it. CEF embeds the browser at zero size; the page is never rendered visibly.

**Fix**: Defer `createBrowser()` via `QTimer::singleShot(0, ...)`.

### Bug 4 — `CefSettings.root_cache_path` not set
CEF's default cache path is a shared system location. When left empty, CEF warns about
potential process-singleton conflicts on every launch.

**Fix**: Set `root_cache_path` to `~/.cache/lilypad` (or equivalent via `QStandardPaths`).
