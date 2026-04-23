#!/usr/bin/env python3
"""
Fix-Checking Unit Test — Task 6.6
===================================
Verifies that a QTimer is connected to a lambda calling CefDoMessageLoopWork()
in main(), and that the timer is started with an interval ≤ 16 ms.

WHAT THIS TESTS
---------------
Requirement 5.1 mandates that CefDoMessageLoopWork() is called periodically
to give CEF a timeslice within the Qt event loop.

Requirement 5.2 mandates that the interval is ≤ 16 ms (the 60 fps budget),
ensuring CEF gets enough CPU time for smooth rendering.

This test uses static source analysis to confirm:
  1. A QTimer is connected (via QObject::connect) to a lambda that calls
     CefDoMessageLoopWork().
  2. The timer is started with an interval ≤ 16 ms (expected: 10 ms).
  3. CefDoMessageLoopWork() is called ONLY inside the timer callback, not as
     a bare call elsewhere in main().

EXPECTED RESULT ON FIXED CODE
------------------------------
  PASS — QTimer connected to CefDoMessageLoopWork() lambda, started at ≤ 16 ms,
         and CefDoMessageLoopWork() not called bare elsewhere in main().

EXPECTED RESULT ON UNFIXED CODE
--------------------------------
  FAIL — Timer absent, interval too large, or CefDoMessageLoopWork() called
         outside the timer callback.

REQUIRES
--------
  - src/main.cpp present at ./src/main.cpp
    (or LILYPAD_SOURCE env var)
  - Python 3.8+ (no external dependencies)

Validates: Requirements 5.1, 5.2
"""

import os
import re
import sys

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
SOURCE_FILE = os.environ.get("LILYPAD_SOURCE", "./src/main.cpp")

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load_source(path: str) -> str:
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def extract_main_body(source: str) -> str:
    """
    Extract the body of the main() function from the source.
    Looks for 'int main(' and extracts the braced body using brace counting.
    """
    match = re.search(r'\bint\s+main\s*\(', source)
    if not match:
        return ""

    # Find the opening brace of the main body
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
                return source[start:i + 1]
    return source[start:]


def non_comment_lines(text: str) -> list:
    """Return lines that are not pure C++ line comments or block comment lines."""
    result = []
    for line in text.splitlines():
        stripped = line.strip()
        if not stripped.startswith("//") and not stripped.startswith("*"):
            result.append(line)
    return result


def strip_comments(text: str) -> str:
    """Remove C++ line comments and block comments from text."""
    # Remove block comments /* ... */
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)
    # Remove line comments // ...
    text = re.sub(r'//[^\n]*', '', text)
    return text


def find_connect_lambda_with_cef_work(main_body: str) -> dict:
    """
    Check that QObject::connect() is used to connect a QTimer's timeout signal
    to a lambda that calls CefDoMessageLoopWork().

    Accepts patterns like:
      QObject::connect(&cef_timer, &QTimer::timeout, [](){ CefDoMessageLoopWork(); });
      QObject::connect(&cef_timer, &QTimer::timeout, []{ CefDoMessageLoopWork(); });
    """
    clean_text = strip_comments(main_body)

    # Find QObject::connect( ... QTimer::timeout ... ) calls
    connect_pattern = re.compile(
        r'QObject\s*::\s*connect\s*\(',
    )

    for match in connect_pattern.finditer(clean_text):
        # Extract the full connect() call by tracking parenthesis depth
        depth = 0
        stmt_end = len(clean_text)
        for i in range(match.start(), len(clean_text)):
            if clean_text[i] == '(':
                depth += 1
            elif clean_text[i] == ')':
                depth -= 1
                if depth == 0:
                    stmt_end = i + 1
                    break

        connect_stmt = clean_text[match.start():stmt_end]

        # Must reference QTimer::timeout
        if not re.search(r'QTimer\s*::\s*timeout', connect_stmt):
            continue

        # Must contain a lambda ([ ... ]) that calls CefDoMessageLoopWork()
        if re.search(r'\[.*?\]', connect_stmt, re.DOTALL) and \
           re.search(r'\bCefDoMessageLoopWork\s*\(\s*\)', connect_stmt):
            excerpt = re.sub(r'\s+', ' ', connect_stmt).strip()
            if len(excerpt) > 100:
                excerpt = excerpt[:97] + "..."
            return {
                "found": True,
                "detail": f"Found: {excerpt}",
            }

    return {
        "found": False,
        "detail": (
            "No QObject::connect() call found that connects QTimer::timeout to a\n"
            "          lambda calling CefDoMessageLoopWork().\n"
            "          Without this, CEF will not receive periodic timeslices and\n"
            "          the browser will not render or process network events."
        ),
    }


