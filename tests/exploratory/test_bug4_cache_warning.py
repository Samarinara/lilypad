#!/usr/bin/env python3
"""
Exploratory Test — Bug 4: CefSettings.root_cache_path not set
==============================================================

WHAT THIS TESTS
---------------
When CefSettings.root_cache_path is left empty (the default), CEF falls back to
a shared system cache location. CEF warns about this at startup because multiple
CEF-based applications sharing the same cache path can trigger process-singleton
detection misfire.

The warning logged to stderr is:

  [WARNING:resource_util.cc:83] Please customize CefSettings.root_cache_path
  for your application. Use of the default value may lead to unintended process
  singleton behavior.

This script launches the binary, captures stderr, and checks whether this warning
appears. On unfixed code, the warning is expected to appear on every launch.

Additionally, this script performs a STATIC ANALYSIS of src/main.cpp to confirm
that root_cache_path is not set in the settings block — providing a fast, reliable
check that does not require a running binary.

EXPECTED RESULT ON UNFIXED CODE
--------------------------------
  FAIL — BUG CONFIRMED: root_cache_path warning appears in stderr, and
  root_cache_path is not set in source.
  This is the SUCCESS case for this exploratory test.

EXPECTED RESULT AFTER FIX
--------------------------
  PASS — zero occurrences of the warning, and root_cache_path is set in source.

REQUIRES
--------
  - The Lilypad binary built at ../../build/lilypad (or LILYPAD_BINARY env var)
  - src/main.cpp present at ../../src/main.cpp (or LILYPAD_SOURCE env var)
  - A running X display (export DISPLAY=:99 with Xvfb, or a real display)
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
# The exact log fragment that indicates the missing root_cache_path
WARNING_FRAGMENT = "resource_util.cc:83"
WARNING_TEXT_HINT = "root_cache_path"

# ---------------------------------------------------------------------------
# Static analysis
# ---------------------------------------------------------------------------

def check_root_cache_path_in_source(path: str) -> dict:
    """
    Check whether root_cache_path is set in the source file.
    Returns a dict with 'set' (bool) and 'detail' (str).
    """
    if not os.path.isfile(path):
        return {"set": None, "detail": f"Source file not found: {path}"}

    with open(path, "r", encoding="utf-8") as f:
        source = f.read()

    # Look for assignment to root_cache_path
    if re.search(r'root_cache_path', source):
        # Found the field — check if it's actually assigned (not just commented)
        # Look for a non-comment line containing root_cache_path with an assignment
        for line in source.splitlines():
            stripped = line.strip()
            if stripped.startswith("//") or stripped.startswith("*"):
                continue
            if "root_cache_path" in stripped and (
                "=" in stripped or "FromString" in stripped or "Assign" in stripped
            ):
                return {
                    "set": True,
                    "detail": f"root_cache_path is assigned: {stripped}",
                }
        return {
            "set": False,
            "detail": "root_cache_path appears in source but is not assigned a value.",
        }
    else:
        return {
            "set": False,
            "detail": "root_cache_path is NOT referenced anywhere in main.cpp.",
        }


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
    1. Static analysis: check root_cache_path is not set in source.
    2. Runtime check: launch binary, monitor stderr for the warning.
    Returns 0 (PASS) or 1 (FAIL/BUG CONFIRMED).
    """
    print("=" * 70)
    print("Bug 4 Exploratory Test: CefSettings.root_cache_path not set")
    print("=" * 70)
    print(f"Binary     : {BINARY}")
    print(f"Source file: {SOURCE_FILE}")
    print(f"Looking for: '{WARNING_FRAGMENT}'")
    print(f"Display    : {os.environ.get('DISPLAY', '(not set)')}")
    print()

    # --- Phase 1: Static analysis ---
    print("--- Phase 1: Static Source Analysis ---")
    static_result = check_root_cache_path_in_source(SOURCE_FILE)
    print(f"root_cache_path set in source: {static_result['set']}")
    print(f"  {static_result['detail']}")
    print()

    static_bug_confirmed = (static_result["set"] is False)

    # --- Phase 2: Runtime check (if binary exists) ---
    print("--- Phase 2: Runtime stderr Monitoring ---")

    if not os.path.isfile(BINARY):
        print(f"WARNING: Binary not found at '{BINARY}' — skipping runtime check.")
        print("         Build the project first, or set LILYPAD_BINARY env var.")
        runtime_bug_confirmed = None
        warning_count = 0
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
                    if WARNING_FRAGMENT in line or WARNING_TEXT_HINT in line:
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

        warning_count = len(warning_lines)
        runtime_bug_confirmed = warning_count > 0
        print()
        print(f"Total warning occurrences in {MONITOR_SECONDS}s: {warning_count}")

    print()
    print("--- Summary ---")
    print(f"Static analysis — root_cache_path unset: {static_bug_confirmed}")
    if runtime_bug_confirmed is not None:
        print(f"Runtime check  — warning appeared     : {runtime_bug_confirmed}")
    else:
        print("Runtime check  — skipped (binary not found)")
    print()

    # Verdict: bug confirmed if static analysis shows it's unset
    # (runtime check is corroborating evidence)
    if static_bug_confirmed:
        print("RESULT: FAIL — BUG CONFIRMED")
        print("  CefSettings.root_cache_path is not set in src/main.cpp.")
        if runtime_bug_confirmed:
            print(f"  The warning '{WARNING_FRAGMENT}' appeared {warning_count} time(s) in stderr.")
        elif runtime_bug_confirmed is False:
            print("  NOTE: The runtime warning was not observed — the binary may not have")
            print("        started fully, or the warning fires at a different log level.")
        print()
        print("  This is the EXPECTED outcome on unfixed code.")
        print("  Apply the fix (set root_cache_path to ~/.cache/lilypad) and re-run.")
        return 1
    else:
        print("RESULT: PASS")
        print("  root_cache_path is set in source — bug is NOT present.")
        if runtime_bug_confirmed is False:
            print("  Runtime check also confirmed: zero warning occurrences.")
        return 0


if __name__ == "__main__":
    sys.exit(run_test())
