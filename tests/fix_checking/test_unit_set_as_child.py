#!/usr/bin/env python3
"""
Fix-Checking Unit Test — Task 2.3
===================================
Verifies that SetAsChild() is called in createBrowser() with
m_container->winId() as the first argument and a cef_rect_t built from
m_container->geometry() dimensions as the second argument.

WHAT THIS TESTS
---------------
Requirement 2.2 mandates that when createBrowser() is called, the
CefHandler invokes window_info.SetAsChild() with the Container's native
window handle (winId()) and a Geometry Rect derived from the Container's
current dimensions.

Requirement 2.3 mandates that the Browser's native window is a child of
the Container's native window, not a child of the desktop root window.
This is achieved by passing m_container->winId() to SetAsChild().

This test uses static source analysis to confirm:
  1. SetAsChild() is called inside the createBrowser() function body.
  2. The first argument to SetAsChild() is m_container->winId()
     (possibly via a cast such as static_cast<CefWindowHandle>(...)).
  3. A cef_rect_t is constructed using m_container->geometry().width()
     and m_container->geometry().height() (or equivalent QRect accessors).
  4. The cef_rect_t variable is passed as the second argument to SetAsChild().

EXPECTED RESULT ON FIXED CODE
------------------------------
  PASS — SetAsChild() is called with winId() and a geometry-derived rect.

EXPECTED RESULT ON UNFIXED CODE
--------------------------------
  FAIL — SetAsChild() is absent, winId() is not used, or the rect is not
  derived from m_container->geometry().

REQUIRES
--------
  - src/mainwindow.cpp present at ./src/mainwindow.cpp
    (or LILYPAD_MAINWINDOW env var)
  - Python 3.8+ (no external dependencies)

Validates: Requirements 2.2, 2.3
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


def extract_function_body(source: str, function_name: str) -> str:
    """
    Extract the body of a named function (ClassName::functionName or just
    functionName) from the source. Uses brace counting to find the full body.
    """
    # Match both qualified (Foo::bar) and unqualified (bar) forms
    pattern = re.compile(
        r'\b' + re.escape(function_name) + r'\b\s*\([^)]*\)\s*\{',
        re.DOTALL,
    )
    match = pattern.search(source)
    if not match:
        return ""

    # The opening brace is the last character of the match
    start = match.end() - 1
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


def check_set_as_child_called(body: str) -> dict:
    """
    Check that SetAsChild() is called somewhere in the function body.
    """
    clean_text = "\n".join(non_comment_lines(body))
    pattern = re.compile(r'\bSetAsChild\s*\(')
    match = pattern.search(clean_text)
    if match:
        # Extract the full call for reporting (up to the closing paren on the same line)
        line_start = clean_text.rfind('\n', 0, match.start()) + 1
        line_end = clean_text.find('\n', match.end())
        snippet = clean_text[line_start: line_end if line_end != -1 else len(clean_text)].strip()
        return {"found": True, "detail": f"Found: {snippet}"}
    return {
        "found": False,
        "detail": (
            "SetAsChild() not found in createBrowser().\n"
            "          Without SetAsChild(), CEF will create a top-level window\n"
            "          instead of embedding inside m_container."
        ),
    }


def check_win_id_argument(body: str) -> dict:
    """
    Check that SetAsChild() receives m_container->winId() as its first argument.

    Accepts patterns like:
      window_info.SetAsChild(m_container->winId(), ...)
      window_info.SetAsChild(static_cast<CefWindowHandle>(m_container->winId()), ...)
      window_info.SetAsChild((CefWindowHandle)m_container->winId(), ...)
    """
    clean_text = "\n".join(non_comment_lines(body))

    # Pattern: SetAsChild( [optional cast] m_container->winId() , ...
    pattern = re.compile(
        r'\bSetAsChild\s*\('
        r'\s*(?:static_cast\s*<[^>]+>\s*\()?'   # optional static_cast<...>(
        r'\s*(?:\([^)]*\)\s*)?'                  # optional C-style cast
        r'\s*m_container\s*->\s*winId\s*\(\s*\)',
    )
    match = pattern.search(clean_text)
    if match:
        return {"found": True, "detail": f"Found winId() as first argument to SetAsChild()."}
    return {
        "found": False,
        "detail": (
            "m_container->winId() not found as the first argument of SetAsChild().\n"
            "          The native window handle of m_container must be passed so that\n"
            "          CEF embeds the browser as a child of the container widget."
        ),
    }


def check_cef_rect_from_geometry(body: str) -> dict:
    """
    Check that a cef_rect_t is constructed using m_container->geometry()
    dimensions (width() and height()).

    Accepts patterns like:
      cef_rect_t rect{0, 0, qrect.width(), qrect.height()};
      cef_rect_t rect{0, 0, m_container->geometry().width(), ...};
      cef_rect_t rect = {0, 0, w, h};  (where w/h come from geometry())

    Strategy:
      1. Confirm cef_rect_t is declared in the function body.
      2. Confirm that geometry().width() and geometry().height() (or
         equivalent QRect variable accessors) are used somewhere in the
         function body near the rect construction.
    """
    clean_text = "\n".join(non_comment_lines(body))

    # Check cef_rect_t is declared
    rect_decl = re.compile(r'\bcef_rect_t\b')
    if not rect_decl.search(clean_text):
        return {
            "found": False,
            "detail": (
                "cef_rect_t not declared in createBrowser().\n"
                "          A cef_rect_t must be constructed to pass geometry to SetAsChild()."
            ),
        }

    # Check that geometry().width() and geometry().height() are referenced.
    # They may be accessed directly on m_container or via an intermediate QRect variable.
    width_pattern = re.compile(r'(?:geometry\s*\(\s*\)\s*\.\s*width\s*\(\s*\)|\.width\s*\(\s*\))')
    height_pattern = re.compile(r'(?:geometry\s*\(\s*\)\s*\.\s*height\s*\(\s*\)|\.height\s*\(\s*\))')

    width_match = width_pattern.search(clean_text)
    height_match = height_pattern.search(clean_text)

    if width_match and height_match:
        return {
            "found": True,
            "detail": (
                "cef_rect_t is declared and geometry().width() / .height() are used."
            ),
        }

    missing = []
    if not width_match:
        missing.append("width()")
    if not height_match:
        missing.append("height()")

    return {
        "found": False,
        "detail": (
            f"cef_rect_t declared but geometry dimension(s) not found: {', '.join(missing)}.\n"
            "          The rect must be built from m_container->geometry() dimensions\n"
            "          so that the CEF viewport matches the container size."
        ),
    }


def extract_set_as_child_args(body: str) -> str:
    """
    Extract the full argument string of the SetAsChild() call, handling
    nested parentheses (e.g. static_cast<...>(...), winId()).

    Returns the content between the outermost parentheses of SetAsChild(...)
    or an empty string if not found.
    """
    clean_text = "\n".join(non_comment_lines(body))
    match = re.search(r'\bSetAsChild\s*\(', clean_text)
    if not match:
        return ""

    start = match.end()  # position just after the opening '('
    depth = 1
    for i in range(start, len(clean_text)):
        if clean_text[i] == '(':
            depth += 1
        elif clean_text[i] == ')':
            depth -= 1
            if depth == 0:
                return clean_text[start:i]
    return clean_text[start:]


def check_rect_passed_to_set_as_child(body: str) -> dict:
    """
    Check that the cef_rect_t variable is passed as the second argument to
    SetAsChild().

    Strategy:
      1. Find the name of the cef_rect_t variable declared in the body.
      2. Extract the full argument list of SetAsChild() (handling nested parens).
      3. Confirm the rect variable name appears in those arguments.

    Accepts patterns like:
      cef_rect_t rect{...};
      window_info.SetAsChild(..., rect);
    """
    clean_text = "\n".join(non_comment_lines(body))

    # Find the cef_rect_t variable name
    decl_pattern = re.compile(r'\bcef_rect_t\s+(\w+)\b')
    decl_match = decl_pattern.search(clean_text)
    if not decl_match:
        return {
            "found": False,
            "detail": "cef_rect_t variable not found; cannot verify it is passed to SetAsChild().",
        }

    rect_var = decl_match.group(1)

    # Extract the full argument list of SetAsChild(), respecting nested parens
    args = extract_set_as_child_args(body)
    if not args:
        return {
            "found": False,
            "detail": "SetAsChild() call not found; cannot verify rect argument.",
        }

    # Check that the rect variable name appears in the argument list
    if re.search(r'\b' + re.escape(rect_var) + r'\b', args):
        return {
            "found": True,
            "detail": f"cef_rect_t variable '{rect_var}' is passed to SetAsChild().",
        }
    return {
        "found": False,
        "detail": (
            f"cef_rect_t variable '{rect_var}' is NOT passed to SetAsChild().\n"
            "          The geometry rect must be the second argument to SetAsChild()\n"
            "          so that CEF sizes the browser viewport to match the container."
        ),
    }


# ---------------------------------------------------------------------------
# Main test logic
# ---------------------------------------------------------------------------

def run_test() -> int:
    """
    Static analysis of MainWindow::createBrowser().
    Returns 0 (PASS) or 1 (FAIL).
    """
    print("=" * 70)
    print("Fix-Checking Unit Test 2.3: SetAsChild() with winId() and geometry rect")
    print("=" * 70)
    print(f"Source file: {SOURCE_FILE}")
    print()

    if not os.path.isfile(SOURCE_FILE):
        print(f"ERROR: Source file not found at '{SOURCE_FILE}'")
        print("       Run from the repo root, or set LILYPAD_MAINWINDOW env var.")
        return 1

    source = load_source(SOURCE_FILE)
    function_body = extract_function_body(source, "createBrowser")

    if not function_body:
        print("ERROR: Could not locate createBrowser() function in source.")
        return 1

    failures = []

    # --- Assertion 1: SetAsChild() is called ---
    result = check_set_as_child_called(function_body)
    if result["found"]:
        print(f"[PASS] SetAsChild() is called in createBrowser().")
        print(f"       {result['detail']}")
    else:
        failures.append(
            f"FAIL: SetAsChild() is NOT called in createBrowser().\n"
            f"      {result['detail']}"
        )
    print()

    # --- Assertion 2: m_container->winId() is the first argument ---
    result = check_win_id_argument(function_body)
    if result["found"]:
        print(f"[PASS] m_container->winId() is passed as the first argument to SetAsChild().")
        print(f"       {result['detail']}")
    else:
        failures.append(
            f"FAIL: m_container->winId() is NOT the first argument of SetAsChild().\n"
            f"      {result['detail']}"
        )
    print()

    # --- Assertion 3: cef_rect_t is built from m_container->geometry() ---
    result = check_cef_rect_from_geometry(function_body)
    if result["found"]:
        print(f"[PASS] cef_rect_t is constructed from m_container->geometry() dimensions.")
        print(f"       {result['detail']}")
    else:
        failures.append(
            f"FAIL: cef_rect_t is NOT constructed from m_container->geometry() dimensions.\n"
            f"      {result['detail']}"
        )
    print()

    # --- Assertion 4: cef_rect_t variable is passed to SetAsChild() ---
    result = check_rect_passed_to_set_as_child(function_body)
    if result["found"]:
        print(f"[PASS] cef_rect_t variable is passed to SetAsChild().")
        print(f"       {result['detail']}")
    else:
        failures.append(
            f"FAIL: cef_rect_t variable is NOT passed to SetAsChild().\n"
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
        "  SetAsChild() is called with m_container->winId() and a cef_rect_t\n"
        "  built from m_container->geometry() dimensions."
    )
    return 0


if __name__ == "__main__":
    sys.exit(run_test())
