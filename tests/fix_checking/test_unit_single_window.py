#!/usr/bin/env python3
"""
Fix-Checking Unit Test — Task 8.2
===================================
Verifies that the application uses a single top-level window and that the
CEF browser is always embedded as a native child (not a floating popup).

WHAT THIS TESTS
---------------
Property 1 mandates that the browser is always embedded as a native child
window, never as a floating top-level window.

Requirement 1.1 mandates that the application presents a single unified
window to the user.

Requirement 1.3 mandates that the CEF browser view is embedded inside the
Qt window, not displayed as a separate top-level window.

Requirement 2.2 mandates that SetAsChild() is used to embed the browser.

Requirement 2.3 mandates that the browser's native window is a child of
the container's native window, not a child of the desktop root window.

This test uses static source analysis to confirm:
  1. SetAsChild() is used in mainwindow.cpp (not SetAsPopup() or other
     modes that would create a floating window).
  2. No additional QMainWindow instantiations exist outside of the
     MainWindow class definition itself.
  3. No additional top-level show() calls exist outside of the expected
     `w.show()` in main().

EXPECTED RESULT ON FIXED CODE
------------------------------
  PASS — SetAsChild() used, no extra QMainWindow instances, no extra
         top-level show() calls.

EXPECTED RESULT ON UNFIXED CODE
--------------------------------
  FAIL — SetAsPopup() used, extra QMainWindow created, or extra top-level
         show() calls found.

REQUIRES
--------
  - src/mainwindow.cpp present at ./src/mainwindow.cpp
    (or LILYPAD_MAINWINDOW env var)
  - src/main.cpp present at ./src/main.cpp
    (or LILYPAD_SOURCE env var)
  - Python 3.8+ (no external dependencies)

Validates: Requirements 1.1, 1.3, 2.2, 2.3
"""

import os
import re
import sys

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
MAINWINDOW_FILE = os.environ.get("LILYPAD_MAINWINDOW", "./src/mainwindow.cpp")
SOURCE_FILE = os.environ.get("LILYPAD_SOURCE", "./src/main.cpp")

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load_source(path: str) -> str:
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def strip_comments(text: str) -> str:
    """Remove C++ line comments and block comments from text."""
    # Remove block comments /* ... */
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)
    # Remove line comments // ...
    text = re.sub(r'//[^\n]*', '', text)
    return text


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


def extract_class_body(source: str, class_name: str) -> str:
    """
    Extract the body of a class definition from the source.
    Uses brace counting to find the full class body.
    """
    pattern = re.compile(
        r'\bclass\s+' + re.escape(class_name) + r'\b[^{]*\{',
        re.DOTALL,
    )
    match = pattern.search(source)
    if not match:
        return ""

    start = match.end() - 1  # position of the opening '{'
    depth = 0
    for i in range(start, len(source)):
        if source[i] == '{':
            depth += 1
        elif source[i] == '}':
            depth -= 1
            if depth == 0:
                return source[start:i + 1]
    return source[start:]


# ---------------------------------------------------------------------------
# Assertion functions
# ---------------------------------------------------------------------------

def check_set_as_child_not_popup(mainwindow_source: str) -> dict:
    """
    Assert that SetAsChild() is used in mainwindow.cpp and that
    SetAsPopup() is NOT used anywhere in the file.

    SetAsPopup() would create a floating top-level window instead of
    embedding the browser as a native child.
    """
    clean = strip_comments(mainwindow_source)

    # Check SetAsChild() is present
    has_set_as_child = bool(re.search(r'\bSetAsChild\s*\(', clean))

    # Check SetAsPopup() is absent
    popup_match = re.search(r'\bSetAsPopup\s*\(', clean)
    has_set_as_popup = popup_match is not None

    # Check for any other window mode calls that would create floating windows
    # (SetAsWindowless is for off-screen rendering, not a floating window, so we allow it)
    other_mode_match = re.search(r'\bSetAs(?!Child\b|Windowless\b)\w+\s*\(', clean)
    has_other_mode = other_mode_match is not None

    if not has_set_as_child:
        return {
            "found": False,
            "detail": (
                "SetAsChild() not found in mainwindow.cpp.\n"
                "          Without SetAsChild(), CEF will create a top-level window\n"
                "          instead of embedding inside m_container."
            ),
        }

    if has_set_as_popup:
        # Find context around the popup call
        ctx_start = max(0, popup_match.start() - 40)
        ctx_end = min(len(clean), popup_match.end() + 40)
        ctx = re.sub(r'\s+', ' ', clean[ctx_start:ctx_end]).strip()
        return {
            "found": False,
            "detail": (
                f"SetAsPopup() found in mainwindow.cpp — this creates a floating\n"
                f"          top-level window instead of embedding the browser.\n"
                f"          Context: ...{ctx}..."
            ),
        }

    if has_other_mode:
        ctx_start = max(0, other_mode_match.start() - 40)
        ctx_end = min(len(clean), other_mode_match.end() + 40)
        ctx = re.sub(r'\s+', ' ', clean[ctx_start:ctx_end]).strip()
        return {
            "found": False,
            "detail": (
                f"Unexpected window mode call found: '{other_mode_match.group(0).strip()}'\n"
                f"          Only SetAsChild() should be used for embedded browser windows.\n"
                f"          Context: ...{ctx}..."
            ),
        }

    return {
        "found": True,
        "detail": "SetAsChild() is used and SetAsPopup() is absent.",
    }


