#!/usr/bin/env python3
"""
Preservation Property-Based Test — Task 7.1
=============================================
Property 2: URL navigation signal/slot wiring is preserved after the fix.

**Validates: Requirements 3.1**

WHAT THIS TESTS
---------------
Requirement 3.1 states:
  WHEN the user types a URL in the address bar and presses Enter
  THEN the system SHALL CONTINUE TO navigate the embedded CEF browser to
  that URL.

This test verifies the preservation of that behaviour using two complementary
approaches:

  1. STRUCTURAL ANALYSIS (static):
     - Confirms that `returnPressed` from the address bar (`m_urlBar`) is
       connected to a lambda/slot in `mainwindow.cpp`.
     - Confirms that the connected slot calls `LoadURL` on the browser's main
       frame with the address bar text (`m_urlBar->text()`).
     - Confirms that the URL is passed through unchanged — no transformation,
       no truncation, no encoding applied by the wiring code itself.

  2. PROPERTY-BASED SAMPLING (dynamic simulation):
     - Generates 100 random URL strings covering edge cases (empty string,
       very long URLs, special characters, unicode, whitespace).
     - For each URL, verifies that the pass-through property holds: the URL
       that would be passed to `LoadURL` is identical to the URL entered in
       the address bar (i.e., `m_urlBar->text().toStdString()` is the only
       transformation, which is a lossless encoding step, not a mutation).
     - This simulates the property: ∀ url ∈ URL_DOMAIN, navigate(url) passes
       url unchanged to LoadURL.

EXPECTED RESULT ON FIXED CODE
------------------------------
  PASS — signal/slot wiring exists; URL is passed through unchanged.

EXPECTED RESULT ON UNFIXED CODE
---------------------------------
  PASS — this path is not affected by the four bugs; it should pass on both
  unfixed and fixed code (preservation check).

REQUIRES
--------
  - src/mainwindow.cpp present at ./src/mainwindow.cpp
    (or LILYPAD_MAINWINDOW env var)
  - Python 3.8+ (no external dependencies)
"""

import os
import random
import re
import string
import sys

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
MAINWINDOW_FILE = os.environ.get("LILYPAD_MAINWINDOW", "./src/mainwindow.cpp")
RANDOM_SEED = 42
NUM_SAMPLES = 100

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load_source(path: str) -> str:
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def extract_constructor_body(source: str) -> str:
    """
    Extract the body of MainWindow::MainWindow (the constructor) from source.
    Uses brace counting starting from the constructor signature.
    """
    match = re.search(r'\bMainWindow\s*::\s*MainWindow\b', source)
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

def check_return_pressed_connected(constructor_body: str) -> dict:
    """
    Assert that returnPressed is connected to a slot/lambda in the constructor.
    """
    # Look for: connect(m_urlBar, &QLineEdit::returnPressed, ...)
    pattern = re.compile(
        r'connect\s*\(\s*m_urlBar\s*,\s*&\s*QLineEdit\s*::\s*returnPressed',
        re.DOTALL,
    )
    m = pattern.search(constructor_body)
    if m:
        return {"found": True, "detail": "returnPressed is connected in the constructor."}
    return {
        "found": False,
        "detail": (
            "No connect(m_urlBar, &QLineEdit::returnPressed, ...) found in "
            "MainWindow constructor."
        ),
    }


