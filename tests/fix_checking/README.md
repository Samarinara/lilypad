# Fix-Checking Test Suite

This directory contains tests that verify the four CEF initialisation bugs in
`src/main.cpp` have been correctly fixed. They are the counterpart to the
exploratory tests in `tests/exploratory/`, which confirmed the bugs existed on
the unfixed code.

---

## Background

The Lilypad CEF+Qt6 app had four interrelated startup bugs:

| # | Bug | Root cause |
|---|-----|------------|
| 1 | Extra blank window on launch | `CefExecuteProcess()` not called before `QApplication` |
| 2 | Network service crash loop | `in-process-gpu` contradicts `disable-gpu` family switches |
| 3 | CEF container renders at zero size | `createBrowser()` called before Qt layout is processed |
| 4 | Repeated `root_cache_path` warning | `CefSettings.root_cache_path` left empty |

All four fixes are in `src/main.cpp`. No changes were required to
`src/mainwindow.cpp` or `src/cef_handler.cpp`.

---

## Tests

### Unit Tests (static source analysis — no binary required)

| File | Task | What it checks |
|------|------|----------------|
| `test_unit_no_inprocess_gpu.py` | 6.1 | `OnBeforeCommandLineProcessing` does **not** append `in-process-gpu`; `disable-gpu` family switches are still present |
| `test_unit_root_cache_path.py` | 6.2 | `CefSettings.root_cache_path` is assigned a non-empty, application-specific path using `QStandardPaths` |

These tests parse `src/main.cpp` directly and require no compiled binary or
running display. They are fast and suitable for CI.

### Integration Tests (require a built binary and X display)

| File | Task | What it checks |
|------|------|----------------|
| `test_integration_launch.py` | 6.3 | Exactly 1 new top-level window; zero `network_service_instance_impl.cc:608` occurrences in stderr over 5 s |
| `test_integration_geometry.py` | 6.4 | Main window has non-zero width and height after `OnAfterCreated` fires (3 s wait) |
| `test_integration_no_cache_warning.py` | 6.5 | Zero `resource_util.cc:83` occurrences in stderr over 5 s; also checks source statically |

Integration tests **gracefully skip** (exit 0 with an informative message) when:
- The binary is not found at `./build/lilypad`.
- `wmctrl` / `xdotool` is not installed.
- `DISPLAY` is not set.

---

## Running the Tests

### Prerequisites

```bash
# Build the binary
cmake -B build -S .
cmake --build build

# Install window-management tools (for integration tests)
sudo apt install wmctrl xdotool

# Start a virtual display if running headless
Xvfb :99 -screen 0 1280x1024x24 &
export DISPLAY=:99
```

### Run all fix-checking tests

```bash
# From the repo root
python3 tests/fix_checking/test_unit_no_inprocess_gpu.py
python3 tests/fix_checking/test_unit_root_cache_path.py
python3 tests/fix_checking/test_integration_launch.py
python3 tests/fix_checking/test_integration_geometry.py
python3 tests/fix_checking/test_integration_no_cache_warning.py
```

### Run only the static analysis tests (no binary needed)

```bash
python3 tests/fix_checking/test_unit_no_inprocess_gpu.py
python3 tests/fix_checking/test_unit_root_cache_path.py
```

### Override paths via environment variables

```bash
export LILYPAD_BINARY=/path/to/lilypad
export LILYPAD_SOURCE=/path/to/src/main.cpp
python3 tests/fix_checking/test_integration_launch.py
```

---

## Expected Results on Fixed Code

All five tests should exit with code **0** (PASS) on the fixed source.

| Test | Expected exit code |
|------|--------------------|
| `test_unit_no_inprocess_gpu.py` | 0 (PASS) |
| `test_unit_root_cache_path.py` | 0 (PASS) |
| `test_integration_launch.py` | 0 (PASS or SKIP) |
| `test_integration_geometry.py` | 0 (PASS or SKIP) |
| `test_integration_no_cache_warning.py` | 0 (PASS or SKIP) |

---

## Relationship to Exploratory Tests

The exploratory tests in `tests/exploratory/` were designed to **fail** on
unfixed code (confirming the bugs exist). These fix-checking tests are designed
to **pass** on fixed code (confirming the bugs are resolved). Together they
provide a complete before/after verification of the fix.
