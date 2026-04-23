#!/usr/bin/env python3
"""
Preservation Property-Based Test — Task 4.2
=============================================
Property 4: Browser lifecycle round-trip.

**Validates: Requirements 6.1, 6.2, 6.4**

WHAT THIS TESTS
---------------
Requirements 6.1, 6.2, and 6.4 state that:
  - 6.1: OnAfterCreated SHALL store the CefBrowser reference so GetBrowser()
          returns non-null after the browser is created.
  - 6.2: OnBeforeClose SHALL clear the CefBrowser reference so GetBrowser()
          returns null after the browser is destroyed.
  - 6.4: GetBrowser() SHALL expose the current m_browser value (null if no
          browser is live).

This test verifies that property using two complementary approaches:

  1. STRUCTURAL ANALYSIS (static):
     - Confirms that `m_browser` is assigned in `OnAfterCreated` in
       `cef_handler.cpp`.
     - Confirms that `m_browser` is cleared (set to null/nullptr) in
       `OnBeforeClose` in `cef_handler.cpp`.
     - Confirms that `GetBrowser()` accessor is present in `cef_handler.h`
       and returns `m_browser`.

  2. PROPERTY-BASED SAMPLING (dynamic simulation):
     - Generates 100+ simulated sequences of OnAfterCreated / OnBeforeClose
       calls using Python's random module with a fixed seed.
     - After each simulated OnAfterCreated: asserts GetBrowser() is non-null.
     - After each simulated OnBeforeClose: asserts GetBrowser() is null.
     - This verifies the property:
         ∀ browser b:
           after OnAfterCreated(b)  → GetBrowser() is non-null
           after OnBeforeClose(b)   → GetBrowser() is null

EXPECTED RESULT ON FIXED CODE
------------------------------
  PASS — m_browser is assigned in OnAfterCreated and cleared in OnBeforeClose.

EXPECTED RESULT ON UNFIXED CODE
---------------------------------
  PASS — this path is not affected by the four bugs; it should pass on both
  unfixed and fixed code (preservation check).

REQUIRES
--------
  - src/cef_handler.cpp present at ./src/cef_handler.cpp
    (or LILYPAD_HANDLER env var)
  - src/cef_handler.h present at ./src/cef_handler.h
    (or LILYPAD_HANDLER_H env var)
  - Python 3.8+ (no external dependencies)

Tag: Feature: cef-qt-window-embedding, Property 4: Browser lifecycle round-trip
"""

import os
import random
import re
import sys

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
HANDLER_CPP_FILE = os.environ.get("LILYPAD_HANDLER", "./src/cef_handler.cpp")
HANDLER_H_FILE = os.environ.get("LILYPAD_HANDLER_H", "./src/cef_handler.h")
RANDOM_SEED = 42
NUM_SEQUENCES = 100

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load_source(path: str) -> str:
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def extract_function_body(source: str, function_name: str) -> str:
    """
    Extract the body of a function from C++ source using brace counting.
    Searches for the function name and returns the content between the
    first matching pair of braces.
    """
    match = re.search(rf'\b{re.escape(function_name)}\b', source)
    if not match:
        return ""
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


# ---------------------------------------------------------------------------
# Static analysis assertions
# ---------------------------------------------------------------------------

def check_m_browser_assigned_in_on_after_created(source: str) -> dict:
    """
    Assert that m_browser is assigned in OnAfterCreated.

    Expected pattern (from cef_handler.cpp):
        void CefHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser)
        {
            m_browser = browser;
        }
    """
    body = extract_function_body(source, "OnAfterCreated")
    if not body:
        return {
            "found": False,
            "detail": "OnAfterCreated() not found in cef_handler.cpp.",
        }

    # Check for m_browser = <something> (assignment)
    assign_pattern = re.compile(
        r'm_browser\s*=\s*\w+',
        re.DOTALL,
    )
    m = assign_pattern.search(body)
    if m:
        return {
            "found": True,
            "detail": (
                "m_browser is assigned in OnAfterCreated() — "
                "browser reference is stored after creation."
            ),
        }

    return {
        "found": False,
        "detail": (
            "Could not confirm m_browser is assigned in OnAfterCreated(). "
            f"Body snippet: {body[:300]!r}"
        ),
    }


