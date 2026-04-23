#!/usr/bin/env python3
"""
Fix-Checking Unit Test — Task 6.1
===================================
Verifies that OnBeforeCommandLineProcessing does NOT append in-process-gpu.

WHAT THIS TESTS
---------------
Bug 2 was caused by appending both `in-process-gpu` and the `disable-gpu`
family of switches in OnBeforeCommandLineProcessing. These switches directly
contradict each other, destabilising the Chromium network service.

The fix removes `in-process-gpu`. This test uses static source analysis to
confirm:
  1. `in-process-gpu` does NOT appear as an AppendSwitch argument inside
     OnBeforeCommandLineProcessing.
  2. The `disable-gpu` family switches ARE still present (regression check).

EXPECTED RESULT ON FIXED CODE
------------------------------
  PASS — in-process-gpu is absent; disable-gpu family is present.

EXPECTED RESULT ON UNFIXED CODE
--------------------------------
  FAIL — in-process-gpu is present as an AppendSwitch argument.

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

# The switch that must NOT be present
FORBIDDEN_SWITCH = "in-process-gpu"

# Switches that MUST still be present (regression check)
REQUIRED_SWITCHES = [
    "disable-gpu",
    "disable-software-rasterizer",
    "disable-gpu-compositing",
    "disable-gpu-rasterization",
]

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load_source(path: str) -> str:
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def extract_on_before_command_line(source: str) -> str:
    """
    Extract the body of OnBeforeCommandLineProcessing from the source.
    Uses brace counting starting from the function signature.
    """
    match = re.search(r'\bOnBeforeCommandLineProcessing\b', source)
    if not match:
        return ""

    # Find the opening brace of the function body
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


def find_append_switch_args(function_body: str) -> list:
    """
    Return a list of all switch names passed to AppendSwitch() in the body.
    Handles both single and double quotes, and strips surrounding whitespace.
    """
    pattern = re.compile(
        r'AppendSwitch\s*\(\s*["\']([^"\']+)["\']\s*\)',
        re.IGNORECASE,
    )
    return [m.group(1) for m in pattern.finditer(function_body)]


# ---------------------------------------------------------------------------
# Main test logic
# ---------------------------------------------------------------------------

def run_test() -> int:
    """
    Static analysis of OnBeforeCommandLineProcessing.
    Returns 0 (PASS) or 1 (FAIL).
    """
    print("=" * 70)
    print("Fix-Checking Unit Test 6.1: in-process-gpu absent from AppendSwitch")
    print("=" * 70)
    print(f"Source file: {SOURCE_FILE}")
    print()

    if not os.path.isfile(SOURCE_FILE):
        print(f"ERROR: Source file not found at '{SOURCE_FILE}'")
        print("       Run from the repo root, or set LILYPAD_SOURCE env var.")
        return 1

    source = load_source(SOURCE_FILE)
    func_body = extract_on_before_command_line(source)

    if not func_body:
        print("ERROR: Could not locate OnBeforeCommandLineProcessing in source.")
        return 1

    switches = find_append_switch_args(func_body)
    print(f"AppendSwitch calls found in OnBeforeCommandLineProcessing:")
    for sw in switches:
        print(f"  - \"{sw}\"")
    print()

    failures = []

    # --- Assertion 1: in-process-gpu must NOT be present ---
    if FORBIDDEN_SWITCH in switches:
        failures.append(
            f"FAIL: '{FORBIDDEN_SWITCH}' is present as an AppendSwitch argument.\n"
            f"      This switch contradicts disable-gpu and destabilises the network service."
        )
    else:
        print(f"[PASS] '{FORBIDDEN_SWITCH}' is NOT present in AppendSwitch calls.")

    # --- Assertion 2: disable-gpu family must still be present ---
    for required in REQUIRED_SWITCHES:
        if required in switches:
            print(f"[PASS] '{required}' is present (regression check OK).")
        else:
            failures.append(
                f"FAIL: Required switch '{required}' is MISSING from AppendSwitch calls.\n"
                f"      This is a regression — the disable-gpu family must remain."
            )

    print()
    if failures:
        print("RESULT: FAIL")
        for msg in failures:
            print(f"  {msg}")
        return 1

    print("RESULT: PASS")
    print("  in-process-gpu is absent; all disable-gpu family switches are present.")
    return 0


if __name__ == "__main__":
    sys.exit(run_test())
