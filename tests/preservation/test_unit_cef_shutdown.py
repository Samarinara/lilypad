#!/usr/bin/env python3
"""
Preservation Unit Test — Task 7.2
===================================
Verifies that CefShutdown() is called exactly once, after qapp.exec(),
and is not inside a conditional block that could skip it.

**Validates: Requirements 3.2**

WHAT THIS TESTS
---------------
Requirement 3.2 states:
  WHEN the application is closed THEN the system SHALL CONTINUE TO shut down
  CEF cleanly via CefShutdown() without crashes or resource leaks.

The fix to main.cpp must not alter the shutdown sequence. This test uses
static source analysis to confirm:

  1. CefShutdown() appears exactly once in the main() function body.
  2. CefShutdown() appears AFTER qapp.exec() in the source (correct order:
     run the event loop first, then shut down CEF).
  3. CefShutdown() is NOT inside a conditional block (if/else/switch) that
     could cause it to be skipped under some conditions.

EXPECTED RESULT ON FIXED CODE
------------------------------
  PASS — CefShutdown() appears once, after qapp.exec(), unconditionally.

EXPECTED RESULT ON UNFIXED CODE
---------------------------------
  PASS — this path is not affected by the four bugs; it should pass on both
  unfixed and fixed code (preservation check).

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


def strip_line_comments(source: str) -> str:
    """Remove C++ single-line comments (// ...) from source."""
    return re.sub(r'//[^\n]*', '', source)


def strip_block_comments(source: str) -> str:
    """Remove C-style block comments (/* ... */) from source."""
    return re.sub(r'/\*.*?\*/', '', source, flags=re.DOTALL)


def strip_comments(source: str) -> str:
    return strip_line_comments(strip_block_comments(source))


def extract_main_body(source: str) -> str:
    """
    Extract the body of the main() function from source.
    Uses brace counting starting from the main() signature.
    Returns the content between the outermost braces (exclusive).
    """
    # Match: int main(int argc, char *argv[]) or int main(int argc, char** argv)
    match = re.search(r'\bint\s+main\s*\(', source)
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
                return source[start + 1:i]  # body without outer braces
    return source[start + 1:]


def find_all_occurrences(text: str, pattern: str) -> list:
    """Return list of (start_pos, line_number) for all matches of pattern."""
    results = []
    for m in re.finditer(pattern, text):
        line_no = text[:m.start()].count('\n') + 1
        results.append((m.start(), line_no))
    return results


def is_inside_conditional(text: str, pos: int) -> bool:
    """
    Heuristic: determine whether the character at `pos` in `text` is inside
    an if/else/switch/while/for block.

    Strategy: scan backwards from `pos` counting braces. When we reach depth 0
    (the enclosing block), look at the token immediately before the opening
    brace. If it is a conditional keyword, return True.
    """
    # Walk backwards to find the enclosing opening brace
    depth = 0
    i = pos
    while i >= 0:
        ch = text[i]
        if ch == '}':
            depth += 1
        elif ch == '{':
            if depth == 0:
                # Found the enclosing opening brace at position i
                # Look at the token before it
                before = text[:i].rstrip()
                # Check if the last token before '{' is a conditional keyword
                # or a closing paren (which follows if/for/while/switch)
                if before.endswith(')'):
                    # Find the matching '(' and look at the keyword before it
                    paren_depth = 0
                    j = len(before) - 1
                    while j >= 0:
                        if before[j] == ')':
                            paren_depth += 1
                        elif before[j] == '(':
                            paren_depth -= 1
                            if paren_depth == 0:
                                keyword_before = before[:j].rstrip()
                                # Extract last word
                                kw_match = re.search(r'\b(\w+)\s*$', keyword_before)
                                if kw_match:
                                    kw = kw_match.group(1)
                                    if kw in ('if', 'else', 'for', 'while', 'switch'):
                                        return True
                                break
                        j -= 1
                # Also check for bare 'else {'
                kw_match = re.search(r'\b(\w+)\s*$', before)
                if kw_match and kw_match.group(1) in ('else', 'try', 'catch'):
                    return True
                return False
            else:
                depth -= 1
        i -= 1
    return False


# ---------------------------------------------------------------------------
# Main test logic
# ---------------------------------------------------------------------------

def run_test() -> int:
    """
    Static analysis of CefShutdown() call in main().
    Returns 0 (PASS) or 1 (FAIL).
    """
    print("=" * 70)
    print("Preservation Unit Test 7.2: CefShutdown() called exactly once after qapp.exec()")
    print("=" * 70)
    print(f"Source file: {SOURCE_FILE}")
    print()

    if not os.path.isfile(SOURCE_FILE):
        print(f"ERROR: Source file not found at '{SOURCE_FILE}'")
        print("       Run from the repo root, or set LILYPAD_SOURCE env var.")
        return 1

    raw_source = load_source(SOURCE_FILE)
    source = strip_comments(raw_source)
    main_body = extract_main_body(source)

    if not main_body:
        print("ERROR: Could not locate main() function in source.")
        return 1

    failures = []

    # ------------------------------------------------------------------
    # Assertion 1: CefShutdown() appears exactly once in main()
    # ------------------------------------------------------------------
    shutdown_occurrences = find_all_occurrences(
        main_body, r'\bCefShutdown\s*\(\s*\)'
    )
    count = len(shutdown_occurrences)

    if count == 1:
        _, line_no = shutdown_occurrences[0]
        print(f"[PASS] CefShutdown() appears exactly once in main() (line ~{line_no}).")
    elif count == 0:
        failures.append(
            "FAIL: CefShutdown() does NOT appear in main().\n"
            "      CEF will not be shut down cleanly — resource leaks are likely."
        )
        print("[FAIL] CefShutdown() is MISSING from main().")
    else:
        failures.append(
            f"FAIL: CefShutdown() appears {count} times in main() (expected exactly 1).\n"
            "      Multiple shutdown calls may cause crashes or double-free errors."
        )
        print(f"[FAIL] CefShutdown() appears {count} times in main() (expected 1).")

    # ------------------------------------------------------------------
    # Assertion 2: CefShutdown() appears AFTER qapp.exec()
    # ------------------------------------------------------------------
    exec_occurrences = find_all_occurrences(
        main_body, r'\bqapp\s*\.\s*exec\s*\(\s*\)'
    )

    if not exec_occurrences:
        failures.append(
            "FAIL: qapp.exec() not found in main().\n"
            "      Cannot verify shutdown order."
        )
        print("[FAIL] qapp.exec() not found in main().")
    elif shutdown_occurrences:
        exec_pos = exec_occurrences[0][0]
        shutdown_pos = shutdown_occurrences[0][0]

        if shutdown_pos > exec_pos:
            print(
                f"[PASS] CefShutdown() (pos {shutdown_pos}) appears AFTER "
                f"qapp.exec() (pos {exec_pos}) — correct shutdown order."
            )
        else:
            failures.append(
                f"FAIL: CefShutdown() (pos {shutdown_pos}) appears BEFORE "
                f"qapp.exec() (pos {exec_pos}).\n"
                "      CEF would be shut down before the event loop exits — "
                "this will cause crashes."
            )
            print(
                f"[FAIL] CefShutdown() appears BEFORE qapp.exec() — wrong order."
            )

    # ------------------------------------------------------------------
    # Assertion 3: CefShutdown() is NOT inside a conditional block
    # ------------------------------------------------------------------
    if shutdown_occurrences:
        shutdown_pos = shutdown_occurrences[0][0]
        inside_conditional = is_inside_conditional(main_body, shutdown_pos)

        if not inside_conditional:
            print(
                "[PASS] CefShutdown() is NOT inside a conditional block — "
                "it will always be called."
            )
        else:
            failures.append(
                "FAIL: CefShutdown() appears to be inside a conditional block "
                "(if/else/switch/while/for).\n"
                "      Under some conditions, CefShutdown() could be skipped, "
                "causing resource leaks."
            )
            print(
                "[FAIL] CefShutdown() is inside a conditional block — "
                "it may be skipped."
            )

    print()

    if failures:
        print("RESULT: FAIL")
        for msg in failures:
            print(f"  {msg}")
        return 1

    print("RESULT: PASS")
    print("  CefShutdown() is called exactly once, after qapp.exec(), unconditionally.")
    return 0


if __name__ == "__main__":
    sys.exit(run_test())