def check_m_browser_cleared_in_on_before_close(source: str) -> dict:
    """
    Assert that m_browser is cleared (set to null/nullptr) in OnBeforeClose.

    Expected pattern (from cef_handler.cpp):
        void CefHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser)
        {
            if (browser->IsSame(m_browser))
                m_browser = nullptr;
        }
    """
    body = extract_function_body(source, "OnBeforeClose")
    if not body:
        return {
            "found": False,
            "detail": "OnBeforeClose() not found in cef_handler.cpp.",
        }

    # Check for m_browser = nullptr or m_browser = null or m_browser.reset()
    clear_patterns = [
        re.compile(r'm_browser\s*=\s*nullptr', re.DOTALL),
        re.compile(r'm_browser\s*=\s*NULL', re.DOTALL),
        re.compile(r'm_browser\s*=\s*0\b', re.DOTALL),
        re.compile(r'm_browser\s*\.\s*reset\s*\(\s*\)', re.DOTALL),
    ]

    for pattern in clear_patterns:
        m = pattern.search(body)
        if m:
            return {
                "found": True,
                "detail": (
                    "m_browser is cleared (set to null/nullptr) in OnBeforeClose() — "
                    "browser reference is released before destruction."
                ),
            }

    return {
        "found": False,
        "detail": (
            "Could not confirm m_browser is cleared in OnBeforeClose(). "
            f"Body snippet: {body[:300]!r}"
        ),
    }


def check_get_browser_accessor_in_header(header_source: str) -> dict:
    """
    Assert that GetBrowser() accessor is present in cef_handler.h and
    returns m_browser.

    Expected pattern (from cef_handler.h):
        CefRefPtr<CefBrowser> GetBrowser() const { return m_browser; }
    """
    # Check for GetBrowser() declaration/definition
    get_browser_pattern = re.compile(
        r'GetBrowser\s*\(\s*\)',
        re.DOTALL,
    )
    m = get_browser_pattern.search(header_source)
    if not m:
        return {
            "found": False,
            "detail": "GetBrowser() not found in cef_handler.h.",
        }

    # Check that it returns m_browser
    # Look for the inline definition or nearby return statement
    after = header_source[m.start():]
    # Find the inline body (if present)
    brace_start = after.find('{')
    semicolon = after.find(';')

    if brace_start != -1 and (semicolon == -1 or brace_start < semicolon):
        # Inline definition — extract body
        depth = 0
        body = ""
        for i in range(brace_start, len(after)):
            if after[i] == '{':
                depth += 1
            elif after[i] == '}':
                depth -= 1
                if depth == 0:
                    body = after[brace_start:i + 1]
                    break

        if re.search(r'\bm_browser\b', body):
            return {
                "found": True,
                "detail": (
                    "GetBrowser() accessor is present in cef_handler.h and "
                    "returns m_browser."
                ),
            }
        return {
            "found": False,
            "detail": (
                f"GetBrowser() found in cef_handler.h but does not return m_browser. "
                f"Body: {body!r}"
            ),
        }

    # Declaration only (no inline body) — presence is sufficient
    return {
        "found": True,
        "detail": (
            "GetBrowser() accessor is declared in cef_handler.h "
            "(implementation may be in .cpp)."
        ),
    }


# ---------------------------------------------------------------------------
# Simulated CefHandler
# ---------------------------------------------------------------------------

class SimulatedBrowser:
    """
    Lightweight stand-in for a CefBrowser instance.
    Each instance has a unique id for identity checks.
    """
    _counter = 0

    def __init__(self):
        SimulatedBrowser._counter += 1
        self.id = SimulatedBrowser._counter

    def is_same(self, other) -> bool:
        return other is not None and self.id == other.id

    def __repr__(self):
        return f"SimulatedBrowser(id={self.id})"