def find_timer_start_interval(main_body: str) -> dict:
    """
    Check that the CEF timer is started with an interval ≤ 16 ms.

    Looks for cef_timer.start(N) or similar patterns and extracts N.
    Accepts patterns like:
      cef_timer.start(10);
      cef_timer.start( 10 );
    """
    clean_text = strip_comments(main_body)

    # Look for <identifier>.start(<integer>) where the identifier contains "cef"
    # or is a QTimer variable near the CefDoMessageLoopWork connect call.
    # Strategy: find all .start(<integer>) calls and check the interval.
    start_pattern = re.compile(
        r'(\w+)\s*\.\s*start\s*\(\s*(\d+)\s*\)',
    )

    candidates = []
    for match in start_pattern.finditer(clean_text):
        var_name = match.group(1)
        interval = int(match.group(2))
        candidates.append((var_name, interval, match.group(0)))

    if not candidates:
        return {
            "found": False,
            "interval": None,
            "detail": (
                "No timer.start(N) call found in main().\n"
                "          The CEF message loop timer must be started with an interval ≤ 16 ms."
            ),
        }

    # Prefer a candidate whose variable name contains "cef"
    cef_candidates = [(v, i, s) for v, i, s in candidates if "cef" in v.lower()]
    chosen = cef_candidates[0] if cef_candidates else candidates[0]
    var_name, interval, stmt = chosen

    if interval <= 16:
        return {
            "found": True,
            "interval": interval,
            "detail": (
                f"Timer '{var_name}' started with interval {interval} ms "
                f"(≤ 16 ms budget). Found: {stmt}"
            ),
        }
    else:
        return {
            "found": False,
            "interval": interval,
            "detail": (
                f"Timer '{var_name}' started with interval {interval} ms, "
                f"which exceeds the 16 ms (60 fps) budget.\n"
                "          Reduce the interval to ≤ 16 ms so CEF gets enough timeslices\n"
                "          for smooth rendering."
            ),
        }


def find_bare_cef_do_message_loop_work(main_body: str) -> dict:
    """
    Check that CefDoMessageLoopWork() is NOT called as a bare statement
    outside the timer callback lambda in main().

    Strategy:
      1. Find the QObject::connect() call that contains the lambda with
         CefDoMessageLoopWork() — this is the allowed occurrence.
      2. Remove that connect() statement from the text.
      3. Search the remaining text for any CefDoMessageLoopWork() calls.
    """
    clean_text = strip_comments(main_body)

    # Find and remove the connect() statement containing the lambda
    connect_pattern = re.compile(r'QObject\s*::\s*connect\s*\(')
    remaining_text = clean_text

    for match in connect_pattern.finditer(clean_text):
        depth = 0
        stmt_end = len(clean_text)
        for i in range(match.start(), len(clean_text)):
            if clean_text[i] == '(':
                depth += 1
            elif clean_text[i] == ')':
                depth -= 1
                if depth == 0:
                    stmt_end = i + 1
                    break

        connect_stmt = clean_text[match.start():stmt_end]

        # Only remove the connect() that contains the CefDoMessageLoopWork lambda
        if re.search(r'QTimer\s*::\s*timeout', connect_stmt) and \
           re.search(r'\bCefDoMessageLoopWork\s*\(\s*\)', connect_stmt):
            remaining_text = clean_text[:match.start()] + clean_text[stmt_end:]
            break

    # Now search remaining text for any CefDoMessageLoopWork() calls
    bare_pattern = re.compile(r'\bCefDoMessageLoopWork\s*\(\s*\)')
    bare_matches = list(bare_pattern.finditer(remaining_text))

    if not bare_matches:
        return {
            "found": True,  # "found" means the assertion passes (no bare calls)
            "detail": (
                "CefDoMessageLoopWork() is called only inside the timer callback lambda — "
                "no bare calls found elsewhere in main()."
            ),
        }
    else:
        # Show context around the first bare call
        first = bare_matches[0]
        ctx_start = max(0, first.start() - 40)
        ctx_end = min(len(remaining_text), first.end() + 40)
        ctx = re.sub(r'\s+', ' ', remaining_text[ctx_start:ctx_end]).strip()
        return {
            "found": False,
            "detail": (
                f"CefDoMessageLoopWork() is called {len(bare_matches)} time(s) "
                f"outside the timer callback lambda.\n"
                f"          Context: ...{ctx}...\n"
                "          CefDoMessageLoopWork() must only be called from the periodic\n"
                "          timer callback to avoid double-pumping the CEF message loop."
            ),
        }


