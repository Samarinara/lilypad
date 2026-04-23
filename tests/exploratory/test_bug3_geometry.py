#!/usr/bin/env python3
"""
Exploratory Test — Bug 3: createBrowser() called before widget geometry is valid
=================================================================================

WHAT THIS TESTS
---------------
QWidget::show() posts a paint event but does NOT process it synchronously.
This means m_container->geometry() returns QRect(0,0,0,0) at the point
createBrowser() reads it immediately after show() returns.

CEF then embeds the browser at zero size, and the page is never rendered visibly.

The fix is to defer createBrowser() via QTimer::singleShot(0, ...), which posts
a zero-delay callback that fires AFTER the event loop has processed the pending
show/layout/paint events, guaranteeing that m_container->geometry() returns a
non-zero rect.

APPROACH
--------
Since we cannot easily inspect runtime geometry without modifying source code or
attaching a debugger, this test uses STATIC SOURCE ANALYSIS of src/main.cpp to
detect the bug condition:

  Bug condition: createBrowser() is called in the same function scope as show(),
                 WITHOUT an intervening QTimer::singleShot deferral.

This is a reliable proxy for the runtime bug because:
  1. The only place createBrowser() is called is in main().
  2. If it is called synchronously after show() (no QTimer::singleShot), the
     geometry will always be (0,0,0,0) at that point.
  3. If it is wrapped in QTimer::singleShot(0, ...), the geometry will be valid.

EXPECTED RESULT ON UNFIXED CODE
--------------------------------
  FAIL — BUG CONFIRMED: createBrowser() is called synchronously after show()
  without QTimer::singleShot deferral.
  This is the SUCCESS case for this exploratory test.

EXPECTED RESULT AFTER FIX
--------------------------
  PASS — createBrowser() is deferred via QTimer::singleShot.

REQUIRES
--------
  - src/main.cpp present at ../../src/main.cpp (or SOURCE_FILE env var)
  - Python 3.8+ (no external dependencies)
"""

import os
import re
import sys

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
# Path to main.cpp, relative to the repo root (where this script is run from)
SOURCE_FILE = os.environ.get("LILYPAD_SOURCE", "./src/main.cpp")

# ---------------------------------------------------------------------------
# Analysis helpers
# ---------------------------------------------------------------------------

def load_source(path: str) -> str:
    """Load and return the source file contents."""
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def find_main_function_body(source: str) -> str:
    """
    Extract the body of the main() function from the source.
    Uses a simple brace-counting approach — sufficient for this well-structured file.
    """
    # Find the start of main()
    main_match = re.search(r'\bint\s+main\s*\(', source)
    if not main_match:
        return ""

    # Walk forward from the opening brace to find the matching closing brace
    start = source.find('{', main_match.end())
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
    return source[start:]  # unterminated — return rest


def check_cef_execute_process(source: str) -> dict:
    """Check whether CefExecuteProcess() is called before QApplication."""
    has_cef_execute = bool(re.search(r'\bCefExecuteProcess\s*\(', source))
    has_qapplication = bool(re.search(r'\bQApplication\b', source))

    if not has_cef_execute:
        return {
            "present": False,
            "detail": "CefExecuteProcess() is NOT called anywhere in main.cpp",
        }

    # Check ordering: CefExecuteProcess must appear before QApplication
    cef_pos = source.find("CefExecuteProcess")
    qapp_pos = source.find("QApplication")
    if has_qapplication and cef_pos > qapp_pos:
        return {
            "present": True,
            "correct_order": False,
            "detail": "CefExecuteProcess() is called AFTER QApplication — wrong order",
        }
    return {
        "present": True,
        "correct_order": True,
        "detail": "CefExecuteProcess() is called before QApplication — correct",
    }


