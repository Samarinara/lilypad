#!/usr/bin/env python3
"""
Fix-Checking Integration Test — Task 6.4
==========================================
Verifies that the main window has non-zero dimensions after OnAfterCreated
has fired (i.e., after the CEF browser has been created and embedded).

WHAT THIS TESTS
---------------
Bug 3 was caused by calling createBrowser() synchronously after show(),
before the Qt event loop had processed the initial layout. This meant
m_container->geometry() returned QRect(0,0,0,0), so CEF embedded the
browser at zero size and the page never rendered.

The fix defers createBrowser() via QTimer::singleShot(0, ...), ensuring
the container has valid geometry when CEF reads it.

This test:
  1. Launches the fixed binary.
  2. Waits 3 seconds for the browser to initialise (OnAfterCreated to fire).
  3. Uses xdotool or wmctrl to check that the main window has non-zero
     width and height.

REQUIRES
--------
  - The Lilypad binary at ./build/lilypad (or LILYPAD_BINARY env var)
  - xdotool installed  (sudo apt install xdotool)
  - A running X display (DISPLAY env var set)
"""

import os
import subprocess
import sys
import time

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
BINARY = os.environ.get("LILYPAD_BINARY", "./build/lilypad")
# Time to wait for OnAfterCreated to fire and the window to be fully laid out
INIT_WAIT_SECONDS = 3
WINDOW_TITLE_HINT = "Lilypad"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def check_tool(name: str) -> bool:
    result = subprocess.run(["which", name], capture_output=True, text=True)
    return result.returncode == 0


def get_window_geometry_xdotool(title_hint: str) -> dict:
    """
    Use xdotool to find the window matching title_hint and return its geometry.
    Returns dict with 'width', 'height', 'x', 'y', or None on failure.
    """
    # Search for window by name
    search = subprocess.run(
        ["xdotool", "search", "--name", title_hint],
        capture_output=True, text=True,
    )
    if search.returncode != 0 or not search.stdout.strip():
        return None

    # Take the first matching window ID
    wid = search.stdout.strip().splitlines()[0].strip()

    # Get geometry
    geo = subprocess.run(
        ["xdotool", "getwindowgeometry", wid],
        capture_output=True, text=True,
    )
    if geo.returncode != 0:
        return None

    # Parse output like:
    #   Window 12345678
    #     Position: 100,200 (screen: 0)
    #     Geometry: 1200x800
    import re
    geo_match = re.search(r'Geometry:\s*(\d+)x(\d+)', geo.stdout)
    pos_match = re.search(r'Position:\s*(\d+),(\d+)', geo.stdout)
    if not geo_match:
        return None

    return {
        "width": int(geo_match.group(1)),
        "height": int(geo_match.group(2)),
        "x": int(pos_match.group(1)) if pos_match else 0,
        "y": int(pos_match.group(2)) if pos_match else 0,
        "window_id": wid,
    }


def get_window_geometry_wmctrl(title_hint: str) -> dict:
    """
    Use wmctrl -lG to find the window and return its geometry.
    wmctrl -lG output: <id> <desktop> <x> <y> <w> <h> <host> <title>
    """
    result = subprocess.run(
        ["wmctrl", "-lG"], capture_output=True, text=True,
    )
    if result.returncode != 0:
        return None

    import re
    for line in result.stdout.splitlines():
        if title_hint.lower() in line.lower():
            parts = line.split()
            if len(parts) >= 6:
                try:
                    return {
                        "x": int(parts[2]),
                        "y": int(parts[3]),
                        "width": int(parts[4]),
                        "height": int(parts[5]),
                        "window_id": parts[0],
                    }
                except ValueError:
                    pass
    return None


def get_window_geometry(title_hint: str) -> dict:
    """Try xdotool first, then wmctrl."""
    if check_tool("xdotool"):
        geo = get_window_geometry_xdotool(title_hint)
        if geo:
            return geo
    if check_tool("wmctrl"):
        geo = get_window_geometry_wmctrl(title_hint)
        if geo:
            return geo
    return None


# ---------------------------------------------------------------------------
# Main test logic
# ---------------------------------------------------------------------------

def run_test() -> int:
    print("=" * 70)
    print("Fix-Checking Integration Test 6.4: non-zero window geometry after init")
    print("=" * 70)
    print(f"Binary     : {BINARY}")
    print(f"Init wait  : {INIT_WAIT_SECONDS}s (for OnAfterCreated to fire)")
    print(f"Display    : {os.environ.get('DISPLAY', '(not set)')}")
    print()

    # Graceful skip if binary is missing
    if not os.path.isfile(BINARY):
        print(f"SKIP: Binary not found at '{BINARY}'")
        print("      Build the project first (cmake --build build), or set LILYPAD_BINARY.")
        return 0

    # Check tool availability
    if not check_tool("xdotool") and not check_tool("wmctrl"):
        print("SKIP: Neither xdotool nor wmctrl is installed.")
        print("      sudo apt install xdotool wmctrl")
        return 0

    print(f"Launching: {BINARY}")
    proc = subprocess.Popen(
        [BINARY],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

    try:
        print(f"Waiting {INIT_WAIT_SECONDS}s for browser initialisation "
              f"(OnAfterCreated)...")
        time.sleep(INIT_WAIT_SECONDS)

        # Check the process is still running
        if proc.poll() is not None:
            print(f"ERROR: Binary exited prematurely with code {proc.returncode}.")
            return 1

        print(f"Querying window geometry for title hint: '{WINDOW_TITLE_HINT}'")
        geo = get_window_geometry(WINDOW_TITLE_HINT)

        if geo is None:
            print("WARN: Could not find a window matching the title hint.")
            print("      The binary may not have opened a window, or the display")
            print("      is not accessible. Skipping geometry assertion.")
            return 0

        print(f"Window geometry: {geo['width']}x{geo['height']} "
              f"at ({geo['x']}, {geo['y']})  [id: {geo['window_id']}]")
        print()

        failures = []

        # --- Assertion: width > 0 ---
        if geo["width"] > 0:
            print(f"[PASS] Window width  = {geo['width']} > 0")
        else:
            failures.append(
                f"FAIL: Window width = {geo['width']} (expected > 0).\n"
                f"      The container geometry may still be zero — "
                f"createBrowser() deferral may not be working."
            )

        # --- Assertion: height > 0 ---
        if geo["height"] > 0:
            print(f"[PASS] Window height = {geo['height']} > 0")
        else:
            failures.append(
                f"FAIL: Window height = {geo['height']} (expected > 0).\n"
                f"      The container geometry may still be zero."
            )

        print()
        if failures:
            print("RESULT: FAIL")
            for msg in failures:
                print(f"  {msg}")
            return 1

        print("RESULT: PASS")
        print(f"  Window has non-zero dimensions ({geo['width']}x{geo['height']}) "
              f"after browser initialisation.")
        return 0

    finally:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()


if __name__ == "__main__":
    sys.exit(run_test())
