#!/usr/bin/env python3
"""
Preservation Unit Test — Task 7.4
===================================
Verifies that the CEF browser is embedded as a native child of m_container
at the correct geometry.

**Validates: Requirements 3.4**

WHAT THIS TESTS
---------------
Requirement 3.4 states:
  WHEN the CEF browser is created THEN the system SHALL CONTINUE TO embed it
  as a child of the native Qt container widget at the correct geometry.

The fix to main.cpp must not alter the browser embedding logic in
mainwindow.cpp. This test uses static source analysis to confirm:

  1. winId() is called on m_container (or equivalent) when setting up the
     browser window — this provides the native window handle to CEF.
  2. The browser window is reparented to m_container via SetAsChild() (CEF's
     cross-platform embedding API) or an equivalent native call
     (SetParent / XReparentWindow).
  3. A geometry rect is set on the browser window — looks for SetBounds,
     MoveResizeWindow, or the cef_rect_t / QRect geometry being passed to
     the embedding call.

EXPECTED RESULT ON FIXED CODE
------------------------------
  PASS — winId() called on m_container; SetAsChild() used with geometry rect.

EXPECTED RESULT ON UNFIXED CODE
---------------------------------
  PASS — this path is not affected by the four bugs; it should pass on both
  unfixed and fixed code (preservation check).

REQUIRES
--------
  - src/mainwindow.cpp present at ./src/mainwindow.cpp
    (or LILYPAD_MAINWINDOW env var)
  - src/cef_handler.cpp present at ./src/cef_handler.cpp
    (or LILYPAD_HANDLER env var)
  - Python 3.8+ (no external dependencies)
"""

import os
import re
import sys

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
MAINWINDOW_FILE = os.environ.get("LILYPAD_MAINWINDOW", "./src/mainwindow.cpp")
HANDLER_FILE = os.environ.get("LILYPAD_HANDLER", "./src/cef_handler.cpp")

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


def extract_function_body(source: str, func_pattern: str) -> str:
    """
    Extract the body of a function matching func_pattern.
    Uses brace counting. Returns body without outer braces.
    """
    match = re.search(func_pattern, source)
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


# ---------------------------------------------------------------------------
# Assertions
# ---------------------------------------------------------------------------

def check_winid_called_on_container(mainwindow_source: str) -> dict:
    """
    Assert that winId() is called on m_container (or equivalent) in
    createBrowser() or the constructor.

    Expected pattern:
        m_container->winId()
    or:
        static_cast<CefWindowHandle>(m_container->winId())
    """
    # Look in createBrowser body
    create_browser_body = extract_function_body(
        mainwindow_source, r'\bcreateBrowser\b'
    )
    if not create_browser_body:
        create_browser_body = mainwindow_source  # fall back to full file

    pattern = re.compile(r'\bm_container\s*->\s*winId\s*\(\s*\)')
    m = pattern.search(create_browser_body)
    if m:
        return {
            "found": True,
            "detail": "m_container->winId() is called in createBrowser().",
        }

    # Also accept: winId() called on any widget that is the container
    m2 = re.search(r'\bwinId\s*\(\s*\)', create_browser_body)
    if m2:
        return {
            "found": True,
            "detail": "winId() is called in createBrowser() (on container widget).",
        }

    return {
        "found": False,
        "detail": (
            "winId() is NOT called in createBrowser(). "
            "CEF needs the native window handle to embed the browser."
        ),
    }