def check_create_browser_deferral(main_body: str) -> dict:
    """
    Check whether createBrowser() is deferred via QTimer::singleShot.

    Returns a dict with:
      - synchronous: True if createBrowser() is called synchronously after show()
      - deferred: True if createBrowser() is wrapped in QTimer::singleShot
      - detail: human-readable explanation
    """
    has_show = bool(re.search(r'\bshow\s*\(\s*\)', main_body))
    has_create_browser = bool(re.search(r'\bcreateBrowser\s*\(', main_body))
    has_qtimer_singleshot = bool(re.search(r'QTimer\s*::\s*singleShot', main_body))

    if not has_create_browser:
        return {
            "synchronous": False,
            "deferred": False,
            "detail": "createBrowser() not found in main() — cannot determine call site",
        }

    if not has_show:
        return {
            "synchronous": False,
            "deferred": False,
            "detail": "show() not found in main() — cannot determine call order",
        }

    # Check if createBrowser is inside a QTimer::singleShot lambda
    # Pattern: QTimer::singleShot(...) followed by createBrowser within the same lambda
    singleshot_with_browser = bool(re.search(
        r'QTimer\s*::\s*singleShot\s*\([^)]*\)\s*[,;]?\s*\[',
        main_body
    ))

    # More robust: check if createBrowser appears inside a lambda after singleShot
    singleshot_lambda_pattern = re.search(
        r'QTimer\s*::\s*singleShot\s*\(.*?\{.*?createBrowser.*?\}',
        main_body,
        re.DOTALL
    )

    if singleshot_lambda_pattern:
        return {
            "synchronous": False,
            "deferred": True,
            "detail": (
                "createBrowser() is deferred via QTimer::singleShot — CORRECT.\n"
                "  The browser will be created after the event loop processes layout,\n"
                "  ensuring m_container->geometry() returns a non-zero rect."
            ),
        }

    if has_qtimer_singleshot:
        # singleShot is present but createBrowser may not be inside it
        return {
            "synchronous": True,
            "deferred": False,
            "detail": (
                "QTimer::singleShot is present but createBrowser() does not appear\n"
                "  to be inside the lambda. createBrowser() may still be synchronous."
            ),
        }

    # No singleShot at all — createBrowser is synchronous
    # Confirm by checking relative positions
    show_pos = main_body.find("show()")
    browser_pos = main_body.find("createBrowser(")
    if show_pos != -1 and browser_pos != -1 and browser_pos > show_pos:
        return {
            "synchronous": True,
            "deferred": False,
            "detail": (
                "createBrowser() is called SYNCHRONOUSLY after show() — BUG.\n"
                "  m_container->geometry() will be QRect(0,0,0,0) at this point.\n"
                "  No QTimer::singleShot deferral found."
            ),
        }

    return {
        "synchronous": False,
        "deferred": False,
        "detail": "Could not determine call order — manual inspection required.",
    }


# ---------------------------------------------------------------------------
# Main test logic
# ---------------------------------------------------------------------------

def run_test() -> int:
    """
    Perform static analysis of src/main.cpp to confirm Bug 3.
    Returns 0 (PASS — bug not present) or 1 (FAIL — bug confirmed).
    """
    print("=" * 70)
    print("Bug 3 Exploratory Test: createBrowser() called before geometry is valid")
    print("=" * 70)
    print(f"Source file: {SOURCE_FILE}")
    print()

    # Load source
    if not os.path.isfile(SOURCE_FILE):
        print(f"ERROR: Source file not found at '{SOURCE_FILE}'")
        print("       Run from the repo root, or set LILYPAD_SOURCE env var.")
        return 2

    source = load_source(SOURCE_FILE)
    main_body = find_main_function_body(source)

    if not main_body:
        print("ERROR: Could not extract main() function body from source.")
        return 2

    print("--- Static Analysis Results ---")
    print()

    # Check 1: CefExecuteProcess (related to Bug 1, shown here for context)
    cef_result = check_cef_execute_process(source)
    print(f"[Context] CefExecuteProcess() present: {cef_result['present']}")
    print(f"          {cef_result['detail']}")
    print()

    # Check 2: createBrowser() deferral (the actual Bug 3 check)
    browser_result = check_create_browser_deferral(main_body)
    print(f"[Bug 3]   createBrowser() synchronous: {browser_result['synchronous']}")
    print(f"          createBrowser() deferred    : {browser_result['deferred']}")
    print(f"          {browser_result['detail']}")
    print()

    # Show the relevant lines from main() for human review
    print("--- Relevant lines from main() ---")
    for i, line in enumerate(main_body.splitlines(), 1):
        stripped = line.strip()
        if any(kw in stripped for kw in
               ["show()", "createBrowser", "QTimer", "singleShot"]):
            print(f"  line {i:3d}: {line.rstrip()}")
    print()

    # Verdict
    if browser_result["synchronous"] and not browser_result["deferred"]:
        print("RESULT: FAIL — BUG CONFIRMED")
        print("  createBrowser() is called synchronously after show().")
        print("  At this point, m_container->geometry() returns QRect(0,0,0,0).")
        print("  CEF embeds the browser at zero size; the page is never rendered.")
        print()
        print("  This is the EXPECTED outcome on unfixed code.")
        print("  Apply the fix (QTimer::singleShot deferral) and re-run to verify.")
        return 1
    elif browser_result["deferred"]:
        print("RESULT: PASS")
        print("  createBrowser() is deferred via QTimer::singleShot — bug is NOT present.")
        return 0
    else:
        print("RESULT: INCONCLUSIVE")
        print("  Could not determine whether the bug is present from static analysis.")
        print("  Manual inspection of src/main.cpp is required.")
        return 2


if __name__ == "__main__":
    sys.exit(run_test())
