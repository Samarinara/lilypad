#!/usr/bin/env python3
"""
Fix-Checking Unit Test — Task 3.3
===================================
Verifies that GetBrowser() is checked for null before LoadURL() is called
inside the returnPressed lambda in the MainWindow constructor.

WHAT THIS TESTS
---------------
Requirement 4.3 mandates that navigation is only attempted when a live
browser instance exists. If GetBrowser() returns null (e.g., the browser
has not yet been created or has already been closed), calling LoadURL()
on a null pointer would crash the application.

Requirement 6.3 mandates that the application does not crash when the
user presses Enter before the browser is ready. The null-guard ensures
the navigation is silently skipped in that case.

This test uses static source analysis to confirm:
  1. The returnPressed lambda body contains a null-guard check on
     GetBrowser() (e.g., `if (auto browser = m_handler->GetBrowser())`
     or `if (m_handler->GetBrowser())`).
  2. LoadURL() is only called inside the null-guard conditional — i.e.,
     it is not reachable when GetBrowser() returns null.

EXPECTED RESULT ON FIXED CODE
------------------------------
  PASS — GetBrowser() is checked for null; LoadURL() is inside the guard.

EXPECTED RESULT ON UNFIXED CODE
--------------------------------
  FAIL — GetBrowser() null-check is absent, or LoadURL() is called
  unconditionally outside the guard.

REQUIRES
--------
  - src/mainwindow.cpp present at ./src/mainwindow.cpp
    (or LILYPAD_MAINWINDOW env var)
  - Python 3.8+ (no external dependencies)

Validates: Requirements 4.3, 6.3
"""

import os
import re
import sys

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
SOURCE_FILE = os.environ.get("LILYPAD_MAINWINDOW", "./src/mainwindow.cpp")

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load_source(path: str) -> str:
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def extract_constructor_body(source: str) -> str:
    """
    Extract the body of the MainWindow constructor from the source.
    Looks for 'MainWindow::MainWindow' and extracts the braced body using
    brace counting.
    """
    match = re.search(r'\bMainWindow\s*::\s*MainWindow\b', source)
    if not match:
        return ""

    # Find the opening brace of the constructor body
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


def extract_return_pressed_lambda(constructor_body: str) -> str:
    """
    Extract the body of the returnPressed lambda from the constructor body.
    Looks for the connect() call that references returnPressed and extracts
    the lambda body using brace counting.

    Returns the content of the lambda's braced body (including the braces),
    or an empty string if not found.
    """
    # Find the returnPressed connect call
    match = re.search(r'returnPressed', constructor_body)
    if not match:
        return ""

    # Find the opening brace of the lambda body after returnPressed
    # The lambda looks like: [this](){ ... } or [this]() { ... }
    lambda_brace_pattern = re.compile(r'\[this\]\s*\(\s*\)\s*\{', re.DOTALL)
    lambda_match = lambda_brace_pattern.search(constructor_body, match.start())
    if not lambda_match:
        return ""

    # The opening brace is the last character of the match
    start = lambda_match.end() - 1
    depth = 0
    for i in range(start, len(constructor_body)):
        if constructor_body[i] == '{':
            depth += 1
        elif constructor_body[i] == '}':
            depth -= 1
            if depth == 0:
                return constructor_body[start:i + 1]
    return constructor_body[start:]


def non_comment_lines(text: str) -> list:
    """Return lines that are not pure C++ line comments or block comment lines."""
    result = []
    for line in text.splitlines():
        stripped = line.strip()
        if not stripped.startswith("//") and not stripped.startswith("*"):
            result.append(line)
    return result


def check_null_guard_present(lambda_body: str) -> dict:
    """
    Check that GetBrowser() is checked for null inside the lambda body.

    Accepts patterns like:
      if (auto browser = m_handler->GetBrowser())
      if (m_handler->GetBrowser())
      if (auto b = m_handler->GetBrowser())
    """
    clean_lines = non_comment_lines(lambda_body)
    clean_text = "\n".join(clean_lines)

    # Pattern: if ( [optional: auto varname =] m_handler->GetBrowser() )
    pattern = re.compile(
        r'\bif\s*\(\s*'
        r'(?:auto\s+\w+\s*=\s*)?'          # optional: auto varname =
        r'm_handler\s*->\s*GetBrowser\s*\(\s*\)'
        r'\s*\)',
    )
    match = pattern.search(clean_text)
    if match:
        # Extract the line for reporting
        line_start = clean_text.rfind('\n', 0, match.start()) + 1
        line_end = clean_text.find('\n', match.end())
        snippet = clean_text[
            line_start: line_end if line_end != -1 else len(clean_text)
        ].strip()
        return {
            "found": True,
            "detail": f"Found: {snippet}",
        }
    return {
        "found": False,
        "detail": (
            "No null-guard on GetBrowser() found in the returnPressed lambda.\n"
            "          Expected a pattern like:\n"
            "            if (auto browser = m_handler->GetBrowser())\n"
            "          or:\n"
            "            if (m_handler->GetBrowser())\n"
            "          Without this guard, calling LoadURL() when the browser is\n"
            "          not yet ready would dereference a null pointer and crash."
        ),
    }