# ---------------------------------------------------------------------------
# Main test logic
# ---------------------------------------------------------------------------

def run_test() -> int:
    """
    Static analysis of main() in main.cpp.
    Returns 0 (PASS) or 1 (FAIL).
    """
    print("=" * 70)
    print("Fix-Checking Unit Test 6.6: CefDoMessageLoopWork() timer at ≤ 16 ms")
    print("=" * 70)
    print(f"Source file: {SOURCE_FILE}")
    print()

    if not os.path.isfile(SOURCE_FILE):
        print(f"ERROR: Source file not found at '{SOURCE_FILE}'")
        print("       Run from the repo root, or set LILYPAD_SOURCE env var.")
        return 1

    source = load_source(SOURCE_FILE)
    main_body = extract_main_body(source)

    if not main_body:
        print("ERROR: Could not locate main() function in source.")
        return 1

    failures = []

    # --- Assertion 1: QTimer connected to lambda calling CefDoMessageLoopWork() ---
    connect_result = find_connect_lambda_with_cef_work(main_body)
    if connect_result["found"]:
        print("[PASS] QTimer::timeout is connected to a CefDoMessageLoopWork() lambda.")
        print(f"       {connect_result['detail']}")
    else:
        failures.append(
            f"FAIL: QTimer::timeout is NOT connected to a CefDoMessageLoopWork() lambda.\n"
            f"      {connect_result['detail']}"
        )
    print()

    # --- Assertion 2: Timer started with interval ≤ 16 ms ---
    interval_result = find_timer_start_interval(main_body)
    if interval_result["found"]:
        print(f"[PASS] Timer started with interval ≤ 16 ms.")
        print(f"       {interval_result['detail']}")
    else:
        failures.append(
            f"FAIL: Timer interval check failed.\n"
            f"      {interval_result['detail']}"
        )
    print()

    # --- Assertion 3: CefDoMessageLoopWork() only inside timer callback ---
    bare_result = find_bare_cef_do_message_loop_work(main_body)
    if bare_result["found"]:
        print("[PASS] CefDoMessageLoopWork() is called only inside the timer callback.")
        print(f"       {bare_result['detail']}")
    else:
        failures.append(
            f"FAIL: CefDoMessageLoopWork() found outside the timer callback.\n"
            f"      {bare_result['detail']}"
        )
    print()

    if failures:
        print("RESULT: FAIL")
        for msg in failures:
            print(f"  {msg}")
        return 1

    print("RESULT: PASS")
    print(
        "  QTimer connected to CefDoMessageLoopWork() lambda, started at ≤ 16 ms,\n"
        "  and CefDoMessageLoopWork() not called bare elsewhere in main()."
    )
    return 0


if __name__ == "__main__":
    sys.exit(run_test())