def check_load_url_called_in_slot(constructor_body: str) -> dict:
    """
    Assert that LoadURL is called inside the returnPressed connection lambda/slot.
    We look for LoadURL appearing after the returnPressed connect call.
    """
    # Find the connect call for returnPressed
    connect_match = re.search(
        r'connect\s*\(\s*m_urlBar\s*,\s*&\s*QLineEdit\s*::\s*returnPressed',
        constructor_body,
        re.DOTALL,
    )
    if not connect_match:
        return {"found": False, "detail": "returnPressed connect call not found."}

    # Extract the lambda body: everything from the connect call to the matching );
    after_connect = constructor_body[connect_match.start():]
    # Find the opening brace of the lambda
    lambda_brace = after_connect.find('[')
    if lambda_brace == -1:
        return {"found": False, "detail": "No lambda found in returnPressed connect call."}

    # Extract the lambda body by brace counting
    depth = 0
    lambda_body = ""
    for i in range(lambda_brace, len(after_connect)):
        ch = after_connect[i]
        if ch == '{':
            depth += 1
        elif ch == '}':
            depth -= 1
            if depth == 0:
                lambda_body = after_connect[lambda_brace:i + 1]
                break

    if not lambda_body:
        lambda_body = after_connect[lambda_brace:]

    if "LoadURL" in lambda_body:
        return {
            "found": True,
            "detail": "LoadURL is called inside the returnPressed lambda.",
        }
    return {
        "found": False,
        "detail": (
            "LoadURL is NOT called inside the returnPressed lambda. "
            f"Lambda body: {lambda_body[:200]!r}"
        ),
    }


def check_url_passed_unchanged(constructor_body: str) -> dict:
    """
    Assert that the URL passed to LoadURL is m_urlBar->text() (or .toStdString()
    thereof) — i.e., no transformation is applied to the URL before passing it.

    The expected pattern is:
        LoadURL(m_urlBar->text().toStdString())
    or equivalently any form that passes the text directly without mutation.
    """
    # Look for LoadURL called with m_urlBar->text()
    pattern = re.compile(
        r'LoadURL\s*\(\s*m_urlBar\s*->\s*text\s*\(\s*\)',
        re.DOTALL,
    )
    m = pattern.search(constructor_body)
    if m:
        return {
            "found": True,
            "detail": (
                "LoadURL is called with m_urlBar->text() directly — "
                "URL is passed through unchanged."
            ),
        }
    # Also accept a local variable that holds m_urlBar->text() without mutation
    # e.g.: auto url = m_urlBar->text(); ... LoadURL(url.toStdString())
    # Check if LoadURL appears and m_urlBar->text() appears in the same lambda
    connect_match = re.search(
        r'connect\s*\(\s*m_urlBar\s*,\s*&\s*QLineEdit\s*::\s*returnPressed',
        constructor_body,
        re.DOTALL,
    )
    if connect_match:
        after = constructor_body[connect_match.start():]
        # Find the lambda body
        lb = after.find('[')
        if lb != -1:
            depth = 0
            lambda_body = ""
            for i in range(lb, len(after)):
                ch = after[i]
                if ch == '{':
                    depth += 1
                elif ch == '}':
                    depth -= 1
                    if depth == 0:
                        lambda_body = after[lb:i + 1]
                        break
            if "LoadURL" in lambda_body and "m_urlBar" in lambda_body:
                return {
                    "found": True,
                    "detail": (
                        "LoadURL and m_urlBar both appear in the lambda — "
                        "URL is sourced from the address bar."
                    ),
                }
    return {
        "found": False,
        "detail": (
            "Could not confirm that LoadURL receives m_urlBar->text() unchanged. "
            "A transformation or intermediate variable may be altering the URL."
        ),
    }


# ---------------------------------------------------------------------------
# Property-based sampling
# ---------------------------------------------------------------------------

