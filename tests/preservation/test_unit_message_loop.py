#!/usr/bin/env python3
"""
Preservation Unit Test — Task 7.3
===================================
Verifies that CefDoMessageLoopWork() is called at the 10 ms timer interval
and only inside the timer callback.

**Validates: Requirements 3.3**

WHAT THIS TESTS
---------------
Requirement 3.3 states:
  WHEN the application is running THEN the system SHALL CONTINUE TO pump the
  CEF message loop via the Qt timer at the configured interval so that CEF
  remains responsive.

The fix to main.cpp must not alter the message-loop pumping mechanism. This
test uses static source analysis to confirm:

  1. A QTimer is created in main() and connected to a slot/lambda that calls
     CefDoMessageLoopWork().
  2. The timer is started with interval 10 (milliseconds).
  3. CefDoMessageLoopWork() is called ONLY inside the timer callback — not
     elsewhere in main() (e.g., not in a bare call outside the timer).

EXPECTED RESULT ON FIXED CODE
------------------------------
  PASS — QTimer connected to CefDoMessageLoopWork(); started at 10 ms.

EXPECTED RESULT ON UNFIXED CODE
---------------------------------
  PASS — this path is not affected by the four bugs; it should pass on both
  unfixed and fixed code (preservation check).

REQUIRES
--------
  - src/main.cpp present at ./src/main.cpp (or LILYPAD_SOURCE env var)
  - Python 3.8+ (no external dependencies)
"""

import os
import re
import sys

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
SOURCE_FILE = os.environ.get("LILYPAD_SOURCE", "./src/main.cpp")
EXPECTED_INTERVAL_MS = 10

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load_source(path: str) -> str:
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def strip_line_comments(source: str) -> str:
    return re.sub(r'//[^\n]*', '', source)


def strip_block_comments(source: str) -> str:
    return re.sub(r'/\*.*?\*/', '', source, flags=re.DOTALL)


def strip_comments(source: str) -> str:
    return strip_line_comments(strip_block_comments(source))


def extract_main_body(source: str) -> str:
    """
    Extract the body of the main() function from source (without outer braces).
    """
    match = re.search(r'\bint\s+main\s*\(', source)
    if not match:
        return ""
    start = source.find('{', match.end())
    if start == -1:
        return ""
    depth = 0
    for i in range(start, len(source)):
        if source[i] == '{':
            depth += 1
        elif source[i] == '}':
            depth -= 1
            if depth == 0:
                return source[start + 1:i]
    return source[start + 1:]


def extract_lambda_body_at(text: str, pos: int) -> str:
    """
    Given a position in text that is at or after a '[' (lambda capture),
    extract the lambda body (the {...} part) by brace counting.
    Returns the body string (including braces), or empty string if not found.
    """
    # Find the opening brace of the lambda body
    brace_start = text.find('{', pos)
    if brace_start == -1:
        return ""
    depth = 0
    for i in range(brace_start, len(text)):
        if text[i] == '{':
            depth += 1
        elif text[i] == '}':
            depth -= 1
            if depth == 0:
                return text[brace_start:i + 1]
    return text[brace_start:]


# ---------------------------------------------------------------------------
# Assertions
# ---------------------------------------------------------------------------

def check_qtimer_created(main_body: str) -> dict:
    """
    Assert that a QTimer is declared/created in main().
    Accepts: QTimer cef_timer; or QTimer timer; or similar.
    """
    pattern = re.compile(r'\bQTimer\b\s+\w+\s*;')
    m = pattern.search(main_body)
    if m:
        return {
            "found": True,
            "detail": f"QTimer variable declared: {m.group(0).strip()!r}",
            "var_name": re.search(r'\bQTimer\b\s+(\w+)', m.group(0)).group(1),
        }
    # Also accept: new QTimer(...)
    m2 = re.search(r'\bnew\s+QTimer\b', main_body)
    if m2:
        return {
            "found": True,
            "detail": "QTimer created with 'new'.",
            "var_name": None,
        }
    return {
        "found": False,
        "detail": "No QTimer declaration found in main().",
        "var_name": None,
    }


