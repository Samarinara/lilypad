#!/usr/bin/env python3
"""
Fix-Checking Unit Test — Task 1.2
===================================
Verifies that WA_NativeWindow is set on m_container and that both
m_urlBar and m_container are added to a vertical layout in the constructor.

WHAT THIS TESTS
---------------
Requirement 2.1 mandates that the container widget has a native OS window
handle so that CEF can embed into it via winId(). This requires Qt's
WA_NativeWindow attribute to be set on m_container.

Requirement 2.4 mandates that the URL bar and the browser container are
arranged in a vertical layout (URL bar on top, container below).

This test uses static source analysis to confirm:
  1. setAttribute(Qt::WA_NativeWindow) is called on m_container inside
     the MainWindow constructor.
  2. m_urlBar is added to a vertical layout via addWidget.
  3. m_container is added to a vertical layout via addWidget.

EXPECTED RESULT ON FIXED CODE
------------------------------
  PASS — WA_NativeWindow is set; both widgets are added via addWidget.

EXPECTED RESULT ON UNFIXED CODE
--------------------------------
  FAIL — WA_NativeWindow is absent, or one of the addWidget calls is missing.

REQUIRES
--------
  - src/mainwindow.cpp present at ./src/mainwindow.cpp
    (or LILYPAD_MAINWINDOW env var)
  - Python 3.8+ (no external dependencies)

Validates: Requirements 2.1, 2.4
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


def non_comment_lines(text: str) -> list:
    """Return lines that are not pure C++ line comments or block comment lines."""
    result = []
    for line in text.splitlines():
        stripped = line.strip()
        if not stripped.startswith("//") and not stripped.startswith("*"):
            result.append(line)
    return result


def check_wa_native_window(constructor_body: str) -> dict:
    """
    Check that setAttribute(Qt::WA_NativeWindow) is called on m_container
    inside the constructor body.

    Accepts patterns like:
      m_container->setAttribute(Qt::WA_NativeWindow)
      m_container->setAttribute( Qt::WA_NativeWindow )
    """
    clean_lines = non_comment_lines(constructor_body)
    clean_text = "\n".join(clean_lines)

    pattern = re.compile(
        r'm_container\s*->\s*setAttribute\s*\(\s*Qt\s*::\s*WA_NativeWindow\s*\)',
    )
    match = pattern.search(clean_text)
    if match:
        return {
            "found": True,
            "detail": f"Found: {match.group(0).strip()}",
        }
    return {
        "found": False,
        "detail": (
            "m_container->setAttribute(Qt::WA_NativeWindow) not found in constructor.\n"
            "          Without this, Qt may not allocate a native OS window handle,\n"
            "          causing CEF's winId() embedding to fail."
        ),
    }


def check_add_widget(constructor_body: str, widget_name: str) -> dict:
    """
    Check that addWidget is called with the given widget name inside the
    constructor body (indicating it is added to a layout).

    Accepts patterns like:
      lay->addWidget(m_urlBar)
      lay->addWidget(m_container, 1)
      layout->addWidget(m_urlBar)
    """
    clean_lines = non_comment_lines(constructor_body)
    clean_text = "\n".join(clean_lines)

    # Match any layout variable calling addWidget with the target widget as
    # the first argument (optional trailing arguments like stretch factor are OK)
    pattern = re.compile(
        r'\w+\s*->\s*addWidget\s*\(\s*' + re.escape(widget_name) + r'\s*[,)]',
    )
    match = pattern.search(clean_text)
    if match:
        return {
            "found": True,
            "detail": f"Found: {match.group(0).strip()}",
        }
    return {
        "found": False,
        "detail": (
            f"addWidget({widget_name}) not found in constructor.\n"
            f"          {widget_name} must be added to the vertical layout."
        ),
    }


# ---------------------------------------------------------------------------
# Main test logic
# ---------------------------------------------------------------------------

def run_test() -> int:
    """
    Static analysis of MainWindow constructor.
    Returns 0 (PASS) or 1 (FAIL).
    """
    print("=" * 70)
    print("Fix-Checking Unit Test 1.2: WA_NativeWindow and layout structure")
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

    failures = []

    # --- Assertion 1: WA_NativeWindow set on m_container ---
    wa_result = check_wa_native_window(constructor_body)
    if wa_result["found"]:
        print(f"[PASS] WA_NativeWindow is set on m_container.")
        print(f"       {wa_result['detail']}")
    else:
        failures.append(
            f"FAIL: WA_NativeWindow is NOT set on m_container.\n"
            f"      {wa_result['detail']}"
        )
    print()

    # --- Assertion 2: m_urlBar added to layout ---
    url_result = check_add_widget(constructor_body, "m_urlBar")
    if url_result["found"]:
        print(f"[PASS] m_urlBar is added to the layout via addWidget.")
        print(f"       {url_result['detail']}")
    else:
        failures.append(
            f"FAIL: m_urlBar is NOT added to the layout.\n"
            f"      {url_result['detail']}"
        )
    print()

    # --- Assertion 3: m_container added to layout ---
    container_result = check_add_widget(constructor_body, "m_container")
    if container_result["found"]:
        print(f"[PASS] m_container is added to the layout via addWidget.")
        print(f"       {container_result['detail']}")
    else:
        failures.append(
            f"FAIL: m_container is NOT added to the layout.\n"
            f"      {container_result['detail']}"
        )
    print()

    if failures:
        print("RESULT: FAIL")
        for msg in failures:
            print(f"  {msg}")
        return 1

    print("RESULT: PASS")
    print("  WA_NativeWindow is set on m_container; both widgets are in the layout.")
    return 0


if __name__ == "__main__":
    sys.exit(run_test())