def check_no_extra_qmainwindow(mainwindow_source: str, main_source: str) -> dict:
    """
    Assert that no additional QMainWindow instantiations exist outside of
    the MainWindow class definition itself.

    Legitimate occurrences:
      - 'class MainWindow : public QMainWindow' in mainwindow.h / mainwindow.cpp
      - 'MainWindow(parent)' constructor initialiser in mainwindow.cpp
      - 'QMainWindow(parent)' base-class constructor call in mainwindow.cpp

    Illegitimate occurrences:
      - 'new QMainWindow' anywhere (creating an extra top-level window)
      - 'QMainWindow foo;' or 'QMainWindow foo(' variable declarations in main()
    """
    # Check mainwindow.cpp for unexpected QMainWindow instantiations
    # (outside of the class definition / constructor chain)
    mw_clean = strip_comments(mainwindow_source)
    main_clean = strip_comments(main_source)

    # Pattern: 'new QMainWindow' — always suspicious outside the class itself
    new_qmainwindow = re.compile(r'\bnew\s+QMainWindow\b')

    mw_new_match = new_qmainwindow.search(mw_clean)
    main_new_match = new_qmainwindow.search(main_clean)

    if mw_new_match:
        ctx_start = max(0, mw_new_match.start() - 40)
        ctx_end = min(len(mw_clean), mw_new_match.end() + 40)
        ctx = re.sub(r'\s+', ' ', mw_clean[ctx_start:ctx_end]).strip()
        return {
            "found": False,
            "detail": (
                f"'new QMainWindow' found in mainwindow.cpp — this creates an extra\n"
                f"          top-level window outside of the MainWindow class hierarchy.\n"
                f"          Context: ...{ctx}..."
            ),
        }

    if main_new_match:
        ctx_start = max(0, main_new_match.start() - 40)
        ctx_end = min(len(main_clean), main_new_match.end() + 40)
        ctx = re.sub(r'\s+', ' ', main_clean[ctx_start:ctx_end]).strip()
        return {
            "found": False,
            "detail": (
                f"'new QMainWindow' found in main.cpp — this creates an extra\n"
                f"          top-level window in addition to the expected MainWindow.\n"
                f"          Context: ...{ctx}..."
            ),
        }

    # Pattern: local QMainWindow variable declaration in main()
    # e.g. 'QMainWindow w2;' or 'QMainWindow w2('
    main_body = extract_main_body(main_source)
    main_body_clean = strip_comments(main_body)

    # Look for QMainWindow variable declarations that are NOT 'MainWindow' (our subclass)
    # i.e. bare 'QMainWindow' (not preceded by 'class' or followed by '::')
    local_decl = re.compile(r'(?<!class\s)\bQMainWindow\s+\w+\s*[;({]')
    local_match = local_decl.search(main_body_clean)

    if local_match:
        ctx_start = max(0, local_match.start() - 40)
        ctx_end = min(len(main_body_clean), local_match.end() + 40)
        ctx = re.sub(r'\s+', ' ', main_body_clean[ctx_start:ctx_end]).strip()
        return {
            "found": False,
            "detail": (
                f"Extra QMainWindow variable declaration found in main().\n"
                f"          Only one MainWindow (our subclass) should be instantiated.\n"
                f"          Context: ...{ctx}..."
            ),
        }

    return {
        "found": True,
        "detail": (
            "No extra QMainWindow instantiations found outside of the "
            "MainWindow class definition."
        ),
    }


