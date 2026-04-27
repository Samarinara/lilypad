# Input Passthrough Fix — Bugfix Design

## Overview

`CefOSRWidget` is the Qt widget that bridges Qt input events to the embedded Chromium (CEF) browser in off-screen rendering mode. Two input forwarding paths are broken:

1. **Keyboard events** — `keyPressEvent` and `keyReleaseEvent` contain only guard checks; the bodies are empty stubs that never call `SendKeyEvent`. No keyboard input reaches the web page.
2. **Mouse-leave events** — There is no `leaveEvent` override. When the cursor exits the widget, CEF never receives a mouse-leave notification, so CSS `:hover` states, tooltips, and dropdown menus remain permanently stuck in the hovered state.

The fix adds the missing `SendKeyEvent` calls inside the two existing key-event handlers and adds a new `leaveEvent` override that calls `SendMouseMoveEvent` with `mouseLeave = true`. All existing mouse and guard logic must remain unchanged.

---

## Glossary

- **Bug_Condition (C)**: The set of inputs that trigger defective behavior — key press/release events that reach `CefOSRWidget` while `m_inputEnabled` is `true` and the browser is valid, plus any cursor-exit event on the widget.
- **Property (P)**: The desired outcome for buggy inputs — CEF receives the corresponding `CefKeyEvent` or a mouse-leave `CefMouseEvent`.
- **Preservation**: All existing event-forwarding paths (mouse press, release, move, wheel) and all guard conditions (`m_inputEnabled == false`, null browser/handler) must behave identically before and after the fix.
- **`CefOSRWidget`**: The Qt widget in `src/cef_osr_widget.cpp` / `src/cef_osr_widget.h` that owns the CEF off-screen rendering surface and forwards Qt events to CEF.
- **`CefBrowserHost::SendKeyEvent`**: CEF API that delivers a `CefKeyEvent` to the browser's renderer process.
- **`CefBrowserHost::SendMouseMoveEvent`**: CEF API that delivers a mouse-move or mouse-leave event; the second parameter `mouseLeave` distinguishes the two cases.
- **`m_inputEnabled`**: Boolean flag on `CefOSRWidget` that suppresses all event forwarding when `false` (used for inactive tabs).
- **`m_handler`**: Pointer to the `CefHandler` that owns the `CefBrowser` instance.

---

## Bug Details

### Bug Condition

The bug manifests in two distinct situations:

1. A key press or key release event is delivered to `CefOSRWidget` while `m_inputEnabled` is `true` and a valid browser exists — the event is silently discarded instead of being forwarded.
2. The mouse cursor leaves the widget boundary — no `leaveEvent` override exists, so CEF is never notified.

**Formal Specification:**

```
FUNCTION isBugCondition(input)
  INPUT: input — a Qt event delivered to CefOSRWidget
  OUTPUT: boolean

  IF input IS QKeyEvent (press or release)
     AND m_inputEnabled = true
     AND m_handler != null
     AND m_handler->GetBrowser() != null
  THEN RETURN true   // key event will be silently dropped

  IF input IS QLeaveEvent
     AND m_inputEnabled = true
     AND m_handler != null
     AND m_handler->GetBrowser() != null
  THEN RETURN true   // hover state will remain stuck

  RETURN false
END FUNCTION
```

### Examples

- **Key press dropped**: User types "hello" in a text field on the web page → no characters appear. Expected: each key press produces a `KEYEVENT_RAWKEYDOWN` (and `KEYEVENT_CHAR` for printable keys) forwarded to CEF.
- **Key release dropped**: User holds Ctrl and releases it → CEF never sees the `KEYEVENT_KEYUP`. Expected: `KEYEVENT_KEYUP` is forwarded so modifier state is cleared.
- **Hover stuck**: User hovers over a navigation menu item (CSS `:hover` highlights it), then moves the mouse to a different widget → the menu item stays highlighted. Expected: `SendMouseMoveEvent(..., true)` is called, clearing the hover state.
- **Input disabled (no bug)**: `m_inputEnabled` is `false` (inactive tab) → key events and leave events are already suppressed by the guard; no change needed.

---

## Expected Behavior

### Preservation Requirements

**Unchanged Behaviors:**
- Mouse press events continue to call `SendMouseClickEvent` with `up = false`.
- Mouse release events continue to call `SendMouseClickEvent` with `up = true`.
- Mouse move events continue to call `SendMouseMoveEvent` with `mouseLeave = false`.
- Wheel events continue to call `SendMouseWheelEvent`.
- When `m_inputEnabled` is `false`, all event forwarding continues to be suppressed.
- When `m_handler` is null or `m_handler->GetBrowser()` is null, event forwarding continues to be skipped without crashing.
- `paintEvent` and `resizeEvent` behavior is completely unaffected.

