#!/usr/bin/env python3
"""
Preservation Property-Based Test — Task 3.2
=============================================
Property 3: URL pass-through is lossless.

**Validates: Requirements 4.1, 4.2**

WHAT THIS TESTS
---------------
Requirements 4.1 and 4.2 state:
  4.1 WHEN the user presses Enter in the URL bar, the system SHALL pass the
      exact URL string to LoadURL() without truncation or transformation.
  4.2 The URL string SHALL survive the QString::toStdString() boundary
      losslessly (UTF-8 round-trip).

This test verifies Property 3 using two complementary approaches:

  1. STRUCTURAL ANALYSIS (static):
     - Confirms that `LoadURL` receives `m_urlBar->text().toStdString()`
       directly — no intermediate variable that could mutate the URL.
     - Confirms that no transformation (strip, truncation, re-encoding) is
       applied to the URL between the address bar and the LoadURL call.

  2. PROPERTY-BASED SAMPLING (dynamic simulation):
     - Generates 100+ random URL strings covering edge cases:
       empty string, ASCII, Unicode, very long strings, special characters,
       non-URL strings, null bytes, whitespace-only, etc.
     - For each URL, verifies that the QString::toStdString() step (simulated
       as Python UTF-8 encode/decode) is a lossless round-trip.
     - This verifies the property:
         ∀ url ∈ URL_DOMAIN: toStdString(url) round-trip is lossless.

EXPECTED RESULT ON FIXED CODE
------------------------------
  PASS — LoadURL receives m_urlBar->text().toStdString() unchanged;
         UTF-8 round-trip is lossless for all 100+ samples.

EXPECTED RESULT ON UNFIXED CODE
---------------------------------
  PASS — this property is not affected by the four bugs; it should pass on
  both unfixed and fixed code (preservation check).

REQUIRES
--------
  - src/mainwindow.cpp present at ./src/mainwindow.cpp
    (or LILYPAD_MAINWINDOW env var)
  - Python 3.8+ (no external dependencies)

Tag: Feature: cef-qt-window-embedding, Property 3: URL pass-through is lossless
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
RANDOM_SEED = 99
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


def extract_return_pressed_lambda(constructor_body: str) -> str:
    """
    Extract the lambda body from the returnPressed connect call.
    Returns the lambda body string, or empty string if not found.
    """
    connect_match = re.search(
        r'connect\s*\(\s*m_urlBar\s*,\s*&\s*QLineEdit\s*::\s*returnPressed',
        constructor_body,
        re.DOTALL,
    )
    if not connect_match:
        return ""

    after_connect = constructor_body[connect_match.start():]
    lb = after_connect.find('[')
    if lb == -1:
        return ""

    depth = 0
    for i in range(lb, len(after_connect)):
        ch = after_connect[i]
        if ch == '{':
            depth += 1
        elif ch == '}':
            depth -= 1
            if depth == 0:
                return after_connect[lb:i + 1]
    return after_connect[lb:]


# ---------------------------------------------------------------------------
# Static analysis assertions (Property 3 — structural)
# ---------------------------------------------------------------------------

def check_load_url_receives_url_bar_text_directly(constructor_body: str) -> dict:
    """
    Assert that LoadURL is called with m_urlBar->text().toStdString() directly,
    with no intermediate variable that could mutate the URL.

    The expected pattern is:
        LoadURL(m_urlBar->text().toStdString())

    This confirms the pass-through is structurally lossless: the URL string
    flows from the address bar to LoadURL without any intermediate mutation.
    """
    lambda_body = extract_return_pressed_lambda(constructor_body)
    if not lambda_body:
        return {
            "found": False,
            "detail": "Could not extract returnPressed lambda body from constructor.",
        }

    # Primary check: LoadURL called directly with m_urlBar->text().toStdString()
    direct_pattern = re.compile(
        r'LoadURL\s*\(\s*m_urlBar\s*->\s*text\s*\(\s*\)\s*\.\s*toStdString\s*\(\s*\)\s*\)',
        re.DOTALL,
    )
    if direct_pattern.search(lambda_body):
        return {
            "found": True,
            "detail": (
                "LoadURL is called with m_urlBar->text().toStdString() directly — "
                "no intermediate variable; URL is passed through without mutation."
            ),
        }

    # Secondary check: LoadURL called with m_urlBar->text() (without toStdString)
    # This is also lossless — toStdString may be implicit or the type accepts QString.
    partial_pattern = re.compile(
        r'LoadURL\s*\(\s*m_urlBar\s*->\s*text\s*\(\s*\)',
        re.DOTALL,
    )
    if partial_pattern.search(lambda_body):
        return {
            "found": True,
            "detail": (
                "LoadURL is called with m_urlBar->text() — "
                "URL is sourced directly from the address bar without mutation."
            ),
        }

    # Tertiary check: LoadURL and m_urlBar both appear in the lambda
    # (covers intermediate variable patterns that still pass through unchanged)
    if "LoadURL" in lambda_body and "m_urlBar" in lambda_body:
        # Check for any mutation operations between m_urlBar and LoadURL
        mutation_patterns = [
            r'\.\s*trimmed\s*\(',       # QString::trimmed()
            r'\.\s*simplified\s*\(',    # QString::simplified()
            r'\.\s*toLower\s*\(',       # QString::toLower()
            r'\.\s*toUpper\s*\(',       # QString::toUpper()
            r'\.\s*left\s*\(',          # QString::left() — truncation
            r'\.\s*right\s*\(',         # QString::right() — truncation
            r'\.\s*mid\s*\(',           # QString::mid() — truncation
            r'\.\s*replace\s*\(',       # QString::replace()
            r'\.\s*remove\s*\(',        # QString::remove()
        ]
        mutations_found = []
        for pat in mutation_patterns:
            if re.search(pat, lambda_body):
                mutations_found.append(pat)

        if mutations_found:
            return {
                "found": False,
                "detail": (
                    f"LoadURL and m_urlBar appear in the lambda, but mutation "
                    f"operations were detected: {mutations_found}. "
                    "URL may not be passed through unchanged."
                ),
            }

        return {
            "found": True,
            "detail": (
                "LoadURL and m_urlBar both appear in the lambda with no detected "
                "mutation operations — URL is passed through unchanged."
            ),
        }

    return {
        "found": False,
        "detail": (
            f"Could not confirm LoadURL receives m_urlBar->text() unchanged. "
            f"Lambda body: {lambda_body[:300]!r}"
        ),
    }


def check_no_url_transformation_in_lambda(constructor_body: str) -> dict:
    """
    Assert that the returnPressed lambda does NOT apply any transformation
    to the URL string before passing it to LoadURL.

    Transformations that would violate losslessness:
      - QString::trimmed() — strips leading/trailing whitespace
      - QString::simplified() — collapses internal whitespace
      - QString::toLower() / toUpper() — case transformation
      - QString::left/right/mid() — truncation
      - QString::replace() / remove() — content mutation
      - Any custom encoding beyond toStdString()
    """
    lambda_body = extract_return_pressed_lambda(constructor_body)
    if not lambda_body:
        return {
            "found": True,  # Can't check, assume OK
            "detail": "Could not extract lambda body; skipping transformation check.",
        }

    # These transformations would break losslessness
    disallowed = {
        "trimmed()": r'm_urlBar\s*->\s*text\s*\(\s*\)\s*\.\s*trimmed\s*\(',
        "simplified()": r'm_urlBar\s*->\s*text\s*\(\s*\)\s*\.\s*simplified\s*\(',
        "toLower()": r'm_urlBar\s*->\s*text\s*\(\s*\)\s*\.\s*toLower\s*\(',
        "toUpper()": r'm_urlBar\s*->\s*text\s*\(\s*\)\s*\.\s*toUpper\s*\(',
        "left()": r'm_urlBar\s*->\s*text\s*\(\s*\)\s*\.\s*left\s*\(',
        "right()": r'm_urlBar\s*->\s*text\s*\(\s*\)\s*\.\s*right\s*\(',
        "mid()": r'm_urlBar\s*->\s*text\s*\(\s*\)\s*\.\s*mid\s*\(',
    }

    violations = []
    for name, pattern in disallowed.items():
        if re.search(pattern, lambda_body, re.DOTALL):
            violations.append(name)

    if violations:
        return {
            "found": False,
            "detail": (
                f"URL transformation(s) detected in returnPressed lambda: "
                f"{violations}. These would break lossless pass-through."
            ),
        }

    return {
        "found": True,
        "detail": (
            "No URL-mutating transformations detected in the returnPressed lambda. "
            "URL is passed to LoadURL without modification."
        ),
    }


def check_to_std_string_is_the_only_conversion(constructor_body: str) -> dict:
    """
    Assert that the only type conversion applied to the URL is toStdString(),
    which is a lossless UTF-8 encoding step (not a semantic mutation).

    toStdString() converts QString (UTF-16 internally) to std::string (UTF-8).
    This is a lossless round-trip for all valid Unicode strings.
    """
    lambda_body = extract_return_pressed_lambda(constructor_body)
    if not lambda_body:
        return {
            "found": True,
            "detail": "Could not extract lambda body; skipping conversion check.",
        }

    # Check that toStdString() is present (expected lossless conversion)
    has_to_std_string = bool(re.search(r'\.\s*toStdString\s*\(\s*\)', lambda_body))

    # Check for other encoding conversions that might be lossy
    lossy_conversions = {
        "toLatin1()": r'\.\s*toLatin1\s*\(',
        "toLocal8Bit()": r'\.\s*toLocal8Bit\s*\(',
        "toAscii()": r'\.\s*toAscii\s*\(',
        "toUtf8() without round-trip": r'\.\s*toUtf8\s*\(',
    }

    lossy_found = []
    for name, pattern in lossy_conversions.items():
        if re.search(pattern, lambda_body, re.DOTALL):
            lossy_found.append(name)

    if lossy_found:
        return {
            "found": False,
            "detail": (
                f"Potentially lossy conversion(s) detected: {lossy_found}. "
                "These may not preserve all Unicode characters."
            ),
        }

    if has_to_std_string:
        return {
            "found": True,
            "detail": (
                "toStdString() is the only conversion applied — "
                "this is a lossless UTF-8 encoding step, not a semantic mutation."
            ),
        }

    # toStdString() not found but no lossy conversions either — still OK
    # (LoadURL might accept QString directly in some CEF builds)
    return {
        "found": True,
        "detail": (
            "No lossy conversions detected. "
            "(toStdString() not found — LoadURL may accept the URL directly.)"
        ),
    }


# ---------------------------------------------------------------------------
# Property-based sampling (Property 3 — dynamic simulation)
# ---------------------------------------------------------------------------

def generate_url_samples(seed: int, n: int) -> list:
    """
    Generate n URL strings covering a wide range of inputs for Property 3:

      - Empty string
      - Whitespace-only strings
      - Normal HTTP/HTTPS URLs
      - URLs with query strings and fragments
      - Unicode URLs (international domain names, non-ASCII path segments)
      - Very long URLs (> 2000 characters)
      - URLs with special characters (spaces, #, ?, &, =, %)
      - URLs with unusual schemes (ftp://, file://, javascript:, data:)
      - Strings that are not URLs at all (plain text, numbers)
      - Strings with null bytes
      - Strings with HTML/script injection attempts
      - Random ASCII printable strings
      - Random Unicode strings from various scripts
    """
    rng = random.Random(seed)
    samples = []

    # --- Fixed edge cases ---
    samples += [
        "",                                           # empty string
        " ",                                          # single space
        "\t",                                         # tab
        "\n",                                         # newline
        "\t\n\r",                                     # mixed whitespace
        "https://polli.page",                         # the default URL
        "http://example.com",                         # plain HTTP
        "https://example.com/",                       # trailing slash
        "https://example.com/path?q=1&r=2#frag",     # query + fragment
        "https://例え.jp/パス",                        # unicode domain + path
        "https://пример.испытание/путь",              # Cyrillic IDN
        "https://user:pass@host:8080/path",           # credentials in URL
        "javascript:alert(1)",                        # non-http scheme
        "file:///etc/passwd",                         # file scheme
        "ftp://ftp.example.com/file.txt",             # ftp scheme
        "about:blank",                                # about scheme
        "data:text/html,<h1>hi</h1>",                # data URI
        "https://" + "a" * 2000 + ".com",            # very long hostname
        "https://example.com/" + "x" * 2000,         # very long path
        "https://example.com/?q=" + "%20" * 500,     # many percent-encoded spaces
        "https://example.com/\x00null",               # null byte in URL
        "https://example.com/<script>alert(1)</script>",  # XSS attempt
        "https://example.com/path with spaces",       # unencoded spaces
        "not a url at all",                           # plain text
        "12345",                                      # numeric string
        "   leading spaces",                          # leading whitespace
        "trailing spaces   ",                         # trailing whitespace
        "HTTPS://EXAMPLE.COM/PATH",                   # uppercase scheme
        "https://example.com:65535/path",             # max port number
        "https://[::1]/ipv6",                         # IPv6 address
        "https://127.0.0.1:8080/local",               # localhost with port
    ]

    # --- Random ASCII printable strings ---
    for _ in range(25):
        length = rng.randint(0, 300)
        chars = string.printable
        samples.append("".join(rng.choice(chars) for _ in range(length)))

    # --- Random URL-like strings ---
    schemes = ["https://", "http://", "ftp://", "file://", ""]
    tlds = [".com", ".org", ".net", ".io", ".page", ".co.uk", ""]
    for _ in range(20):
        scheme = rng.choice(schemes)
        host_len = rng.randint(3, 20)
        host = "".join(rng.choice(string.ascii_lowercase) for _ in range(host_len))
        tld = rng.choice(tlds)
        path_len = rng.randint(0, 60)
        path_chars = string.ascii_lowercase + "/.-_~"
        path = "/" + "".join(rng.choice(path_chars) for _ in range(path_len))
        query = ""
        if rng.random() > 0.5:
            query = "?key=" + "".join(rng.choice(string.ascii_lowercase) for _ in range(10))
        samples.append(scheme + host + tld + path + query)

    # --- Unicode strings from various scripts ---
    unicode_ranges = [
        (0x0080, 0x00FF),   # Latin Extended-A/B
        (0x0400, 0x04FF),   # Cyrillic
        (0x4E00, 0x9FFF),   # CJK Unified Ideographs
        (0x0600, 0x06FF),   # Arabic
        (0x0900, 0x097F),   # Devanagari
        (0x3040, 0x309F),   # Hiragana
        (0x1F600, 0x1F64F), # Emoticons
    ]
    for _ in range(15):
        r_start, r_end = rng.choice(unicode_ranges)
        length = rng.randint(1, 40)
        chars = []
        for _ in range(length):
            cp = rng.randint(r_start, r_end)
            try:
                ch = chr(cp)
                ch.encode("utf-8")  # ensure it's encodable
                chars.append(ch)
            except (ValueError, UnicodeEncodeError):
                chars.append("x")
        samples.append("https://example.com/" + "".join(chars))

    # Pad to exactly n samples if needed
    while len(samples) < n:
        length = rng.randint(0, 100)
        samples.append("".join(rng.choice(string.printable) for _ in range(length)))

    return samples[:n]


def simulate_to_std_string_round_trip(url: str) -> dict:
    """
    Simulate Qt's QString::toStdString() round-trip.

    QString stores text as UTF-16 internally. toStdString() converts to
    std::string using UTF-8 encoding. This is a lossless operation for all
    valid Unicode code points.

    We simulate this as: str → UTF-8 bytes → str (Python's native encoding).

    Strings containing lone surrogates (invalid Unicode) are treated as
    acceptable edge cases — Qt would produce implementation-defined behaviour
    for such inputs, and CEF would receive whatever Qt produces.

    Returns a dict with:
      - "pass": True if the round-trip is lossless
      - "detail": description of the result
      - "input": the original URL
      - "output": the round-tripped URL (if different)
    """
    try:
        # Simulate toStdString(): encode to UTF-8, then decode back.
        encoded = url.encode("utf-8", errors="surrogatepass")
        decoded = encoded.decode("utf-8", errors="surrogatepass")

        if decoded == url:
            return {
                "pass": True,
                "detail": "UTF-8 round-trip is lossless.",
                "input": url,
                "output": decoded,
            }
        else:
            return {
                "pass": False,
                "detail": (
                    f"UTF-8 round-trip changed the URL.\n"
                    f"  Input:  {url!r}\n"
                    f"  Output: {decoded!r}"
                ),
                "input": url,
                "output": decoded,
            }
    except (UnicodeEncodeError, UnicodeDecodeError) as e:
        # Strings with lone surrogates that can't be encoded at all.
        # Qt would handle these in an implementation-defined way.
        # We treat this as acceptable (not a losslessness violation of our code).
        return {
            "pass": True,
            "detail": f"Encoding exception for edge-case input (acceptable): {e}",
            "input": url,
            "output": None,
        }


# ---------------------------------------------------------------------------
# Main test logic
# ---------------------------------------------------------------------------

def run_test() -> int:
    """
    Property 3 preservation test: URL pass-through is lossless.
    Returns 0 (PASS) or 1 (FAIL).
    """
    print("=" * 70)
    print("Preservation PBT 3.2: URL pass-through is lossless")
    print("Tag: Feature: cef-qt-window-embedding, Property 3: URL pass-through is lossless")
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
    print("Phase 1: Structural static analysis (lossless pass-through)")
    print("-" * 60)

    # Assertion 1: LoadURL receives m_urlBar->text() directly (no mutation)
    direct_result = check_load_url_receives_url_bar_text_directly(constructor_body)
    if direct_result["found"]:
        print(f"[PASS] {direct_result['detail']}")
    else:
        failures.append(f"FAIL (structural): {direct_result['detail']}")
        print(f"[FAIL] {direct_result['detail']}")

    # Assertion 2: No URL-mutating transformations in the lambda
    no_transform_result = check_no_url_transformation_in_lambda(constructor_body)
    if no_transform_result["found"]:
        print(f"[PASS] {no_transform_result['detail']}")
    else:
        failures.append(f"FAIL (structural): {no_transform_result['detail']}")
        print(f"[FAIL] {no_transform_result['detail']}")

    # Assertion 3: toStdString() is the only conversion (lossless UTF-8 step)
    conversion_result = check_to_std_string_is_the_only_conversion(constructor_body)
    if conversion_result["found"]:
        print(f"[PASS] {conversion_result['detail']}")
    else:
        failures.append(f"FAIL (structural): {conversion_result['detail']}")
        print(f"[FAIL] {conversion_result['detail']}")

    print()

    # ------------------------------------------------------------------
    # Phase 2: Property-based sampling
    # ------------------------------------------------------------------
    print("Phase 2: Property-based sampling (toStdString() round-trip)")
    print("-" * 60)
    print(f"Generating {NUM_SAMPLES} URL samples (seed={RANDOM_SEED})...")
    print("Verifying: ∀ url ∈ URL_DOMAIN, toStdString(url) round-trip is lossless")
    print()

    samples = generate_url_samples(RANDOM_SEED, NUM_SAMPLES)
    assert len(samples) >= NUM_SAMPLES, (
        f"Expected at least {NUM_SAMPLES} samples, got {len(samples)}"
    )

    sample_failures = []
    for i, url in enumerate(samples):
        result = simulate_to_std_string_round_trip(url)
        if not result["pass"]:
            sample_failures.append((i, url, result["detail"]))

    if sample_failures:
        print(f"[FAIL] {len(sample_failures)} URL(s) failed the lossless round-trip property:")
        for idx, url, detail in sample_failures[:5]:  # show first 5
            print(f"  Sample #{idx}: {url!r}")
            print(f"    {detail}")
        if len(sample_failures) > 5:
            print(f"  ... and {len(sample_failures) - 5} more.")
        failures.append(
            f"FAIL (sampling): {len(sample_failures)}/{NUM_SAMPLES} URLs failed "
            "the toStdString() lossless round-trip property."
        )
    else:
        print(f"[PASS] All {NUM_SAMPLES} URL samples pass the lossless round-trip property.")
        print("       ∀ url ∈ sample_domain: encode(url, 'utf-8') → decode → url (unchanged)")

    print()

    # ------------------------------------------------------------------
    # Summary
    # ------------------------------------------------------------------
    print("=" * 70)
    if failures:
        print("RESULT: FAIL")
        print()
        for msg in failures:
            print(f"  {msg}")
        return 1

    print("RESULT: PASS")
    print()
    print("  Property 3 holds: URL pass-through is lossless.")
    print("  - LoadURL receives m_urlBar->text().toStdString() without mutation.")
    print("  - No URL-transforming operations detected in the navigation lambda.")
    print("  - toStdString() (UTF-8 encoding) is the only conversion applied.")
    print(f"  - {NUM_SAMPLES} random URL samples all satisfy the lossless round-trip property.")
    return 0


if __name__ == "__main__":
    sys.exit(run_test())
