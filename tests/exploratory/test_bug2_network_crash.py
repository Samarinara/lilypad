#!/usr/bin/env python3
"""
Exploratory Test — Bug 2: in-process-gpu conflicts with disable-gpu
====================================================================

WHAT THIS TESTS
---------------
In OnBeforeCommandLineProcessing, the unfixed code appends both:
  - disable-gpu, disable-gpu-compositing, disable-gpu-rasterization,
    disable-software-rasterizer  (tell Chromium: no GPU at all)
  - in-process-gpu  (tell Chromium: run GPU thread inside the browser process)

These switches directly contradict each other. The contradiction leaves the
Chromium network service in an undefined state. Chromium's process manager
detects the crash and restarts the network service in a tight loop, logging:

  [ERROR:network_service_instance_impl.cc:608] Network service crashed or
  was terminated, restarting service.

This script launches the binary, captures stderr for 5 seconds, and counts
occurrences of that log line. On unfixed code, ≥ 1 occurrence per second is
expected (i.e., ≥ 5 occurrences over 5 seconds).

EXPECTED RESULT ON UNFIXED CODE
--------------------------------
  FAIL — BUG CONFIRMED: network service crash log appears ≥ 1×/second.
  This is the SUCCESS case for this exploratory test.

EXPECTED RESULT AFTER FIX
--------------------------
  PASS — zero occurrences of the crash log over 5 seconds.

REQUIRES
--------
  - The Lilypad binary built at ../../build/lilypad (or LILYPAD_BINARY env var)
  - A running X display (export DISPLAY=:99 with Xvfb, or a real display)
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
MONITOR_SECONDS = 5
# The exact log fragment that indicates a network service crash/restart
CRASH_LOG_FRAGMENT = "network_service_instance_impl.cc:608"
# Minimum occurrences per second to confirm the bug
MIN_RATE_PER_SECOND = 1.0

# ---------------------------------------------------------------------------
# Stderr reader thread
# ---------------------------------------------------------------------------

def stderr_reader(pipe, line_queue: queue.Queue, stop_event: threading.Event):
    """Read lines from pipe and put them on the queue until stop_event is set."""
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
    """
    Launch the binary, monitor stderr for 5 seconds, count crash log lines.
    Returns 0 (PASS) or 1 (FAIL/BUG CONFIRMED).
    """
    print("=" * 70)
    print("Bug 2 Exploratory Test: in-process-gpu conflicts with disable-gpu")
    print("=" * 70)
    print(f"Binary          : {BINARY}")
    print(f"Monitor duration: {MONITOR_SECONDS}s")
    print(f"Looking for     : '{CRASH_LOG_FRAGMENT}'")
    print(f"Display         : {os.environ.get('DISPLAY', '(not set)')}")
    print()

    # Verify binary exists
    if not os.path.isfile(BINARY):
        print(f"ERROR: Binary not found at '{BINARY}'")
        print("       Build the project first, or set LILYPAD_BINARY env var.")
        return 2

    # Launch the binary, capturing stderr
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

    crash_timestamps = []
    start_time = time.time()

    print(f"Monitoring stderr for {MONITOR_SECONDS}s...")
    try:
        while time.time() - start_time < MONITOR_SECONDS:
            try:
                ts, line = line_queue.get(timeout=0.1)
                if CRASH_LOG_FRAGMENT in line:
                    crash_timestamps.append(ts)
                    elapsed = ts - start_time
                    print(f"  [{elapsed:5.2f}s] CRASH LOG: {line}")
            except queue.Empty:
                pass
    finally:
        stop_event.set()
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()

    print()
    total_crashes = len(crash_timestamps)
    actual_rate = total_crashes / MONITOR_SECONDS if MONITOR_SECONDS > 0 else 0

    print(f"Total crash log occurrences in {MONITOR_SECONDS}s: {total_crashes}")
    print(f"Rate: {actual_rate:.2f} occurrences/second")
    print(f"Threshold for BUG CONFIRMED: ≥ {MIN_RATE_PER_SECOND:.1f}/second")
    print()

    if actual_rate >= MIN_RATE_PER_SECOND:
        print("RESULT: FAIL — BUG CONFIRMED")
        print(f"  Network service crash log appeared {actual_rate:.2f}×/second.")
        print("  The in-process-gpu switch contradicts disable-gpu, causing the")
        print("  network service to crash and restart in a tight loop.")
        print()
        print("  This is the EXPECTED outcome on unfixed code.")
        print("  Apply the fix (remove in-process-gpu switch) and re-run to verify.")
        return 1
    elif total_crashes > 0:
        print("RESULT: PARTIAL — crash log appeared but below threshold")
        print(f"  {total_crashes} occurrence(s) in {MONITOR_SECONDS}s ({actual_rate:.2f}/s).")
        print("  The bug may be present but slower to manifest, or partially fixed.")
        return 1
    else:
        print("RESULT: PASS")
        print("  Zero network service crash log occurrences — bug is NOT present.")
        print("  (Either the code has been fixed, or the binary did not start.)")
        return 0


if __name__ == "__main__":
    sys.exit(run_test())
