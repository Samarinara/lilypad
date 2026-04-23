# Preservation Test Suite

This directory contains the **preservation tests** for the CEF network crash fix.

## Purpose

The preservation tests verify **Property 2** from the design document:

> *For any runtime interaction that does NOT involve the four bug conditions
> (isBugCondition returns false — i.e., normal post-launch usage), the fixed
> code SHALL produce exactly the same behaviour as the original code.*

In other words, these tests confirm that the four-bug fix did **not** introduce
any regressions in the behaviours that were already working correctly before the
fix was applied.

**Validates: Requirements 3.1, 3.2, 3.3, 3.4**

---

## Tests

| File | Task | Requirement | What it checks |
|------|------|-------------|----------------|
| `test_pbt_url_navigation.py` | 7.1 | 3.1 | URL navigation signal/slot wiring is preserved; `returnPressed` → `LoadURL` passes the URL unchanged across 100 random samples |
| `test_unit_cef_shutdown.py` | 7.2 | 3.2 | `CefShutdown()` is called exactly once, after `qapp.exec()`, and is not inside a conditional block |
| `test_unit_message_loop.py` | 7.3 | 3.3 | `CefDoMessageLoopWork()` is called at the 10 ms timer interval and only inside the timer callback |
| `test_unit_browser_embedding.py` | 7.4 | 3.4 | Browser is embedded as a native child of `m_container` via `winId()` + `SetAsChild()` with a geometry rect |

All four tests are **static source analysis** tests — they read and parse the
C++ source files directly without compiling or running the binary. This makes
them fast, deterministic, and runnable in any environment.

---

## Running the Tests

Run all preservation tests from the repository root:

```bash
python3 tests/preservation/test_pbt_url_navigation.py
python3 tests/preservation/test_unit_cef_shutdown.py
python3 tests/preservation/test_unit_message_loop.py
python3 tests/preservation/test_unit_browser_embedding.py
```

Each test exits with code `0` on PASS and `1` on FAIL.

To run all four at once and see a summary:

```bash
for t in tests/preservation/test_*.py; do
    echo "--- $t ---"
    python3 "$t"
    echo
done
```

### Environment Variables

Each test accepts environment variables to override the default source file
paths (useful for CI or out-of-tree builds):

| Variable | Default | Used by |
|----------|---------|---------|
| `LILYPAD_SOURCE` | `./src/main.cpp` | 7.2, 7.3 |
| `LILYPAD_MAINWINDOW` | `./src/mainwindow.cpp` | 7.1, 7.4 |
| `LILYPAD_HANDLER` | `./src/cef_handler.cpp` | 7.4 |

Example:

```bash
LILYPAD_SOURCE=/path/to/main.cpp python3 tests/preservation/test_unit_cef_shutdown.py
```

---

## Expected Results

All four tests should **PASS** on both the unfixed and fixed code, because the
preservation tests check behaviours that are **not affected by the four bugs**.
If any test fails on the fixed code, it indicates a regression introduced by
the fix.

| Test | Unfixed code | Fixed code |
|------|-------------|------------|
| 7.1 URL navigation | PASS | PASS |
| 7.2 CefShutdown | PASS | PASS |
| 7.3 Message loop | PASS | PASS |
| 7.4 Browser embedding | PASS | PASS |

---

## Approach

### Static Analysis

All tests parse the C++ source using Python's `re` module (no external
dependencies). The analysis strategy is:

1. **Extract function bodies** using brace counting from a regex-matched
   function signature.
2. **Strip comments** before analysis to avoid false positives from commented-out
   code.
3. **Search for structural patterns** (signal/slot connections, API calls,
   variable declarations) using targeted regular expressions.

### Property-Based Sampling (Test 7.1)

Test 7.1 additionally runs a sampling loop over 100 randomly generated URL
strings to verify the pass-through property:

> ∀ url ∈ URL_DOMAIN: the URL passed to `LoadURL` is identical to the URL
> entered in the address bar.

The sampling covers edge cases including empty strings, very long URLs, special
characters, unicode, and non-URL strings. The random seed is fixed (`42`) for
reproducibility.

No external libraries (e.g., `hypothesis`) are required — the sampling loop is
implemented in pure Python.

---

## Relationship to Other Test Suites

```
tests/
├── exploratory/      # Bug condition tests (run on UNFIXED code to confirm bugs)
├── fix_checking/     # Fix verification tests (run on FIXED code to confirm fixes)
└── preservation/     # Preservation tests (run on FIXED code to confirm no regressions)
```

The three suites together provide full coverage of the bugfix workflow:
- **Exploratory**: confirm the bugs exist on unfixed code.
- **Fix checking**: confirm the bugs are resolved on fixed code.
- **Preservation**: confirm no regressions were introduced by the fix.