def check_load_url_inside_guard(lambda_body: str) -> dict:
    """
    Check that LoadURL() is only called inside the null-guard conditional,
    not unconditionally before the guard.

    Strategy:
      1. Confirm LoadURL() appears in the lambda body at all.
      2. Confirm that every occurrence of LoadURL() in the lambda body
         is preceded (somewhere earlier in the body) by a GetBrowser()
         null-guard `if` statement.  In other words, there must be no
         LoadURL() call that appears before the null-guard check.

    This handles both braced and braceless `if` forms:
      if (auto browser = m_handler->GetBrowser())          // braceless
          browser->GetMainFrame()->LoadURL(...);
      if (auto browser = m_handler->GetBrowser()) {        // braced
          browser->GetMainFrame()->LoadURL(...);
      }
    """
    clean_lines = non_comment_lines(lambda_body)
    clean_text = "\n".join(clean_lines)

    # First, confirm LoadURL() is present at all
    load_url_match = re.search(r'\bLoadURL\s*\(', clean_text)
    if not load_url_match:
        return {
            "found": False,
            "detail": (
                "LoadURL() not found in the returnPressed lambda body.\n"
                "          The lambda must call LoadURL() to navigate when the\n"
                "          browser is ready."
            ),
        }

    # Find the position of the null-guard if statement
    guard_pattern = re.compile(
        r'\bif\s*\(\s*'
        r'(?:auto\s+\w+\s*=\s*)?'          # optional: auto varname =
        r'm_handler\s*->\s*GetBrowser\s*\(\s*\)'
        r'\s*\)',
    )
    guard_match = guard_pattern.search(clean_text)

    if not guard_match:
        # No guard at all — LoadURL() is unconditional
        return {
            "found": False,
            "detail": (
                "LoadURL() is present but no GetBrowser() null-guard was found.\n"
                "          LoadURL() must only be called inside the null-guard\n"
                "          conditional so that it is skipped when GetBrowser() is null."
            ),
        }

    # Check that LoadURL() does NOT appear before the guard
    load_url_pos = load_url_match.start()
    guard_pos = guard_match.start()

    if load_url_pos < guard_pos:
        return {
            "found": False,
            "detail": (
                "LoadURL() appears before the GetBrowser() null-guard in the lambda.\n"
                "          LoadURL() must only be called inside the null-guard\n"
                "          conditional so that it is skipped when GetBrowser() is null."
            ),
        }

    return {
        "found": True,
        "detail": (
            "LoadURL() appears after the GetBrowser() null-guard "
            "(not before it in the lambda body)."
        ),
    }


# ---------------------------------------------------------------------------
# Main test logic
# ---------------------------------------------------------------------------

def run_test() -> int:
    """
    Static analysis of the returnPressed lambda in MainWindow::MainWindow().
    Returns 0 (PASS) or 1 (FAIL).
    """
    print("=" * 70)
    print("Fix-Checking Unit Test 3.3: Null-guard before LoadURL()")
    print("=" * 70)
    print(f"Source file: {SOURCE_FILE}")
    print()

    if not os.path.isfile(SOURCE_FILE):
        print(f"ERROR: Source file not found at '{SOURCE_FILE}'")
        print("       Run from the repo root, or set LILYPAD_MAINWINDOW env var.")
        return 1

    source = load_source(SOURCE_FILE)
    constructor_body = extract_constructor_body(source)

    if not constructor_body:
        print("ERROR: Could not locate MainWindow::MainWindow constructor in source.")
        return 1

    lambda_body = extract_return_pressed_lambda(constructor_body)

    if not lambda_body:
        print("ERROR: Could not locate the returnPressed lambda in the constructor.")
        print("       Expected a connect() call referencing returnPressed with a")
        print("       [this](){...} lambda.")
        return 1

    failures = []

    # --- Assertion 1: GetBrowser() is checked for null ---
    result = check_null_guard_present(lambda_body)
    if result["found"]:
        print("[PASS] GetBrowser() is checked for null before LoadURL().")
        print(f"       {result['detail']}")
    else:
        failures.append(
            f"FAIL: GetBrowser() null-guard is MISSING in the returnPressed lambda.\n"
            f"      {result['detail']}"
        )
    print()

    # --- Assertion 2: LoadURL() is only called inside the null-guard ---
    result = check_load_url_inside_guard(lambda_body)
    if result["found"]:
        print("[PASS] LoadURL() is only called inside the null-guard conditional.")
        print(f"       {result['detail']}")
    else:
        failures.append(
            f"FAIL: LoadURL() is NOT safely guarded.\n"
            f"      {result['detail']}"
        )
    print()

    if failures:
        print("RESULT: FAIL")
        for msg in failures:
            print(f"  {msg}")
        return 1

    print("RESULT: PASS")
    print(
        "  GetBrowser() is null-checked before LoadURL() is called;\n"
        "  LoadURL() is only reachable when the browser is non-null."
    )
    return 0


if __name__ == "__main__":
    sys.exit(run_test())
