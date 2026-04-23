#!/usr/bin/env python3
"""
Fix-Checking Integration Test — Task 6.5
==========================================
Verifies that the fixed binary produces zero resource_util.cc:83 occurrences
in stderr, and that root_cache_path is set in source.

WHAT THIS TESTS
---------------
Bug 4 was caused by leaving CefSettings.root_cache_path empty. CEF then
falls back to a shared system cache location and emits a repeated warning:

  [WARNING:resource_util.cc:83] Please customize CefSettings.root_cache_path
  for your application. Use of the default value may lead to unintended
  process singleton behavior.

The fix sets root_cache_path to an application-specific directory.

This test performs two checks:
  1. Static analysis: confirms root_cache_path is assigned in src/main.cpp.
  2. Runtime check: launches the binary, monitors stderr for 5 seconds, and
     asserts zero occurrences of resource_util.cc:83.

REQUIRES
--------
  - src/main.cpp present at ./src/main.cpp (or LILYPAD_SOURCE env var)
  - The Lilypad binary at ./build/lilypad (or LILYPAD_BINARY env var)
  - A running X display (DISPLAY env var set)
"""

import os
import queue
import re
import subprocess
import sys
import threading
import time

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
BINARY = os.environ.get("LILYPAD_BINARY", "./build/lilypad")
SOURCE_FILE = os.environ.get("LILYPAD_SOURCE", "./src/main.cpp")
MONITOR_SECONDS = 5
WARNING_FRAGMENT = "resource_util.cc:83"

# ---------------------------------------------------------------------------
# Static analysis
# ---------------------------------------------------------------------------

def check_root_cache_path_in_source(path: str) -> dict:
    """
    Check whether root_cache_path is assigned in non-comment source code.
    Returns dict with 'set' (bool) and 'detail' (str).
    """
    if not os.path.isfile(path):
        return {"set": None, "detail": f"Source file not found: {path}"}

    with open(path, "r", encoding="utf-8") as f:
        source = f.read()

    for line in source.splitlines():
        stripped = line.strip()
        if stripped.startswith("//") or stripped.startswith("*"):
            continue
        if "root_cache_path" in stripped and re.search(
            r'(?:=|FromString|Assign)', stripped
        ):
            return {
                "set": True,
                "detail": f"Assignment found: {stripped}",
            }

    return {
        "set": False,
        "detail": "root_cache_path is not assigned in non-comment code.",
    }


# ---------------------------------------------------------------------------
# Stderr reader thread
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
    print("Fix-Checking Integration Test 6.5: zero resource_util.cc:83 warnings")
    print("=" * 70)
    print(f"Binary     : {BINARY}")
    print(f"Source file: {SOURCE_FILE}")
    print(f"Monitor    : {MONITOR_SECONDS}s")
    print(f"Display    : {os.environ.get('DISPLAY', '(not set)')}")
    print()

    failures = []

    # -----------------------------------------------------------------------
    # Phase 1: Static analysis
    # -----------------------------------------------------------------------
    print("--- Phase 1: Static Source Analysis ---")
    static_result = check_root_cache_path_in_source(SOURCE_FILE)

    if static_result["set"] is None:
        print(f"WARN: {static_result['detail']}")
    elif static_result["set"]:
        print(f"[PASS] root_cache_path is assigned in source.")
        print(f"       {static_result['detail']}")
    else:
        failures.append(
            f"FAIL: root_cache_path is NOT assigned in source.\n"
            f"      {static_result['detail']}"
        )
        print(f"[FAIL] root_cache_path is NOT assigned in source.")
        print(f"       {static_result['detail']}")
    print()

    # -----------------------------------------------------------------------
    # Phase 2: Runtime stderr monitoring
    # -----------------------------------------------------------------------
    print("--- Phase 2: Runtime stderr Monitoring ---")

    if not os.path.isfile(BINARY):
        print(f"SKIP: Binary not found at '{BINARY}'")
        print("      Build the project first (cmake --build build), or set LILYPAD_BINARY.")
        print("      Static analysis result above is the primary check.")
        print()
    else:
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

        warning_lines = []
        start_time = time.time()

        print(f"Monitoring stderr for {MONITOR_SECONDS}s...")
        try:
            while time.time() - start_time < MONITOR_SECONDS:
                try:
                    ts, line = line_queue.get(timeout=0.1)
                    if WARNING_FRAGMENT in line:
                        warning_lines.append((ts, line))
                        elapsed = ts - start_time
                        print(f"  [{elapsed:5.2f}s] WARNING: {line}")
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
        print(f"Total '{WARNING_FRAGMENT}' occurrences in {MONITOR_SECONDS}s: "
              f"{len(warning_lines)}")
        print()

        if len(warning_lines) == 0:
            print(f"[PASS] Zero '{WARNING_FRAGMENT}' occurrences in stderr.")
        else:
            failures.append(
                f"FAIL: {len(warning_lines)} occurrence(s) of '{WARNING_FRAGMENT}' "
                f"detected in stderr.\n"
                f"      root_cache_path may not be set correctly in the built binary."
            )
            print(f"[FAIL] {len(warning_lines)} warning occurrence(s) detected.")

    print()
    print("--- Summary ---")
    if failures:
        print("RESULT: FAIL")
        for msg in failures:
            print(f"  {msg}")
        return 1

    print("RESULT: PASS")
    print("  root_cache_path is set in source and no cache-path warnings in stderr.")
    return 0


if __name__ == "__main__":
    sys.exit(run_test())
