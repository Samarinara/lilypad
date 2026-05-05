#!/usr/bin/env python3
"""
Convert bang.ts (TypeScript) to SQLite database for Lilypad browser.
Parses the TypeScript export and creates a bangs table with t, s, u fields.
"""

import re
import sqlite3
import sys
import os


def extract_json_from_ts(ts_path):
    """Extract JSON array from TypeScript file."""
    with open(ts_path, "r") as f:
        content = f.read()

    # Find the array start and end
    # Look for "export const bangs = [" and the closing "]"
    start_marker = "export const bangs = "
    start_idx = content.find(start_marker)
    if start_idx == -1:
        print("Error: Could not find 'export const bangs = ' in file")
        sys.exit(1)

    # Start after the marker
    json_start = start_idx + len(start_marker)

    # Find the matching closing bracket
    # Simple approach: find the last ] that closes the array
    # We need to find the closing ] that matches the opening [
    bracket_count = 0
    in_string = False
    escape_next = False
    pos = json_start

    # Skip whitespace to find opening [
    while pos < len(content) and content[pos] in " \t\n\r":
        pos += 1

    if pos >= len(content) or content[pos] != "[":
        print("Error: Expected '[' after export const bangs =")
        sys.exit(1)

    bracket_count = 1
    pos += 1

    while pos < len(content) and bracket_count > 0:
        char = content[pos]

        if escape_next:
            escape_next = False
            pos += 1
            continue

        if char == "\\":
            escape_next = True
            pos += 1
            continue

        if char == '"' and not in_string:
            in_string = True
        elif char == '"' and in_string:
            in_string = False
        elif not in_string:
            if char == "[":
                bracket_count += 1
            elif char == "]":
                bracket_count -= 1

        pos += 1

    if bracket_count > 0:
        print("Error: Unmatched brackets in file")
        sys.exit(1)

    json_content = content[json_start : pos - 1].strip()
    return json_content


def parse_bang_entry(entry_str):
    """Parse a single bang entry object string into a dict."""
    entry = {}

    # Extract fields using regex
    patterns = {
        "c": r'c:\s*"([^"]*)"',
        "d": r'd:\s*"([^"]*)"',
        "r": r"r:\s*(\d+)",
        "s": r's:\s*"([^"]*)"',
        "sc": r'sc:\s*"([^"]*)"',
        "t": r't:\s*"([^"]*)"',
        "u": r'u:\s*"([^"]*)"',
        "l": r'l:\s*"([^"]*)"',
    }

    for key, pattern in patterns.items():
        match = re.search(pattern, entry_str)
        if match:
            if key == "r":
                entry[key] = int(match.group(1))
            else:
                entry[key] = match.group(1)

    return entry


def convert_ts_to_sqlite(ts_path, db_path):
    """Convert TypeScript bang file to SQLite database."""
    print(f"Reading {ts_path}...")

    # Extract JSON-like content
    json_content = extract_json_from_ts(ts_path)

    print("Parsing bang entries...")

    # Parse individual entries
    # We'll use a simple approach: split by "},\n  {" and parse each
    entries = []

    # Find all entry objects
    # Pattern matches objects: { ... }
    entry_pattern = r"\{\s*c:[\s\S]*?u:[\s\S]*?\}(?=\s*,\s*\{|\s*\])"
    matches = re.finditer(entry_pattern, json_content)

    for match in matches:
        entry_str = match.group(0)
        entry = parse_bang_entry(entry_str)
        if "t" in entry and "u" in entry:
            entries.append(entry)

    print(f"Found {len(entries)} bang entries")

    # Create SQLite database
    print(f"Creating SQLite database at {db_path}...")

    # Remove existing db if present
    if os.path.exists(db_path):
        os.remove(db_path)

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    # Create bangs table
    cursor.execute("""
        CREATE TABLE bangs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            t TEXT,
            s TEXT,
            u TEXT,
            c TEXT,
            d TEXT,
            r INTEGER,
            sc TEXT,
            l TEXT
        )
    """)

    # Create indexes for fast lookup
    cursor.execute("CREATE INDEX idx_t ON bangs(t)")
    cursor.execute("CREATE INDEX idx_s ON bangs(s)")

    # Insert entries
    for entry in entries:
        cursor.execute(
            """
            INSERT INTO bangs (t, s, u, c, d, r, sc, l)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        """,
            (
                entry.get("t", ""),
                entry.get("s", ""),
                entry.get("u", ""),
                entry.get("c", ""),
                entry.get("d", ""),
                entry.get("r", 0),
                entry.get("sc", ""),
                entry.get("l", ""),
            ),
        )

    # Add the "fl" bang for fallback (Google I'm Feeling Lucky)
    cursor.execute(
        """
        INSERT INTO bangs (t, s, u, c, d, r, sc, l)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    """,
        (
            "fl",
            "Google I'm Feeling Lucky",
            "http://www.google.com/search?btnI&q={{{s}}}",
            "Search",
            "www.google.com",
            0,
            "Search",
            "eng",
        ),
    )

    conn.commit()
    conn.close()

    print("Done! Database created successfully.")


if __name__ == "__main__":
    ts_path = os.path.join(os.path.dirname(__file__), "..", "bang.ts")
    db_path = os.path.join(os.path.dirname(__file__), "..", "bangs.db")

    # Allow command line overrides
    if len(sys.argv) > 1:
        ts_path = sys.argv[1]
    if len(sys.argv) > 2:
        db_path = sys.argv[2]

    convert_ts_to_sqlite(ts_path, db_path)