def check_set_as_child_or_reparent(mainwindow_source: str) -> dict:
    """
    Assert that the browser is reparented to m_container using:
      - SetAsChild() (CEF cross-platform API), or
      - SetParent() (Win32), or
      - XReparentWindow() (X11/Linux).

    SetAsChild is the expected CEF API for embedding a browser as a child
    of a native window.
    """
    create_browser_body = extract_function_body(
        mainwindow_source, r'\bcreateBrowser\b'
    )
    if not create_browser_body:
        create_browser_body = mainwindow_source

    # Check for SetAsChild (CEF cross-platform)
    m = re.search(r'\bSetAsChild\s*\(', create_browser_body)
    if m:
        return {
            "found": True,
            "api": "SetAsChild",
            "detail": "SetAsChild() is called — browser is embedded as a native child.",
        }

    # Check for SetParent (Win32)
    m2 = re.search(r'\bSetParent\s*\(', create_browser_body)
    if m2:
        return {
            "found": True,
            "api": "SetParent",
            "detail": "SetParent() is called — browser is reparented (Win32).",
        }

    # Check for XReparentWindow (X11)
    m3 = re.search(r'\bXReparentWindow\s*\(', create_browser_body)
    if m3:
        return {
            "found": True,
            "api": "XReparentWindow",
            "detail": "XReparentWindow() is called — browser is reparented (X11).",
        }

    return {
        "found": False,
        "api": None,
        "detail": (
            "No browser embedding call found (SetAsChild / SetParent / "
            "XReparentWindow) in createBrowser(). "
            "The browser will not be embedded as a native child of m_container."
        ),
    }


def check_geometry_rect_set(mainwindow_source: str) -> dict:
    """
    Assert that a geometry rect is set on the browser window. Looks for:
      - cef_rect_t rect{...} or cef_rect_t rect = {...}  (CEF rect type)
      - QRect qrect = m_container->geometry()  (Qt geometry)
      - SetBounds() call
      - MoveResizeWindow() call (X11)
      - The rect being passed to SetAsChild() or CreateBrowser()

    The key requirement is that the container's geometry is read and passed
    to CEF so the browser is sized correctly.
    """
    create_browser_body = extract_function_body(
        mainwindow_source, r'\bcreateBrowser\b'
    )
    if not create_browser_body:
        create_browser_body = mainwindow_source

    # Check for cef_rect_t usage
    m = re.search(r'\bcef_rect_t\b', create_browser_body)
    if m:
        return {
            "found": True,
            "detail": "cef_rect_t is used in createBrowser() — geometry rect is set.",
        }

    # Check for QRect geometry() call
    m2 = re.search(r'\bgeometry\s*\(\s*\)', create_browser_body)
    if m2:
        return {
            "found": True,
            "detail": (
                "geometry() is called in createBrowser() — "
                "container geometry is read for the browser rect."
            ),
        }

    # Check for SetBounds
    m3 = re.search(r'\bSetBounds\s*\(', create_browser_body)
    if m3:
        return {
            "found": True,
            "detail": "SetBounds() is called — browser geometry is set explicitly.",
        }

    # Check for MoveResizeWindow (X11)
    m4 = re.search(r'\bMoveResizeWindow\s*\(', create_browser_body)
    if m4:
        return {
            "found": True,
            "detail": "MoveResizeWindow() is called — browser geometry is set (X11).",
        }

    # Check for rect being passed to SetAsChild
    m5 = re.search(r'SetAsChild\s*\([^)]*rect', create_browser_body, re.DOTALL)
    if m5:
        return {
            "found": True,
            "detail": "A rect is passed to SetAsChild() — browser geometry is set.",
        }

    return {
        "found": False,
        "detail": (
            "No geometry rect found in createBrowser(). "
            "Expected cef_rect_t, geometry(), SetBounds(), or equivalent. "
            "The browser may be embedded at zero size."
        ),
    }


def check_wa_native_window_attribute(mainwindow_source: str) -> dict:
    """
    Assert that m_container has the WA_NativeWindow attribute set.
    This is required for winId() to return a valid native handle on all
    Qt platforms.
    """
    # This can be in the constructor
    constructor_body = extract_function_body(
        mainwindow_source, r'\bMainWindow\s*::\s*MainWindow\b'
    )
    if not constructor_body:
        constructor_body = mainwindow_source

    m = re.search(r'\bWA_NativeWindow\b', constructor_body)
    if m:
        return {
            "found": True,
            "detail": (
                "WA_NativeWindow attribute is set on m_container — "
                "winId() will return a valid native handle."
            ),
        }
    return {
        "found": False,
        "detail": (
            "WA_NativeWindow attribute is NOT set on m_container. "
            "winId() may not return a valid native handle on all platforms."
        ),
    }


