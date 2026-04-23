#!/usr/bin/env python3
"""
Exploratory Test — Bug 1: Missing CefExecuteProcess()
======================================================

WHAT THIS TESTS
---------------
CEF's multi-process architecture re-invokes the host binary for each sub-process
(renderer, GPU, network service). Without an early CefExecuteProcess() call at the
top of main(), those sub-process invocations fall through to QApplication construction
and MainWindow::show(), producing extra blank top-level windows.

This script launches the Lilypad binary and counts the number of top-level windows
that appear within a short window after startup. On unfixed code, ≥ 2 windows are
expected (the real MainWindow plus at least one blank window from a sub-process).

EXPECTED RESULT ON UNFIXED CODE
--------------------------------
  FAIL — BUG CONFIRMED: ≥ 2 top-level windows detected.
  This is the SUCCESS case for this exploratory test.

EXPECTED RESULT AFTER FIX
--------------------------
  PASS — exactly 1 top-level window (the MainWindow).

REQUIRES
--------
  - wmctrl or xdotool installed  (sudo apt install wmctrl xdotool)
  - A running X display (export DISPLAY=:99 with Xvfb, or a real display)
  - The Lilypad binary built at ../../build/lilypad (or LILYPAD_BINARY env var)
"""

import os
import subprocess
import sys
import time

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
BINARY = os.environ.get("LILYPAD_BINARY", "./build/lilypad")
STARTUP_WAIT_SECONDS = 4      # time to let the app and sub-processes start
POLL_INTERVAL = 0.5           # seconds between window-count polls
WINDOW_TITLE_HINT = "Lilypad" # partial title of the expected main window

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def check_tool(name: str) -> bool:
    """Return True if `name` is available on PATH."""
    result = subprocess.run(
        ["which", name], capture_output=True, text=True
    )
    return result.returncode == 0


def count_windows_wmctrl() -> int:
    """Count top-level windows using wmctrl -l."""
    result = subprocess.run(
        ["wmctrl", "-l"], capture_output=True, text=True
    )
    if result.returncode != 0:
        return -1
    lines = [l for l in result.stdout.splitlines() if l.strip()]
    return len(lines)


def count_windows_xdotool() -> int:
    """Count top-level windows using xdotool search --onlyvisible."""
    result = subprocess.run(
        ["xdotool", "search", "--onlyvisible", "--name", ""],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        return 0
    ids = [l.strip() for l in result.stdout.splitlines() if l.strip()]
    return len(ids)


def count_windows() -> int:
    """Count top-level windows using the best available tool."""
    if check_tool("wmctrl"):
        return count_windows_wmctrl()
    elif check_tool("xdotool"):
        return count_windows_xdotool()
    else:
        print("WARNING: Neither wmctrl nor xdotool found. Cannot count windows.")
        print("         Install with: sudo apt install wmctrl xdotool")
        return -1


# ---------------------------------------------------------------------------
# Main test logic
# ---------------------------------------------------------------------------

def run_test() -> int:
    """
    Launch the binary, wait for startup, count windows.
    Returns 0 (PASS) or 1 (FAIL/BUG CONFIRMED).
    """
    print("=" * 70)
    print("Bug 1 Exploratory Test: Missing CefExecuteProcess()")
    print("=" * 70)
    print(f"Binary : {BINARY}")
    print(f"Display: {os.environ.get('DISPLAY', '(not set)')}")
    print()

    # Verify binary exists
    if not os.path.isfile(BINARY):
        print(f"ERROR: Binary not found at '{BINARY}'")
        print("       Build the project first, or set LILYPAD_BINARY env var.")
        return 2

    # Verify a window-counting tool is available
    if not check_tool("wmctrl") and not check_tool("xdotool"):
        print("ERROR: Neither wmctrl nor xdotool is installed.")
        print("       sudo apt install wmctrl xdotool")
        return 2

    # Baseline window count before launching
    baseline = count_windows()
    print(f"Baseline window count (before launch): {baseline}")

    # Launch the binary
    print(f"Launching: {BINARY}")
    proc = subprocess.Popen(
        [BINARY],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

    max_windows_seen = 0
    try:
        print(f"Waiting {STARTUP_WAIT_SECONDS}s for sub-processes to start...")
        deadline = time.time() + STARTUP_WAIT_SECONDS
        while time.time() < deadline:
            time.sleep(POLL_INTERVAL)
            current = count_windows()
            new_windows = max(0, current - baseline)
            if new_windows > max_windows_seen:
                max_windows_seen = new_windows
                print(f"  [{time.time():.1f}s] New top-level windows: {new_windows}")
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()

    print()
    print(f"Maximum new top-level windows observed: {max_windows_seen}")
    print()

    # Assertion: on unfixed code we expect ≥ 2 new windows
    if max_windows_seen >= 2:
        print("RESULT: FAIL — BUG CONFIRMED")
        print("  ≥ 2 top-level windows appeared.")
        print("  Sub-processes are re-entering main() and opening blank windows.")
        print("  Root cause: CefExecuteProcess() is not called before QApplication.")
        print()
        print("  This is the EXPECTED outcome on unfixed code.")
        print("  Apply the fix (add CefExecuteProcess() guard) and re-run to verify.")
        return 1
    elif max_windows_seen == 1:
        print("RESULT: PASS")
        print("  Exactly 1 new top-level window appeared — bug is NOT present.")
        print("  (Either the code has been fixed, or the sub-processes did not start.)")
        return 0
    else:
        print("RESULT: INCONCLUSIVE")
        print(f"  {max_windows_seen} new windows observed.")
        print("  The binary may not have started, or the display is not accessible.")
        return 2


if __name__ == "__main__":
    sys.exit(run_test())