def check_timer_connected_to_cef_work(main_body: str) -> dict:
    """
    Assert that the QTimer is connected (via QObject::connect or .connect)
    to a lambda/slot that calls CefDoMessageLoopWork().

    Looks for:
      QObject::connect(&cef_timer, &QTimer::timeout, [](){
          CefDoMessageLoopWork();
      });
    or similar patterns.
    """
    # Find all connect calls involving QTimer::timeout
    connect_pattern = re.compile(
        r'(?:QObject\s*::\s*)?connect\s*\([^;]*QTimer\s*::\s*timeout[^;]*\)',
        re.DOTALL,
    )
    # This pattern may not capture the full lambda; use a broader search
    # Find connect( ... timeout ... ) including the lambda body
    # Strategy: find 'connect' followed by 'timeout', then extract to matching ')'
    timeout_match = re.search(
        r'(?:QObject\s*::\s*)?connect\s*\(', main_body
    )

    # Find all connect calls and check if any contain both timeout and CefDoMessageLoopWork
    connect_starts = [m.start() for m in re.finditer(
        r'(?:QObject\s*::\s*)?connect\s*\(', main_body
    )]

    for start in connect_starts:
        # Extract the full connect(...) call by paren counting
        depth = 0
        call_body = ""
        for i in range(start, len(main_body)):
            ch = main_body[i]
            if ch == '(':
                depth += 1
            elif ch == ')':
                depth -= 1
                if depth == 0:
                    call_body = main_body[start:i + 1]
                    break

        if not call_body:
            continue

        has_timeout = bool(re.search(r'\bQTimer\s*::\s*timeout\b', call_body))
        has_cef_work = bool(re.search(r'\bCefDoMessageLoopWork\s*\(\s*\)', call_body))

        if has_timeout and has_cef_work:
            return {
                "found": True,
                "detail": (
                    "QTimer::timeout is connected to a lambda that calls "
                    "CefDoMessageLoopWork()."
                ),
                "call_body": call_body,
            }

    return {
        "found": False,
        "detail": (
            "No connect() call found that links QTimer::timeout to "
            "CefDoMessageLoopWork()."
        ),
        "call_body": None,
    }


def check_timer_started_with_interval(main_body: str, expected_ms: int) -> dict:
    """
    Assert that the timer is started with .start(10) (or the expected interval).
    """
    # Look for: cef_timer.start(10) or timer.start(10)
    pattern = re.compile(
        r'\b\w+\s*\.\s*start\s*\(\s*(\d+)\s*\)'
    )
    matches = list(pattern.finditer(main_body))

    for m in matches:
        interval = int(m.group(1))
        if interval == expected_ms:
            return {
                "found": True,
                "interval": interval,
                "detail": f"Timer started with interval {interval} ms: {m.group(0).strip()!r}",
            }

    if matches:
        found_intervals = [int(m.group(1)) for m in matches]
        return {
            "found": False,
            "interval": None,
            "detail": (
                f"Timer .start() found but with interval(s) {found_intervals}, "
                f"expected {expected_ms}."
            ),
        }

    return {
        "found": False,
        "interval": None,
        "detail": f"No timer .start({expected_ms}) call found in main().",
    }


def check_cef_work_only_in_timer(main_body: str) -> dict:
    """
    Assert that CefDoMessageLoopWork() is called ONLY inside the timer callback
    and not as a bare call elsewhere in main().

    Strategy:
    1. Find all occurrences of CefDoMessageLoopWork() in main_body.
    2. For each occurrence, check whether it is inside a lambda (i.e., inside
       a {...} block that is itself an argument to a connect() call).
    3. If any occurrence is NOT inside such a lambda, report it.
    """
    occurrences = [m.start() for m in re.finditer(
        r'\bCefDoMessageLoopWork\s*\(\s*\)', main_body
    )]

    if not occurrences:
        return {
            "ok": False,
            "detail": "CefDoMessageLoopWork() not found in main() at all.",
            "count": 0,
        }

    # For each occurrence, determine if it is inside a lambda that is itself
    # inside a connect() call.
    bare_calls = []
    for pos in occurrences:
        # Walk backwards to find the enclosing '{' at depth 1 (lambda body)
        depth = 0
        in_lambda = False
        i = pos
        while i >= 0:
            ch = main_body[i]
            if ch == '}':
                depth += 1
            elif ch == '{':
                if depth == 0:
                    # This is the enclosing block. Check if it's a lambda.
                    # Look backwards for '[' (lambda capture) before this '{'
                    before_brace = main_body[:i].rstrip()
                    # Lambda: [...](...)  {  or  [...]  {
                    if re.search(r'\]\s*(?:\([^)]*\))?\s*$', before_brace):
                        in_lambda = True
                    break
                else:
                    depth -= 1
            i -= 1

        if not in_lambda:
            line_no = main_body[:pos].count('\n') + 1
            bare_calls.append((pos, line_no))

    if bare_calls:
        lines = [str(ln) for _, ln in bare_calls]
        return {
            "ok": False,
            "detail": (
                f"CefDoMessageLoopWork() appears outside a lambda at line(s) "
                f"{', '.join(lines)} in main(). It should only be called from "
                "the timer callback."
            ),
            "count": len(occurrences),
        }

    return {
        "ok": True,
        "detail": (
            f"CefDoMessageLoopWork() is called {len(occurrences)} time(s), "
            "all inside timer callback lambda(s)."
        ),
        "count": len(occurrences),
    }


