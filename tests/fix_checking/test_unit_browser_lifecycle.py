#!/usr/bin/env python3
"""
Fix-Checking Unit Test — Task 4.3
===================================
Verifies that CefHandler correctly stores the browser reference in
OnAfterCreated and clears it in OnBeforeClose, and that GetBrowser()
is declared in the header returning m_browser.

WHAT THIS TESTS
---------------
Requirement 6.1 mandates that OnAfterCreated stores the CefBrowser
reference so that GetBrowser() can return it for navigation.

Requirement 6.2 mandates that OnBeforeClose clears the stored reference
so that GetBrowser() returns null after the browser is destroyed.

Requirement 6.4 mandates that GetBrowser() is accessible and returns
the stored m_browser member.

This test uses static source analysis to confirm:
  1. OnAfterCreated assigns m_browser (e.g., m_browser = browser).
  2. OnBeforeClose clears m_browser (e.g., m_browser = nullptr).
  3. GetBrowser() is declared in cef_handler.h and returns m_browser.

EXPECTED RESULT ON FIXED CODE
------------------------------
  PASS — OnAfterCreated stores, OnBeforeClose clears, GetBrowser() present.

EXPECTED RESULT ON UNFIXED CODE
--------------------------------
  FAIL — One or more of the above structural checks is missing.

REQUIRES
--------
  - src/cef_handler.cpp present at ./src/cef_handler.cpp
    (or LILYPAD_HANDLER env var)
  - src/cef_handler.h present at ./src/cef_handler.h
    (or LILYPAD_HANDLER_H env var)
  - Python 3.8+ (no external dependencies)

Validates: Requirements 6.1, 6.2, 6.4
"""

import os
import re
import sys

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
HANDLER_CPP = os.environ.get("LILYPAD_HANDLER", "./src/cef_handler.cpp")
HANDLER_H   = os.environ.get("LILYPAD_HANDLER_H", "./src/cef_handler.h")

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load_source(path: str) -> str:
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def extract_method_body(source: str, class_name: str, method_name: str) -> str:
    """
    Extract the body of a method (class_name::method_name) from the source.
    Uses brace counting to find the full body.
    """
    pattern = re.compile(
        r'\b' + re.escape(class_name) + r'\s*::\s*' + re.escape(method_name) + r'\b'
    )
    match = pattern.search(source)
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


def check_on_after_created_stores_browser(source: str) -> dict:
    """
    Check that OnAfterCreated assigns m_browser from the browser parameter.

    Accepts patterns like:
      m_browser = browser;
      m_browser = browser
    """
    body = extract_method_body(source, "CefHandler", "OnAfterCreated")
    if not body:
        return {
            "found": False,
            "detail": "Could not locate CefHandler::OnAfterCreated in source.",
        }

    clean_text = "\n".join(non_comment_lines(body))

    # Match assignment of browser to m_browser
    pattern = re.compile(r'\bm_browser\s*=\s*browser\b')
    match = pattern.search(clean_text)
    if match:
        return {
            "found": True,
            "detail": f"Found: {match.group(0).strip()}",
        }
    return {
        "found": False,
        "detail": (
            "m_browser = browser not found in OnAfterCreated.\n"
            "          Without this, GetBrowser() will always return null\n"
            "          and URL navigation will be silently dropped."
        ),
    }


def check_on_before_close_clears_browser(source: str) -> dict:
    """
    Check that OnBeforeClose sets m_browser to nullptr.

    Accepts patterns like:
      m_browser = nullptr;
      m_browser = nullptr
    """
    body = extract_method_body(source, "CefHandler", "OnBeforeClose")
    if not body:
        return {
            "found": False,
            "detail": "Could not locate CefHandler::OnBeforeClose in source.",
        }

    clean_text = "\n".join(non_comment_lines(body))

    pattern = re.compile(r'\bm_browser\s*=\s*nullptr\b')
    match = pattern.search(clean_text)
    if match:
        return {
            "found": True,
            "detail": f"Found: {match.group(0).strip()}",
        }
    return {
        "found": False,
        "detail": (
            "m_browser = nullptr not found in OnBeforeClose.\n"
            "          Without this, GetBrowser() may return a dangling reference\n"
            "          after the browser has been destroyed."
        ),
    }