**Scope:**
All inputs that do NOT satisfy `isBugCondition` must be completely unaffected by this fix. This includes:
- All mouse button and wheel events.
- Key events when `m_inputEnabled` is `false`.
- Key events when the browser or handler is null.
- Any other Qt events not explicitly handled.

---

## Hypothesized Root Cause

1. **Incomplete stub implementation**: `keyPressEvent` and `keyReleaseEvent` were scaffolded with the correct guard structure but the `SendKeyEvent` call was never written inside the inner `if` block. The stubs compile and run silently, making the omission invisible at runtime without explicit testing.

2. **Missing `leaveEvent` declaration and override**: `leaveEvent` was never added to the class declaration in `cef_osr_widget.h` nor implemented in `cef_osr_widget.cpp`. Qt's default `leaveEvent` does nothing, so there is no crash — only silent misbehavior (stuck hover states).

3. **No `QKeyEvent` include**: `cef_osr_widget.cpp` currently includes `<QMouseEvent>` but not `<QKeyEvent>`. The key-event handlers compile because `QKeyEvent` is transitively included via Qt headers on some platforms, but an explicit include is required for correctness and portability.

---

## Correctness Properties

Property 1: Bug Condition — Key Events Are Forwarded to CEF

_For any_ `QKeyEvent` delivered to `CefOSRWidget` where `m_inputEnabled` is `true` and a valid browser exists (isBugCondition returns true for key events), the fixed `keyPressEvent` SHALL call `SendKeyEvent` with a `CefKeyEvent` of type `KEYEVENT_RAWKEYDOWN` (and additionally `KEYEVENT_CHAR` for printable characters), and the fixed `keyReleaseEvent` SHALL call `SendKeyEvent` with type `KEYEVENT_KEYUP`.

**Validates: Requirements 2.1, 2.2**

Property 2: Bug Condition — Mouse Leave Is Forwarded to CEF

_For any_ `QLeaveEvent` delivered to `CefOSRWidget` where `m_inputEnabled` is `true` and a valid browser exists (isBugCondition returns true for leave events), the fixed `leaveEvent` SHALL call `SendMouseMoveEvent` with `mouseLeave = true`, clearing hover states in the browser.

**Validates: Requirements 2.3**

Property 3: Preservation — Existing Event Forwarding Is Unchanged

_For any_ input where the bug condition does NOT hold (isBugCondition returns false) — including all mouse events, wheel events, and any event when `m_inputEnabled` is `false` or the browser is null — the fixed `CefOSRWidget` SHALL produce exactly the same behavior as the original code.

**Validates: Requirements 3.1, 3.2, 3.3, 3.4, 3.5**

---

## Fix Implementation

### Changes Required

**File**: `src/cef_osr_widget.cpp`

**Change 1 — Add `<QKeyEvent>` include**
- Add `#include <QKeyEvent>` alongside the existing `#include <QMouseEvent>` for explicit, portable inclusion.

**Change 2 — Implement `keyPressEvent`**
- Inside the existing inner `if (m_handler && m_handler->GetBrowser())` block, construct a `CefKeyEvent`, populate `windows_key_code` from `event->nativeVirtualKey()`, `native_key_code` from `event->nativeScanCode()`, and `modifiers` from the Qt modifier flags mapped to CEF modifier constants.
- Call `host->SendKeyEvent(keyEvent)` with `type = KEYEVENT_RAWKEYDOWN`.
- For printable characters (`!event->text().isEmpty()`), send a second `CefKeyEvent` with `type = KEYEVENT_CHAR` and `character` set from the text.

**Change 3 — Implement `keyReleaseEvent`**
- Mirror the structure of `keyPressEvent` but use `type = KEYEVENT_KEYUP`.

**Change 4 — Add `leaveEvent` declaration to header**

**File**: `src/cef_osr_widget.h`

- Add `void leaveEvent(QEvent* event) override;` to the `protected` section alongside the other event overrides.

**Change 5 — Implement `leaveEvent`**

**File**: `src/cef_osr_widget.cpp`

- Add a new `CefOSRWidget::leaveEvent(QEvent* event)` method.
- Apply the same guard pattern as other event handlers: check `m_inputEnabled`, `m_handler`, and `m_handler->GetBrowser()`.
- Construct a `CefMouseEvent` with coordinates `(0, 0)` and `modifiers = 0` (position is irrelevant for leave).
- Call `host->SendMouseMoveEvent(cefEvent, true)`.

---

## Testing Strategy

### Validation Approach

Testing follows a two-phase approach: first run exploratory tests against the **unfixed** code to confirm the bug manifests as expected and to validate the root cause hypothesis; then run fix-checking and preservation tests against the **fixed** code.

### Exploratory Bug Condition Checking

**Goal**: Surface counterexamples that demonstrate the bug on unfixed code. Confirm or refute the root cause analysis.