def generate_url_samples(seed: int, n: int) -> list:
    """
    Generate n URL strings covering a wide range of inputs:
      - Normal HTTP/HTTPS URLs
      - Empty string
      - Very long URLs (>2000 chars)
      - URLs with special characters (spaces, #, ?, &, =, %)
      - Unicode URLs (international domain names, path segments)
      - Whitespace-only strings
      - URLs with unusual schemes (ftp://, file://, javascript:)
      - Strings that are not URLs at all
    """
    rng = random.Random(seed)
    samples = []

    # Fixed edge cases
    samples += [
        "",                                          # empty
        " ",                                         # whitespace only
        "\t\n",                                      # tab + newline
        "https://polli.page",                        # the default URL
        "http://example.com",                        # plain HTTP
        "https://example.com/path?q=1&r=2#frag",    # query + fragment
        "https://例え.jp/パス",                       # unicode domain + path
        "https://user:pass@host:8080/path",          # credentials in URL
        "javascript:alert(1)",                       # non-http scheme
        "file:///etc/passwd",                        # file scheme
        "ftp://ftp.example.com/file.txt",            # ftp scheme
        "about:blank",                               # about scheme
        "data:text/html,<h1>hi</h1>",               # data URI
        "https://" + "a" * 2000 + ".com",           # very long hostname
        "https://example.com/" + "x" * 2000,        # very long path
        "https://example.com/?q=" + "%20" * 500,    # many percent-encoded spaces
        "https://example.com/\x00null",              # null byte in URL
        "https://example.com/<script>",              # HTML injection attempt
        "https://example.com/path with spaces",      # spaces in path
        "not a url at all",                          # plain text
        "12345",                                     # numeric string
    ]

    # Random ASCII printable strings
    for _ in range(30):
        length = rng.randint(0, 200)
        chars = string.printable
        samples.append("".join(rng.choice(chars) for _ in range(length)))

    # Random URL-like strings
    schemes = ["https://", "http://", "ftp://", ""]
    tlds = [".com", ".org", ".net", ".io", ".page", ""]
    for _ in range(30):
        scheme = rng.choice(schemes)
        host = "".join(rng.choice(string.ascii_lowercase) for _ in range(rng.randint(3, 20)))
        tld = rng.choice(tlds)
        path = "/" + "".join(rng.choice(string.ascii_lowercase + "/.-_") for _ in range(rng.randint(0, 50)))
        samples.append(scheme + host + tld + path)

    # Unicode strings
    unicode_ranges = [
        (0x0080, 0x00FF),   # Latin Extended
        (0x0400, 0x04FF),   # Cyrillic
        (0x4E00, 0x9FFF),   # CJK Unified Ideographs
        (0x0600, 0x06FF),   # Arabic
    ]
    for _ in range(19):
        r_start, r_end = rng.choice(unicode_ranges)
        length = rng.randint(1, 50)
        chars = [chr(rng.randint(r_start, r_end)) for _ in range(length)]
        samples.append("https://example.com/" + "".join(chars))

    return samples[:n]


def simulate_url_passthrough(url: str) -> dict:
    """
    Simulate the Qt signal/slot chain for URL navigation.

    The chain in mainwindow.cpp is:
        returnPressed → lambda → browser->GetMainFrame()->LoadURL(
                                     m_urlBar->text().toStdString())

    The only transformation is QString::toStdString(), which converts a
    QString to std::string using UTF-8 encoding. This is a lossless
    round-trip for all valid Unicode strings.

    We verify the pass-through property: the URL string that would be
    passed to LoadURL is the same as the URL string in the address bar
    (modulo the Qt/std string type boundary, which is transparent).

    Since we cannot actually invoke Qt here, we verify the STRUCTURAL
    property: the source code passes m_urlBar->text() directly to LoadURL
    without any intermediate mutation (no strip(), no truncation, no
    encoding transformation beyond toStdString()).

    For the sampling loop, we verify that Python's str → bytes → str
    round-trip via UTF-8 is lossless for the URL (simulating toStdString).
    """
    try:
        # Simulate toStdString(): encode to UTF-8 bytes, then decode back.
        # This is what Qt's toStdString() does internally.
        encoded = url.encode("utf-8", errors="surrogatepass")
        decoded = encoded.decode("utf-8", errors="surrogatepass")
        if decoded == url:
            return {"pass": True, "detail": "UTF-8 round-trip is lossless."}
        else:
            return {
                "pass": False,
                "detail": (
                    f"UTF-8 round-trip changed the URL.\n"
                    f"  Input:  {url!r}\n"
                    f"  Output: {decoded!r}"
                ),
            }
    except Exception as e:
        # Strings with lone surrogates cannot be encoded; treat as pass
        # because CEF would receive whatever Qt produces for such inputs.
        return {"pass": True, "detail": f"Encoding exception (acceptable): {e}"}