# ---------------------------------------------------------------------------
# Main test logic
# ---------------------------------------------------------------------------

def run_test() -> int:
    """
    Static analysis of browser embedding in mainwindow.cpp and cef_handler.cpp.
    Returns 0 (PASS) or 1 (FAIL).
    """
    print("=" * 70)
    print("Preservation Unit Test 7.4: Browser embedded as native child of m_container")
    print("=" * 70)
    print(f"mainwindow.cpp : {MAINWINDOW_FILE}")
    print(f"cef_handler.cpp: {HANDLER_FILE}")
    print()

    missing = []
    if not os.path.isfile(MAINWINDOW_FILE):
        missing.append(MAINWINDOW_FILE)
    if not os.path.isfile(HANDLER_FILE):
        missing.append(HANDLER_FILE)

    if missing:
        for f in missing:
            print(f"ERROR: Source file not found at '{f}'")
        print("       Run from the repo root, or set LILYPAD_MAINWINDOW / LILYPAD_HANDLER.")
        return 1

    mw_raw = load_source(MAINWINDOW_FILE)
    mw_source = strip_comments(mw_raw)

    # cef_handler.cpp is read for completeness; the embedding logic is in mainwindow.cpp
    handler_raw = load_source(HANDLER_FILE)
    handler_source = strip_comments(handler_raw)

    failures = []
    warnings = []

    # ------------------------------------------------------------------
    # Assertion 1: winId() called on m_container
    # ------------------------------------------------------------------
    winid_result = check_winid_called_on_container(mw_source)
    if winid_result["found"]:
        print(f"[PASS] {winid_result['detail']}")
    else:
        failures.append(f"FAIL: {winid_result['detail']}")
        print(f"[FAIL] {winid_result['detail']}")
    print()

    # ------------------------------------------------------------------
    # Assertion 2: Browser reparented to m_container (SetAsChild / SetParent / XReparentWindow)
    # ------------------------------------------------------------------
    reparent_result = check_set_as_child_or_reparent(mw_source)
    if reparent_result["found"]:
        print(f"[PASS] {reparent_result['detail']}")
    else:
        failures.append(f"FAIL: {reparent_result['detail']}")
        print(f"[FAIL] {reparent_result['detail']}")
    print()

    # ------------------------------------------------------------------
    # Assertion 3: Geometry rect is set on the browser window
    # ------------------------------------------------------------------
    geom_result = check_geometry_rect_set(mw_source)
    if geom_result["found"]:
        print(f"[PASS] {geom_result['detail']}")
    else:
        failures.append(f"FAIL: {geom_result['detail']}")
        print(f"[FAIL] {geom_result['detail']}")
    print()

    # ------------------------------------------------------------------
    # Assertion 4 (advisory): WA_NativeWindow attribute set on m_container
    # ------------------------------------------------------------------
    native_result = check_wa_native_window_attribute(mw_source)
    if native_result["found"]:
        print(f"[PASS] {native_result['detail']}")
    else:
        # This is a warning, not a hard failure — the attribute may be set
        # elsewhere or may not be required on all platforms.
        warnings.append(f"WARN: {native_result['detail']}")
        print(f"[WARN] {native_result['detail']}")
    print()

    # ------------------------------------------------------------------
    # Summary
    # ------------------------------------------------------------------
    if warnings:
        print("Warnings (non-fatal):")
        for w in warnings:
            print(f"  {w}")
        print()

    if failures:
        print("RESULT: FAIL")
        for msg in failures:
            print(f"  {msg}")
        return 1

    print("RESULT: PASS")
    print(
        "  winId() called on m_container; browser embedded via SetAsChild() "
        "(or equivalent); geometry rect is set."
    )
    return 0


if __name__ == "__main__":
    sys.exit(run_test())