def check_no_extra_toplevel_show(main_source: str) -> dict:
    """
    Assert that no additional top-level show() calls exist outside of the
    expected `w.show()` in main().

    The only legitimate show() call in main() is `w.show()` where `w` is
    the single MainWindow instance. Any other `<identifier>.show()` call
    on a widget variable would reveal a second top-level window being made
    visible.

    Strategy:
      1. Extract the main() body.
      2. Find all `<identifier>.show()` calls.
      3. Identify the MainWindow variable name (the one declared as
         'MainWindow <name>').
      4. Assert that only the MainWindow variable's show() call is present.
    """
    main_body = extract_main_body(main_source)
    if not main_body:
        return {
            "found": False,
            "detail": "Could not locate main() function body.",
        }

    clean = strip_comments(main_body)

    # Find the MainWindow variable name
    mw_decl = re.compile(r'\bMainWindow\s+(\w+)\b')
    mw_match = mw_decl.search(clean)
    if not mw_match:
        return {
            "found": False,
            "detail": (
                "Could not find a MainWindow variable declaration in main().\n"
                "          Expected something like 'MainWindow w;'."
            ),
        }

    mainwindow_var = mw_match.group(1)

    # Find all <identifier>.show() calls
    show_pattern = re.compile(r'\b(\w+)\s*\.\s*show\s*\(\s*\)')
    show_matches = list(show_pattern.finditer(clean))

    if not show_matches:
        return {
            "found": False,
            "detail": (
                f"No show() calls found in main(). "
                f"Expected '{mainwindow_var}.show()' to make the window visible."
            ),
        }

    # Check that all show() calls are on the MainWindow variable
    extra_shows = [
        m for m in show_matches
        if m.group(1) != mainwindow_var
    ]

    if extra_shows:
        details = []
        for m in extra_shows:
            ctx_start = max(0, m.start() - 40)
            ctx_end = min(len(clean), m.end() + 40)
            ctx = re.sub(r'\s+', ' ', clean[ctx_start:ctx_end]).strip()
            details.append(f"'{m.group(0)}' — context: ...{ctx}...")
        return {
            "found": False,
            "detail": (
                f"Extra top-level show() call(s) found in main() beyond "
                f"'{mainwindow_var}.show()':\n"
                + "\n          ".join(details)
            ),
        }

    # Confirm the expected w.show() is present
    expected_shows = [m for m in show_matches if m.group(1) == mainwindow_var]
    return {
        "found": True,
        "detail": (
            f"Only '{mainwindow_var}.show()' found in main() — "
            f"no extra top-level show() calls."
        ),
    }


# ---------------------------------------------------------------------------
# Main test logic
# ---------------------------------------------------------------------------

def run_test() -> int:
    """
    Static analysis of mainwindow.cpp and main.cpp.
    Returns 0 (PASS) or 1 (FAIL).
    """
    print("=" * 70)
    print("Fix-Checking Unit Test 8.2: Single top-level window (Property 1)")
    print("=" * 70)
    print(f"mainwindow.cpp: {MAINWINDOW_FILE}")
    print(f"main.cpp:       {SOURCE_FILE}")
    print()

    if not os.path.isfile(MAINWINDOW_FILE):
        print(f"ERROR: mainwindow.cpp not found at '{MAINWINDOW_FILE}'")
        print("       Run from the repo root, or set LILYPAD_MAINWINDOW env var.")
        return 1

    if not os.path.isfile(SOURCE_FILE):
        print(f"ERROR: main.cpp not found at '{SOURCE_FILE}'")
        print("       Run from the repo root, or set LILYPAD_SOURCE env var.")
        return 1

    mainwindow_source = load_source(MAINWINDOW_FILE)
    main_source = load_source(SOURCE_FILE)

    failures = []

    # --- Assertion 1: SetAsChild() used, not SetAsPopup() ---
    result = check_set_as_child_not_popup(mainwindow_source)
    if result["found"]:
        print("[PASS] SetAsChild() is used (not SetAsPopup() or other floating-window modes).")
        print(f"       {result['detail']}")
    else:
        failures.append(
            f"FAIL: Incorrect window embedding mode in mainwindow.cpp.\n"
            f"      {result['detail']}"
        )
    print()

    # --- Assertion 2: No extra QMainWindow instantiations ---
    result = check_no_extra_qmainwindow(mainwindow_source, main_source)
    if result["found"]:
        print("[PASS] No extra QMainWindow instantiations found outside MainWindow class.")
        print(f"       {result['detail']}")
    else:
        failures.append(
            f"FAIL: Extra QMainWindow instantiation detected.\n"
            f"      {result['detail']}"
        )
    print()

    # --- Assertion 3: No extra top-level show() calls ---
    result = check_no_extra_toplevel_show(main_source)
    if result["found"]:
        print("[PASS] No extra top-level show() calls found in main().")
        print(f"       {result['detail']}")
    else:
        failures.append(
            f"FAIL: Extra top-level show() call(s) detected.\n"
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
        "  SetAsChild() used for native embedding, no extra QMainWindow instances,\n"
        "  and no extra top-level show() calls beyond the expected w.show()."
    )
    return 0


if __name__ == "__main__":
    sys.exit(run_test())