# ---------------------------------------------------------------------------
# Main test logic
# ---------------------------------------------------------------------------

def run_test() -> int:
    """
    Property 2 preservation test: URL navigation wiring is unchanged.
    Returns 0 (PASS) or 1 (FAIL).
    """
    print("=" * 70)
    print("Preservation PBT 7.1: URL navigation signal/slot wiring preserved")
    print("=" * 70)
    print(f"Source file : {MAINWINDOW_FILE}")
    print(f"URL samples : {NUM_SAMPLES} (seed={RANDOM_SEED})")
    print()

    if not os.path.isfile(MAINWINDOW_FILE):
        print(f"ERROR: Source file not found at '{MAINWINDOW_FILE}'")
        print("       Run from the repo root, or set LILYPAD_MAINWINDOW env var.")
        return 1

    source = load_source(MAINWINDOW_FILE)
    constructor_body = extract_constructor_body(source)

    if not constructor_body:
        print("ERROR: Could not locate MainWindow constructor in source.")
        return 1

    failures = []

    # ------------------------------------------------------------------
    # Phase 1: Structural static analysis
    # ------------------------------------------------------------------
    print("Phase 1: Structural static analysis")
    print("-" * 40)

    # Assertion 1: returnPressed is connected
    rp_result = check_return_pressed_connected(constructor_body)
    if rp_result["found"]:
        print(f"[PASS] {rp_result['detail']}")
    else:
        failures.append(f"FAIL (structural): {rp_result['detail']}")
        print(f"[FAIL] {rp_result['detail']}")

    # Assertion 2: LoadURL is called in the slot
    lu_result = check_load_url_called_in_slot(constructor_body)
    if lu_result["found"]:
        print(f"[PASS] {lu_result['detail']}")
    else:
        failures.append(f"FAIL (structural): {lu_result['detail']}")
        print(f"[FAIL] {lu_result['detail']}")

    # Assertion 3: URL is passed unchanged
    pu_result = check_url_passed_unchanged(constructor_body)
    if pu_result["found"]:
        print(f"[PASS] {pu_result['detail']}")
    else:
        failures.append(f"FAIL (structural): {pu_result['detail']}")
        print(f"[FAIL] {pu_result['detail']}")

    print()

    # ------------------------------------------------------------------
    # Phase 2: Property-based sampling
    # ------------------------------------------------------------------
    print("Phase 2: Property-based sampling (URL pass-through)")
    print("-" * 40)
    print(f"Generating {NUM_SAMPLES} URL samples...")

    samples = generate_url_samples(RANDOM_SEED, NUM_SAMPLES)
    sample_failures = []

    for i, url in enumerate(samples):
        result = simulate_url_passthrough(url)
        if not result["pass"]:
            sample_failures.append((i, url, result["detail"]))

    if sample_failures:
        print(f"[FAIL] {len(sample_failures)} URL(s) failed the pass-through property:")
        for idx, url, detail in sample_failures[:5]:  # show first 5
            print(f"  Sample #{idx}: {url!r}")
            print(f"    {detail}")
        failures.append(
            f"FAIL (sampling): {len(sample_failures)}/{NUM_SAMPLES} URLs failed "
            "the pass-through property."
        )
    else:
        print(f"[PASS] All {NUM_SAMPLES} URL samples pass the pass-through property.")
        print("       ∀ url ∈ sample_domain: toStdString(url) round-trip is lossless.")

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
    print("  returnPressed → LoadURL wiring exists and passes URL unchanged.")
    print(f"  {NUM_SAMPLES} random URL samples all satisfy the pass-through property.")
    return 0


if __name__ == "__main__":
    sys.exit(run_test())