# ---------------------------------------------------------------------------
# Main test logic
# ---------------------------------------------------------------------------

def run_test() -> int:
    """
    Static analysis of CefDoMessageLoopWork() timer setup in main().
    Returns 0 (PASS) or 1 (FAIL).
    """
    print("=" * 70)
    print("Preservation Unit Test 7.3: CefDoMessageLoopWork() at 10 ms interval")
    print("=" * 70)
    print(f"Source file      : {SOURCE_FILE}")
    print(f"Expected interval: {EXPECTED_INTERVAL_MS} ms")
    print()

    if not os.path.isfile(SOURCE_FILE):
        print(f"ERROR: Source file not found at '{SOURCE_FILE}'")
        print("       Run from the repo root, or set LILYPAD_SOURCE env var.")
        return 1

    raw_source = load_source(SOURCE_FILE)
    source = strip_comments(raw_source)
    main_body = extract_main_body(source)

    if not main_body:
        print("ERROR: Could not locate main() function in source.")
        return 1

    failures = []

    # ------------------------------------------------------------------
    # Assertion 1: QTimer is created in main()
    # ------------------------------------------------------------------
    timer_result = check_qtimer_created(main_body)
    if timer_result["found"]:
        print(f"[PASS] QTimer is created in main().")
        print(f"       {timer_result['detail']}")
    else:
        failures.append(
            f"FAIL: {timer_result['detail']}\n"
            "      A QTimer is required to pump the CEF message loop."
        )
        print(f"[FAIL] {timer_result['detail']}")
    print()

    # ------------------------------------------------------------------
    # Assertion 2: Timer is connected to CefDoMessageLoopWork()
    # ------------------------------------------------------------------
    connect_result = check_timer_connected_to_cef_work(main_body)
    if connect_result["found"]:
        print(f"[PASS] {connect_result['detail']}")
    else:
        failures.append(
            f"FAIL: {connect_result['detail']}\n"
            "      CefDoMessageLoopWork() must be called from the timer callback."
        )
        print(f"[FAIL] {connect_result['detail']}")
    print()

    # ------------------------------------------------------------------
    # Assertion 3: Timer started with interval 10 ms
    # ------------------------------------------------------------------
    start_result = check_timer_started_with_interval(main_body, EXPECTED_INTERVAL_MS)
    if start_result["found"]:
        print(f"[PASS] {start_result['detail']}")
    else:
        failures.append(
            f"FAIL: {start_result['detail']}\n"
            f"      The timer must be started with interval {EXPECTED_INTERVAL_MS} ms."
        )
        print(f"[FAIL] {start_result['detail']}")
    print()

    # ------------------------------------------------------------------
    # Assertion 4: CefDoMessageLoopWork() only inside timer callback
    # ------------------------------------------------------------------
    only_in_timer_result = check_cef_work_only_in_timer(main_body)
    if only_in_timer_result["ok"]:
        print(f"[PASS] {only_in_timer_result['detail']}")
    else:
        failures.append(
            f"FAIL: {only_in_timer_result['detail']}"
        )
        print(f"[FAIL] {only_in_timer_result['detail']}")
    print()

    if failures:
        print("RESULT: FAIL")
        for msg in failures:
            print(f"  {msg}")
        return 1

    print("RESULT: PASS")
    print(
        f"  QTimer connected to CefDoMessageLoopWork(); started at "
        f"{EXPECTED_INTERVAL_MS} ms; called only inside the timer callback."
    )
    return 0


if __name__ == "__main__":
    sys.exit(run_test())
