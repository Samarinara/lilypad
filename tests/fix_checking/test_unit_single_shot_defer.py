#!/usr/bin/env python3
"""
Fix-Checking Unit Test — Task 6.5
===================================
Verifies that QTimer::singleShot(0, ...) is used to defer the createBrowser()
call in main(), and that this deferral appears after w.show().

WHAT THIS TESTS
---------------
Requirement 3.3 mandates that createBrowser() is called only after the Qt
window has been shown and its layout has been processed, so that m_container
has non-zero geometry when CEF reads it.

Requirement 3.4 mandates that the deferral mechanism is QTimer::singleShot(0, ...)
which guarantees at least one event-loop tick separates w.show() from
createBrowser(), ensuring valid geometry.

This test uses static source analysis to confirm:
  1. QTimer::singleShot(0, ...) is called inside main().
  2. The singleShot lambda wraps a createBrowser() call.
  3. The singleShot call appears after w.show() in source order.

EXPECTED RESULT ON FIXED CODE
------------------------------
  PASS — QTimer::singleShot(0, ...) wraps createBrowser() and appears after w.show().

EXPECTED RESULT ON UNFIXED CODE
--------------------------------
  FAIL — singleShot is absent, does not wrap createBrowser(), or appears before w.show().

REQUIRES
--------
  - src/main.cpp present at ./src/main.cpp
    (or LILYPAD_SOURCE env var)
  - Python 3.8+ (no external dependencies)

Validates: Requirements 3.3, 3.4
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


def find_line_index(lines: list, pattern: re.Pattern) -> int:
    """Return the index of the first line matching pattern, or -1 if not found."""
    for i, line in enumerate(lines):
        if pattern.search(line):
            return i
    return -1


def check_single_shot_present(main_body: str) -> dict:
    """
    Check that QTimer::singleShot(0, ...) is called inside main().

    Accepts patterns like:
      QTimer::singleShot(0, ...)
      QTimer::singleShot( 0, ...)
    """
    clean_lines = non_comment_lines(main_body)
    clean_text = "\n".join(clean_lines)

    pattern = re.compile(
        r'QTimer\s*::\s*singleShot\s*\(\s*0\s*,',
    )
    match = pattern.search(clean_text)
    if match:
        return {
            "found": True,
            "detail": f"Found: {match.group(0).strip()}...",
        }
    return {
        "found": False,
        "detail": (
            "QTimer::singleShot(0, ...) not found in main().\n"
            "          Without this deferral, createBrowser() may be called before\n"
            "          m_container has valid geometry, causing a zero-size CEF viewport."
        ),
    }


def check_single_shot_wraps_create_browser(main_body: str) -> dict:
    """
    Check that the QTimer::singleShot(0, ...) lambda contains a createBrowser() call.

    Looks for the singleShot call and then scans the lambda body (braced block
    or inline expression) for a createBrowser invocation.

    Accepts patterns like:
      QTimer::singleShot(0, [&w](){ w.createBrowser("..."); });
      QTimer::singleShot(0, [&w]{ w.createBrowser("..."); });
    """
    clean_lines = non_comment_lines(main_body)
    clean_text = "\n".join(clean_lines)

    # Find the singleShot call
    single_shot_pattern = re.compile(
        r'QTimer\s*::\s*singleShot\s*\(\s*0\s*,',
    )
    match = single_shot_pattern.search(clean_text)
    if not match:
        return {
            "found": False,
            "detail": "QTimer::singleShot(0, ...) not found — cannot check lambda body.",
        }

    # Extract from the singleShot call to the end of the statement (closing ");")
    # We look for createBrowser within a reasonable window after the singleShot
    search_start = match.start()
    # Find the end of the singleShot statement by tracking parenthesis depth
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

    single_shot_stmt = clean_text[search_start:stmt_end]

    create_browser_pattern = re.compile(r'\bcreateBrowser\s*\(')
    cb_match = create_browser_pattern.search(single_shot_stmt)
    if cb_match:
        # Show a compact excerpt
        excerpt = single_shot_stmt.replace('\n', ' ')
        excerpt = re.sub(r'\s+', ' ', excerpt).strip()
        if len(excerpt) > 80:
            excerpt = excerpt[:77] + "..."
        return {
            "found": True,
            "detail": f"Found createBrowser() inside singleShot lambda: {excerpt}",
        }
    return {
        "found": False,
        "detail": (
            "createBrowser() not found inside the QTimer::singleShot(0, ...) lambda.\n"
            "          The singleShot must wrap the createBrowser() call to defer it\n"
            "          until after the event loop has processed the initial layout."
        ),
    }


def check_single_shot_after_show(main_body: str) -> dict:
    """
    Check that QTimer::singleShot(0, ...) appears after w.show() in source order.

    Compares the line indices of the first w.show() call and the first
    QTimer::singleShot(0, ...) call in the cleaned main() body.
    """
    clean_lines = non_comment_lines(main_body)

    show_pattern = re.compile(r'\bw\s*\.\s*show\s*\(\s*\)')
    single_shot_pattern = re.compile(r'QTimer\s*::\s*singleShot\s*\(\s*0\s*,')

    show_idx = find_line_index(clean_lines, show_pattern)
    single_shot_idx = find_line_index(clean_lines, single_shot_pattern)

    if show_idx == -1:
        return {
            "found": False,
            "detail": (
                "w.show() not found in main(). Cannot verify ordering.\n"
                "          The window must be shown before createBrowser() is deferred."
            ),
        }

    if single_shot_idx == -1:
        return {
            "found": False,
            "detail": (
                "QTimer::singleShot(0, ...) not found in main(). Cannot verify ordering."
            ),
        }

    if single_shot_idx > show_idx:
        return {
            "found": True,
            "detail": (
                f"w.show() is at line {show_idx + 1} of main() body; "
                f"QTimer::singleShot(0, ...) is at line {single_shot_idx + 1} — correct order."
            ),
        }
    else:
        return {
            "found": False,
            "detail": (
                f"QTimer::singleShot(0, ...) appears at line {single_shot_idx + 1} of main() body, "
                f"but w.show() is at line {show_idx + 1}.\n"
                "          singleShot must come AFTER w.show() so the window is visible\n"
                "          and has valid geometry before createBrowser() is deferred."
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
    print("Fix-Checking Unit Test 6.5: QTimer::singleShot(0, ...) deferral")
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

    # --- Assertion 1: QTimer::singleShot(0, ...) is present in main() ---
    ss_result = check_single_shot_present(main_body)
    if ss_result["found"]:
        print(f"[PASS] QTimer::singleShot(0, ...) is present in main().")
        print(f"       {ss_result['detail']}")
    else:
        failures.append(
            f"FAIL: QTimer::singleShot(0, ...) is NOT present in main().\n"
            f"      {ss_result['detail']}"
        )
    print()

    # --- Assertion 2: singleShot lambda wraps createBrowser() ---
    cb_result = check_single_shot_wraps_create_browser(main_body)
    if cb_result["found"]:
        print(f"[PASS] singleShot lambda wraps a createBrowser() call.")
        print(f"       {cb_result['detail']}")
    else:
        failures.append(
            f"FAIL: singleShot lambda does NOT wrap createBrowser().\n"
            f"      {cb_result['detail']}"
        )
    print()

    # --- Assertion 3: singleShot appears after w.show() ---
    order_result = check_single_shot_after_show(main_body)
    if order_result["found"]:
        print(f"[PASS] QTimer::singleShot(0, ...) appears after w.show().")
        print(f"       {order_result['detail']}")
    else:
        failures.append(
            f"FAIL: QTimer::singleShot(0, ...) does NOT appear after w.show().\n"
            f"      {order_result['detail']}"
        )
    print()

    if failures:
        print("RESULT: FAIL")
        for msg in failures:
            print(f"  {msg}")
        return 1

    print("RESULT: PASS")
    print(
        "  QTimer::singleShot(0, ...) wraps createBrowser() and appears after w.show()."
    )
    return 0


if __name__ == "__main__":
    sys.exit(run_test())
