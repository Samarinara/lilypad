# Bugfix Requirements Document

## Introduction

The `CefOSRWidget` class is responsible for forwarding Qt input events to the embedded Chromium browser (CEF). Two categories of input are not being passed through correctly:

1. **Keyboard input**: `keyPressEvent` and `keyReleaseEvent` are implemented as empty stubs — they check the input-enabled flag and browser pointer but never call `SendKeyEvent` on the CEF browser host. As a result, no keyboard input (typing, shortcuts, arrow keys, etc.) reaches the web page.

2. **Hover/mouse-leave events**: There is no `leaveEvent` override to notify CEF when the cursor exits the widget. Without this, the browser never receives a mouse-leave signal, leaving hover states (CSS `:hover`, tooltips, dropdown menus) permanently stuck in the hovered state after the cursor moves away.

Both issues affect the active tab's `CefOSRWidget` and make the embedded browser effectively non-interactive for keyboard users and partially broken for mouse users.

---

## Bug Analysis

### Current Behavior (Defect)

1.1 WHEN the user presses a key while the active tab's `CefOSRWidget` has focus THEN the system does not forward the key event to the CEF browser, so no keyboard input reaches the web page

1.2 WHEN the user releases a key while the active tab's `CefOSRWidget` has focus THEN the system does not forward the key release event to the CEF browser

1.3 WHEN the mouse cursor moves outside the bounds of the active tab's `CefOSRWidget` THEN the system does not send a mouse-leave notification to the CEF browser, leaving hover states on the web page permanently active

### Expected Behavior (Correct)

2.1 WHEN the user presses a key while the active tab's `CefOSRWidget` has focus THEN the system SHALL forward a `CefKeyEvent` of type `KEYEVENT_RAWKEYDOWN` (and `KEYEVENT_CHAR` for printable characters) to the CEF browser host via `SendKeyEvent`

2.2 WHEN the user releases a key while the active tab's `CefOSRWidget` has focus THEN the system SHALL forward a `CefKeyEvent` of type `KEYEVENT_KEYUP` to the CEF browser host via `SendKeyEvent`

2.3 WHEN the mouse cursor leaves the bounds of the active tab's `CefOSRWidget` THEN the system SHALL call `SendMouseMoveEvent` on the CEF browser host with the `mouseLeave` parameter set to `true`, so that CSS hover states and tooltips are cleared correctly

### Unchanged Behavior (Regression Prevention)

3.1 WHEN `m_inputEnabled` is `false` THEN the system SHALL CONTINUE TO suppress all mouse and keyboard event forwarding to CEF (input isolation for inactive tabs is preserved)

3.2 WHEN the user moves the mouse within the bounds of the active tab's `CefOSRWidget` THEN the system SHALL CONTINUE TO forward `SendMouseMoveEvent` with `mouseLeave = false` as it does today

3.3 WHEN the user clicks or releases a mouse button within the active tab's `CefOSRWidget` THEN the system SHALL CONTINUE TO forward `SendMouseClickEvent` as it does today

3.4 WHEN the user scrolls the mouse wheel within the active tab's `CefOSRWidget` THEN the system SHALL CONTINUE TO forward `SendMouseWheelEvent` as it does today

3.5 WHEN the browser is null or the handler is null THEN the system SHALL CONTINUE TO skip event forwarding without crashing