**Test Plan**: Create a test harness that instantiates `CefOSRWidget` with a mock `CefHandler`/`CefBrowserHost`, synthesizes Qt events, and asserts that the corresponding CEF API calls are made. Run against unfixed code — these tests should fail, confirming the stubs never call `SendKeyEvent` and that no `leaveEvent` exists.

**Test Cases**:
1. **Key Press Not Forwarded**: Synthesize a `QKeyEvent` (press, key = Qt::Key_A) → assert `SendKeyEvent` was called with `KEYEVENT_RAWKEYDOWN` (will fail on unfixed code).
2. **Key Release Not Forwarded**: Synthesize a `QKeyEvent` (release, key = Qt::Key_A) → assert `SendKeyEvent` was called with `KEYEVENT_KEYUP` (will fail on unfixed code).
3. **Printable Char Not Forwarded**: Synthesize a `QKeyEvent` (press, key = Qt::Key_H, text = "h") → assert `SendKeyEvent` called twice: once with `KEYEVENT_RAWKEYDOWN`, once with `KEYEVENT_CHAR` (will fail on unfixed code).
4. **Leave Event Not Forwarded**: Synthesize a `QLeaveEvent` → assert `SendMouseMoveEvent` was called with `mouseLeave = true` (will fail on unfixed code — method doesn't exist).

**Expected Counterexamples**:
- `SendKeyEvent` is never called regardless of key input.
- `SendMouseMoveEvent(_, true)` is never called on cursor exit.
- Possible causes: empty stub bodies, missing `leaveEvent` override.

### Fix Checking

**Goal**: Verify that for all inputs where the bug condition holds, the fixed function produces the expected behavior.

**Pseudocode:**
```
FOR ALL input WHERE isBugCondition(input) DO
  result := CefOSRWidget_fixed.handleEvent(input)
  ASSERT expectedBehavior(result)
END FOR
```

### Preservation Checking

**Goal**: Verify that for all inputs where the bug condition does NOT hold, the fixed widget behaves identically to the original.

**Pseudocode:**
```
FOR ALL input WHERE NOT isBugCondition(input) DO
  ASSERT CefOSRWidget_original.handleEvent(input)
       = CefOSRWidget_fixed.handleEvent(input)
END FOR
```

**Testing Approach**: Property-based testing is well-suited here because:
- It generates many random mouse-event inputs automatically.
- It catches edge cases (e.g., null handler, `m_inputEnabled = false`) that manual tests might miss.
- It provides strong guarantees that the existing forwarding paths are unchanged.

**Test Cases**:
1. **Mouse Press Preservation**: Verify `SendMouseClickEvent(_, MBT_LEFT, false, 1)` is still called for mouse press events after the fix.
2. **Mouse Release Preservation**: Verify `SendMouseClickEvent(_, MBT_LEFT, true, 1)` is still called for mouse release events.
3. **Mouse Move Preservation**: Verify `SendMouseMoveEvent(_, false)` is still called for in-widget mouse moves.
4. **Wheel Preservation**: Verify `SendMouseWheelEvent` is still called with correct delta.
5. **Input Disabled Guard**: Verify no CEF calls are made for any event when `m_inputEnabled = false`.
6. **Null Handler Guard**: Verify no crash and no CEF calls when `m_handler` is null.

### Unit Tests

- Test `keyPressEvent` with a non-printable key → exactly one `SendKeyEvent(KEYEVENT_RAWKEYDOWN)` call.
- Test `keyPressEvent` with a printable key → two `SendKeyEvent` calls (`KEYEVENT_RAWKEYDOWN` then `KEYEVENT_CHAR`).
- Test `keyReleaseEvent` → exactly one `SendKeyEvent(KEYEVENT_KEYUP)` call.
- Test `leaveEvent` → exactly one `SendMouseMoveEvent(_, true)` call.
- Test all four new paths with `m_inputEnabled = false` → zero CEF calls.
- Test all four new paths with null handler → zero CEF calls, no crash.

### Property-Based Tests

- Generate random `QKeyEvent` inputs (random key codes, modifiers, text) with `m_inputEnabled = true` and valid browser → assert `SendKeyEvent` is always called at least once.
- Generate random `QKeyEvent` inputs with `m_inputEnabled = false` → assert `SendKeyEvent` is never called.
- Generate random mouse events (press, release, move, wheel) → assert the same CEF calls are produced by both original and fixed code (preservation).

### Integration Tests

- Full widget lifecycle: create `CefOSRWidget`, attach a real (or high-fidelity mock) `CefHandler`, type a sequence of keys, verify the browser's input buffer reflects the typed text.
- Hover-state clearing: simulate mouse enter → mouse move → mouse leave sequence; verify the leave triggers `SendMouseMoveEvent(..., true)`.
- Tab switching: enable input on widget A, disable on widget B; verify key events on B produce no CEF calls while key events on A are forwarded correctly.