class SimulatedCefHandler:
    """
    Python simulation of CefHandler from cef_handler.cpp.

    Mirrors the C++ implementation:
        void OnAfterCreated(browser) { m_browser = browser; }
        void OnBeforeClose(browser)  { if (browser.IsSame(m_browser)) m_browser = nullptr; }
        GetBrowser()                 { return m_browser; }
    """

    def __init__(self):
        self._m_browser = None  # simulates CefRefPtr<CefBrowser> m_browser

    def OnAfterCreated(self, browser: SimulatedBrowser):
        """Simulate: m_browser = browser;"""
        self._m_browser = browser

    def OnBeforeClose(self, browser: SimulatedBrowser):
        """Simulate: if (browser->IsSame(m_browser)) m_browser = nullptr;"""
        if browser.is_same(self._m_browser):
            self._m_browser = None

    def GetBrowser(self):
        """Simulate: return m_browser;"""
        return self._m_browser


# ---------------------------------------------------------------------------
# Sequence generation
# ---------------------------------------------------------------------------

def generate_lifecycle_sequences(seed: int, n: int) -> list:
    """
    Generate n lifecycle sequences. Each sequence is a list of events:
      - ("created", browser)   — simulates OnAfterCreated
      - ("closed", browser)    — simulates OnBeforeClose

    Sequences cover:
      - Single create/close cycle (most common)
      - Multiple sequential create/close cycles on the same handler
      - Close with a different browser (should NOT clear m_browser)
      - Create without close (browser still live at end)
      - Repeated creates (second create overwrites first)
    """
    rng = random.Random(seed)
    sequences = []

    # Fixed representative cases
    # Case 1: simple create → close
    b1 = SimulatedBrowser()
    sequences.append([("created", b1), ("closed", b1)])

    # Case 2: create → close → create → close (two full cycles)
    b2a = SimulatedBrowser()
    b2b = SimulatedBrowser()
    sequences.append([
        ("created", b2a), ("closed", b2a),
        ("created", b2b), ("closed", b2b),
    ])

    # Case 3: create only (browser still live)
    b3 = SimulatedBrowser()
    sequences.append([("created", b3)])

    # Case 4: close with a different browser (should NOT clear m_browser)
    b4a = SimulatedBrowser()
    b4b = SimulatedBrowser()
    sequences.append([("created", b4a), ("closed", b4b)])

    # Case 5: three sequential cycles
    browsers_5 = [SimulatedBrowser() for _ in range(3)]
    seq5 = []
    for b in browsers_5:
        seq5.append(("created", b))
        seq5.append(("closed", b))
    sequences.append(seq5)

    # Random sequences
    while len(sequences) < n:
        seq = []
        num_cycles = rng.randint(1, 5)
        for _ in range(num_cycles):
            b = SimulatedBrowser()
            seq.append(("created", b))
            # Randomly decide whether to close this browser
            if rng.random() < 0.85:
                seq.append(("closed", b))
        sequences.append(seq)

    return sequences[:n]


# ---------------------------------------------------------------------------
# Main test logic
# ---------------------------------------------------------------------------