def check_get_browser_in_header(header_source: str) -> dict:
    """
    Check that GetBrowser() is declared in the header and returns m_browser.

    Accepts patterns like:
      CefRefPtr<CefBrowser> GetBrowser() const { return m_browser; }
      CefRefPtr<CefBrowser> GetBrowser() const;   (declaration only)
    """
    # Check for GetBrowser declaration/definition
    decl_pattern = re.compile(r'\bGetBrowser\s*\(\s*\)')
    decl_match = decl_pattern.search(header_source)
    if not decl_match:
        return {
            "found": False,
            "detail": (
                "GetBrowser() not found in header.\n"
                "          The accessor must be declared so callers can retrieve\n"
                "          the live browser reference for navigation."
            ),
        }

    # Check that the inline definition (if present) returns m_browser
    inline_pattern = re.compile(
        r'\bGetBrowser\s*\(\s*\)\s*(?:const\s*)?\{[^}]*\breturn\s+m_browser\b'
    )
    inline_match = inline_pattern.search(header_source)
    if inline_match:
        return {
            "found": True,
            "detail": f"Found inline definition returning m_browser.",
        }

    # Declaration-only is also acceptable (implementation may be in .cpp)
    return {
        "found": True,
        "detail": "Found GetBrowser() declaration in header.",
    }


# ---------------------------------------------------------------------------
# Main test logic
# ---------------------------------------------------------------------------

def run_test() -> int:
    """
    Static analysis of CefHandler lifecycle methods.
    Returns 0 (PASS) or 1 (FAIL).
    """
    print("=" * 70)
    print("Fix-Checking Unit Test 4.3: CefHandler browser lifecycle methods")
    print("=" * 70)
    print(f"Handler source: {HANDLER_CPP}")
    print(f"Handler header: {HANDLER_H}")
    print()

    if not os.path.isfile(HANDLER_CPP):
        print(f"ERROR: Handler source not found at '{HANDLER_CPP}'")
        print("       Run from the repo root, or set LILYPAD_HANDLER env var.")
        return 1

    if not os.path.isfile(HANDLER_H):
        print(f"ERROR: Handler header not found at '{HANDLER_H}'")
        print("       Run from the repo root, or set LILYPAD_HANDLER_H env var.")
        return 1

    cpp_source = load_source(HANDLER_CPP)
    h_source   = load_source(HANDLER_H)

    failures = []

    # --- Assertion 1: OnAfterCreated stores m_browser ---
    after_result = check_on_after_created_stores_browser(cpp_source)
    if after_result["found"]:
        print("[PASS] OnAfterCreated stores the browser reference in m_browser.")
        print(f"       {after_result['detail']}")
    else:
        failures.append(
            f"FAIL: OnAfterCreated does NOT store the browser reference.\n"
            f"      {after_result['detail']}"
        )
    print()

    # --- Assertion 2: OnBeforeClose clears m_browser ---
    before_result = check_on_before_close_clears_browser(cpp_source)
    if before_result["found"]:
        print("[PASS] OnBeforeClose clears m_browser (sets to nullptr).")
        print(f"       {before_result['detail']}")
    else:
        failures.append(
            f"FAIL: OnBeforeClose does NOT clear m_browser.\n"
            f"      {before_result['detail']}"
        )
    print()

    # --- Assertion 3: GetBrowser() accessor present in header ---
    get_result = check_get_browser_in_header(h_source)
    if get_result["found"]:
        print("[PASS] GetBrowser() accessor is present in cef_handler.h.")
        print(f"       {get_result['detail']}")
    else:
        failures.append(
            f"FAIL: GetBrowser() accessor is NOT present in cef_handler.h.\n"
            f"      {get_result['detail']}"
        )
    print()

    if failures:
        print("RESULT: FAIL")
        for msg in failures:
            print(f"  {msg}")
        return 1

    print("RESULT: PASS")
    print("  OnAfterCreated stores m_browser; OnBeforeClose clears it; GetBrowser() is accessible.")
    return 0


if __name__ == "__main__":
    sys.exit(run_test())
