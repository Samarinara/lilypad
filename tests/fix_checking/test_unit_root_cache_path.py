#!/usr/bin/env python3
"""
Fix-Checking Unit Test — Task 6.2
===================================
Verifies that CefSettings.root_cache_path is set to a non-empty,
application-specific path using QStandardPaths.

WHAT THIS TESTS
---------------
Bug 4 was caused by leaving CefSettings.root_cache_path empty. CEF then falls
back to a shared system cache location and emits a repeated warning.

The fix sets root_cache_path to an application-specific directory derived from
QStandardPaths::CacheLocation + "/lilypad".

This test uses static source analysis to confirm:
  1. root_cache_path is assigned a value (not left empty).
  2. The assigned path includes "lilypad" (application-specific, not generic).
  3. QStandardPaths is used to derive the path (not a hardcoded string).

EXPECTED RESULT ON FIXED CODE
------------------------------
  PASS — root_cache_path is assigned, includes "lilypad", uses QStandardPaths.

EXPECTED RESULT ON UNFIXED CODE
--------------------------------
  FAIL — root_cache_path is not assigned.

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

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load_source(path: str) -> str:
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def non_comment_lines(source: str) -> list:
    """Return source lines that are not pure C++ line comments."""
    result = []
    for line in source.splitlines():
        stripped = line.strip()
        if not stripped.startswith("//") and not stripped.startswith("*"):
            result.append(line)
    return result


def check_root_cache_path_assigned(lines: list) -> dict:
    """
    Check whether root_cache_path is assigned a value in non-comment code.
    Returns dict with 'assigned' (bool), 'line' (str or None), 'detail' (str).
    """
    for line in lines:
        stripped = line.strip()
        if "root_cache_path" not in stripped:
            continue
        # Must be an assignment or a FromString / Assign call
        if re.search(r'root_cache_path.*(?:=|FromString|Assign)', stripped):
            return {
                "assigned": True,
                "line": stripped,
                "detail": f"Assignment found: {stripped}",
            }
    return {
        "assigned": False,
        "line": None,
        "detail": "root_cache_path is not assigned anywhere in non-comment code.",
    }


def check_lilypad_in_path(source: str) -> dict:
    """
    Check whether the string 'lilypad' appears in the context of the
    root_cache_path assignment (within a few lines of it).
    """
    lines = source.splitlines()
    for i, line in enumerate(lines):
        if "root_cache_path" in line and re.search(r'(?:=|FromString|Assign)', line):
            # Check this line and the surrounding 5 lines for "lilypad"
            context = "\n".join(lines[max(0, i - 3):i + 4])
            if "lilypad" in context.lower():
                return {
                    "found": True,
                    "detail": "\"lilypad\" appears near the root_cache_path assignment.",
                }
            return {
                "found": False,
                "detail": (
                    "\"lilypad\" does NOT appear near the root_cache_path assignment.\n"
                    "          The path may not be application-specific."
                ),
            }
    return {
        "found": False,
        "detail": "Could not locate root_cache_path assignment to check for 'lilypad'.",
    }


def check_qstandardpaths_used(source: str) -> dict:
    """
    Check whether QStandardPaths is used in the source (not a hardcoded path).
    """
    # Check for the include
    has_include = bool(re.search(r'#include\s*[<"]QStandardPaths[>"]', source))
    # Check for actual usage
    has_usage = bool(re.search(r'\bQStandardPaths\b', source))

    if has_include and has_usage:
        return {
            "used": True,
            "detail": "QStandardPaths is included and used — path is not hardcoded.",
        }
    elif has_usage:
        return {
            "used": True,
            "detail": "QStandardPaths is used (include may be indirect).",
        }
    else:
        return {
            "used": False,
            "detail": (
                "QStandardPaths is NOT used. The path may be hardcoded, which is\n"
                "          fragile and non-portable."
            ),
        }


# ---------------------------------------------------------------------------
# Main test logic
# ---------------------------------------------------------------------------

def run_test() -> int:
    """
    Static analysis of CefSettings.root_cache_path assignment.
    Returns 0 (PASS) or 1 (FAIL).
    """
    print("=" * 70)
    print("Fix-Checking Unit Test 6.2: root_cache_path set correctly")
    print("=" * 70)
    print(f"Source file: {SOURCE_FILE}")
    print()

    if not os.path.isfile(SOURCE_FILE):
        print(f"ERROR: Source file not found at '{SOURCE_FILE}'")
        print("       Run from the repo root, or set LILYPAD_SOURCE env var.")
        return 1

    source = load_source(SOURCE_FILE)
    clean_lines = non_comment_lines(source)

    failures = []

    # --- Assertion 1: root_cache_path is assigned ---
    assigned_result = check_root_cache_path_assigned(clean_lines)
    if assigned_result["assigned"]:
        print(f"[PASS] root_cache_path is assigned a value.")
        print(f"       {assigned_result['detail']}")
    else:
        failures.append(
            f"FAIL: root_cache_path is NOT assigned.\n"
            f"      {assigned_result['detail']}"
        )
    print()

    # --- Assertion 2: path includes "lilypad" ---
    lilypad_result = check_lilypad_in_path(source)
    if lilypad_result["found"]:
        print(f"[PASS] Path includes 'lilypad' — application-specific.")
        print(f"       {lilypad_result['detail']}")
    else:
        failures.append(
            f"FAIL: Path does not include 'lilypad'.\n"
            f"      {lilypad_result['detail']}"
        )
    print()

    # --- Assertion 3: QStandardPaths is used ---
    qsp_result = check_qstandardpaths_used(source)
    if qsp_result["used"]:
        print(f"[PASS] QStandardPaths is used — path is not hardcoded.")
        print(f"       {qsp_result['detail']}")
    else:
        failures.append(
            f"FAIL: QStandardPaths is NOT used.\n"
            f"      {qsp_result['detail']}"
        )
    print()

    if failures:
        print("RESULT: FAIL")
        for msg in failures:
            print(f"  {msg}")
        return 1

    print("RESULT: PASS")
    print("  root_cache_path is assigned, includes 'lilypad', and uses QStandardPaths.")
    return 0


if __name__ == "__main__":
    sys.exit(run_test())
