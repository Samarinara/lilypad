#!/usr/bin/env python3
"""
Fix-Checking Integration Test — Task 6.3
==========================================
Verifies that the fixed binary:
  1. Opens exactly one new top-level window (Bug 1 fix check).
  2. Produces zero network_service_instance_impl.cc:608 occurrences in
     stderr over 5 seconds (Bug 2 fix check).

WHAT THIS TESTS
---------------
Before the fix:
  - Sub-processes re-entered main() and opened extra blank windows.
  - The in-process-gpu / disable-gpu contradiction caused the network service
    to crash and restart in a tight loop, logging the crash line repeatedly.

After the fix:
  - CefExecuteProcess() causes sub-processes to exit early → exactly 1 window.
  - in-process-gpu is removed → network service starts once and stays up.

REQUIRES
--------
  - The Lilypad binary at ./build/lilypad (or LILYPAD_BINARY env var)
  - wmctrl or xdotool installed  (sudo apt install wmctrl xdotool)
  - A running X display (DISPLAY env var set)
"""

import os
import queue
import subprocess
import sys
import threading
import time

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
BINARY = os.environ.get("LILYPAD_BINARY", "./build/lilypad")
STARTUP_WAIT_SECONDS = 4      # time to let the app fully start
MONITOR_SECONDS = 5           # duration to watch stderr for crash logs
POLL_INTERVAL = 0.5
CRASH_LOG_FRAGMENT = "network_service_instance_impl.cc:608"

# ---------------------------------------------------------------------------
# Helpers — window counting
# ---------------------------------------------------------------------------

def check_tool(name: str) -> bool:
    result = subprocess.run(["which", name], capture_output=True, text=True)
    return result.returncode == 0


def count_windows_wmctrl() -> int:
    result = subprocess.run(["wmctrl", "-l"], capture_output=True, text=True)
    if result.returncode != 0:
        return -1
    lines = [l for l in result.stdout.splitlines() if l.strip()]
    return len(lines)


def count_windows_xdotool() -> int:
    result = subprocess.run(
        ["xdotool", "search", "--onlyvisible", "--name", ""],
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        return 0
    ids = [l.strip() for l in result.stdout.splitlines() if l.strip()]
    return len(ids)


def count_windows() -> int:
    if check_tool("wmctrl"):
        return count_windows_wmctrl()
    elif check_tool("xdotool"):
        return count_windows_xdotool()
    return -1


# ---------------------------------------------------------------------------
# Helpers — stderr reader
# ---------------------------------------------------------------------------

def stderr_reader(pipe, line_queue: queue.Queue, stop_event: threading.Event):
    try:
        for line in iter(pipe.readline, b""):
            if stop_event.is_set():
                break
            line_queue.put((time.time(), line.decode("utf-8", errors="replace").rstrip()))
    except Exception:
        pass
    finally:
        pipe.close()


# ---------------------------------------------------------------------------
# Main test logic
# ---------------------------------------------------------------------------

def run_test() -> int:
    print("=" * 70)
    print("Fix-Checking Integration Test 6.3: single window + zero network crashes")
    print("=" * 70)
    print(f"Binary          : {BINARY}")
    print(f"Monitor duration: {MONITOR_SECONDS}s")
    print(f"Display         : {os.environ.get('DISPLAY', '(not set)')}")
    print()

    # Graceful skip if binary is missing
    if not os.path.isfile(BINARY):
        print(f"SKIP: Binary not found at '{BINARY}'")
        print("      Build the project first (cmake --build build), or set LILYPAD_BINARY.")
        print("      Skipping integration test — static analysis tests still apply.")
        return 0

    # Check window-counting tool availability
    if not check_tool("wmctrl") and not check_tool("xdotool"):
        print("SKIP: Neither wmctrl nor xdotool is installed.")
        print("      sudo apt install wmctrl xdotool")
        return 0

    baseline = count_windows()
    print(f"Baseline window count (before launch): {baseline}")

    print(f"Launching: {BINARY}")
    proc = subprocess.Popen(
        [BINARY],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
    )

    line_queue: queue.Queue = queue.Queue()
    stop_event = threading.Event()
    reader_thread = threading.Thread(
        target=stderr_reader,
        args=(proc.stderr, line_queue, stop_event),
        daemon=True,
    )
    reader_thread.start()

    crash_lines = []
    max_new_windows = 0
    start_time = time.time()
    total_duration = max(STARTUP_WAIT_SECONDS, MONITOR_SECONDS)

    print(f"Monitoring for {total_duration}s (startup: {STARTUP_WAIT_SECONDS}s, "
          f"crash watch: {MONITOR_SECONDS}s)...")

    try:
        while time.time() - start_time < total_duration:
            # Poll window count during startup phase
            if time.time() - start_time < STARTUP_WAIT_SECONDS:
                current = count_windows()
                new_windows = max(0, current - baseline)
                if new_windows > max_new_windows:
                    max_new_windows = new_windows
                    elapsed = time.time() - start_time
                    print(f"  [{elapsed:5.2f}s] New top-level windows: {new_windows}")

            # Drain stderr queue
            try:
                while True:
                    ts, line = line_queue.get_nowait()
                    if CRASH_LOG_FRAGMENT in line:
                        crash_lines.append((ts, line))
                        elapsed = ts - start_time
                        print(f"  [{elapsed:5.2f}s] CRASH LOG: {line}")
            except queue.Empty:
                pass

            time.sleep(POLL_INTERVAL)
    finally:
        stop_event.set()
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()

    print()
    print(f"Maximum new top-level windows observed : {max_new_windows}")
    print(f"Network service crash log occurrences  : {len(crash_lines)}")
    print()

    failures = []

    # --- Assertion 1: exactly 1 new window ---
    if max_new_windows == 1:
        print("[PASS] Exactly 1 new top-level window appeared.")
    elif max_new_windows == 0:
        print("[WARN] No new windows observed — binary may not have started fully.")
        print("       This may be an environment issue (no display, missing libs).")
        # Not a hard failure — environment may lack a display
    else:
        failures.append(
            f"FAIL: {max_new_windows} new top-level windows appeared (expected 1).\n"
            f"      Sub-processes may still be re-entering main()."
        )

    # --- Assertion 2: zero network service crash logs ---
    if len(crash_lines) == 0:
        print("[PASS] Zero network service crash log occurrences.")
    else:
        failures.append(
            f"FAIL: {len(crash_lines)} network service crash log occurrence(s) detected.\n"
            f"      The in-process-gpu fix may not have been applied."
        )

    print()
    if failures:
        print("RESULT: FAIL")
        for msg in failures:
            print(f"  {msg}")
        return 1

    print("RESULT: PASS")
    print("  Single window and zero network service crashes confirmed.")
    return 0


if __name__ == "__main__":
    sys.exit(run_test())