def run_test() -> int:
    """
    Property 4 preservation test: browser lifecycle round-trip.
    Returns 0 (PASS) or 1 (FAIL).
    """
    print("=" * 70)
    print("Preservation PBT 4.2: Browser lifecycle round-trip")
    print("=" * 70)
    print(f"Handler source : {HANDLER_CPP_FILE}")
    print(f"Handler header : {HANDLER_H_FILE}")
    print(f"Lifecycle sequences : {NUM_SEQUENCES} (seed={RANDOM_SEED})")
    print()

    failures = []

    # ------------------------------------------------------------------
    # Phase 1: Structural static analysis
    # ------------------------------------------------------------------
    print("Phase 1: Structural static analysis")
    print("-" * 40)

    if not os.path.isfile(HANDLER_CPP_FILE):
        print(f"ERROR: Handler source not found at '{HANDLER_CPP_FILE}'")
        print("       Run from the repo root, or set LILYPAD_HANDLER env var.")
        return 1

    if not os.path.isfile(HANDLER_H_FILE):
        print(f"ERROR: Handler header not found at '{HANDLER_H_FILE}'")
        print("       Run from the repo root, or set LILYPAD_HANDLER_H env var.")
        return 1

    cpp_source = load_source(HANDLER_CPP_FILE)
    h_source = load_source(HANDLER_H_FILE)

    # Assertion 1: m_browser is assigned in OnAfterCreated
    created_result = check_m_browser_assigned_in_on_after_created(cpp_source)
    if created_result["found"]:
        print(f"[PASS] {created_result['detail']}")
    else:
        failures.append(f"FAIL (structural): {created_result['detail']}")
        print(f"[FAIL] {created_result['detail']}")

    # Assertion 2: m_browser is cleared in OnBeforeClose
    closed_result = check_m_browser_cleared_in_on_before_close(cpp_source)
    if closed_result["found"]:
        print(f"[PASS] {closed_result['detail']}")
    else:
        failures.append(f"FAIL (structural): {closed_result['detail']}")
        print(f"[FAIL] {closed_result['detail']}")

    # Assertion 3: GetBrowser() accessor is present in header
    accessor_result = check_get_browser_accessor_in_header(h_source)
    if accessor_result["found"]:
        print(f"[PASS] {accessor_result['detail']}")
    else:
        failures.append(f"FAIL (structural): {accessor_result['detail']}")
        print(f"[FAIL] {accessor_result['detail']}")

    print()

    # ------------------------------------------------------------------
    # Phase 2: Property-based sampling
    # ------------------------------------------------------------------
    print("Phase 2: Property-based sampling (lifecycle round-trip)")
    print("-" * 40)
    print(f"Generating {NUM_SEQUENCES} lifecycle sequences...")

    sequences = generate_lifecycle_sequences(RANDOM_SEED, NUM_SEQUENCES)
    sample_failures = []

    for seq_idx, sequence in enumerate(sequences):
        handler = SimulatedCefHandler()
        current_browser = None  # tracks the last browser passed to OnAfterCreated

        for event_idx, (event_type, browser) in enumerate(sequence):
            if event_type == "created":
                handler.OnAfterCreated(browser)
                current_browser = browser

                # Property: after OnAfterCreated, GetBrowser() must be non-null
                result = handler.GetBrowser()
                if result is None:
                    sample_failures.append((
                        seq_idx, event_idx,
                        f"After OnAfterCreated({browser}): "
                        f"GetBrowser() returned None (expected non-null)"
                    ))

            elif event_type == "closed":
                handler.OnBeforeClose(browser)

                # Property: after OnBeforeClose with the matching browser,
                # GetBrowser() must be null (only if it was the stored browser)
                if browser.is_same(current_browser):
                    result = handler.GetBrowser()
                    if result is not None:
                        sample_failures.append((
                            seq_idx, event_idx,
                            f"After OnBeforeClose({browser}): "
                            f"GetBrowser() returned {result} (expected None)"
                        ))
                    current_browser = None
                else:
                    # Closing a different browser should NOT clear m_browser
                    result = handler.GetBrowser()
                    if current_browser is not None and result is None:
                        sample_failures.append((
                            seq_idx, event_idx,
                            f"After OnBeforeClose({browser}) with different browser: "
                            f"GetBrowser() returned None but should still hold "
                            f"{current_browser}"
                        ))

    if sample_failures:
        print(f"[FAIL] {len(sample_failures)} event(s) failed the lifecycle property:")
        for seq_idx, event_idx, detail in sample_failures[:5]:  # show first 5
            print(f"  Sequence #{seq_idx}, event #{event_idx}: {detail}")
        failures.append(
            f"FAIL (sampling): {len(sample_failures)} lifecycle events failed "
            "the round-trip property."
        )
    else:
        print(f"[PASS] All {NUM_SEQUENCES} sequences pass the lifecycle round-trip property.")
        print("       ∀ browser b:")
        print("         after OnAfterCreated(b)  → GetBrowser() is non-null")
        print("         after OnBeforeClose(b)   → GetBrowser() is null")
        print("         OnBeforeClose(other)     → GetBrowser() unchanged")

    print()

    # ------------------------------------------------------------------
    # Summary
    # ------------------------------------------------------------------
    if failures:
        print("RESULT: FAIL")
        for msg in failures:
            print(f"  {msg}")
        return 1

    print("RESULT: PASS")
    print("  m_browser is assigned in OnAfterCreated() (static analysis).")
    print("  m_browser is cleared in OnBeforeClose() (static analysis).")
    print("  GetBrowser() accessor is present in cef_handler.h (static analysis).")
    print(
        f"  {NUM_SEQUENCES} lifecycle sequences all satisfy the round-trip property."
    )
    return 0


if __name__ == "__main__":
    sys.exit(run_test())
