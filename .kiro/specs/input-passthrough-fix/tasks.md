# Input Passthrough Fix — Tasks

## Task List

- [x] 1. Add `<QKeyEvent>` include to `cef_osr_widget.cpp`
  - [x] 1.1 Add `#include <QKeyEvent>` alongside the existing `#include <QMouseEvent>` in `src/cef_osr_widget.cpp`

- [x] 2. Implement `keyPressEvent` in `src/cef_osr_widget.cpp`
  - [x] 2.1 Inside the existing `if (m_handler && m_handler->GetBrowser())` block, obtain the `CefBrowserHost` via `m_handler->GetBrowser()->GetHost()`
  - [x] 2.2 Construct a `CefKeyEvent`, set `windows_key_code` from `event->nativeVirtualKey()`, `native_key_code` from `event->nativeScanCode()`, and map Qt modifiers to CEF modifier flags in `modifiers`
  - [x] 2.3 Set `type = KEYEVENT_RAWKEYDOWN` and call `host->SendKeyEvent(keyEvent)`
  - [x] 2.4 For printable characters (`!event->text().isEmpty()`), send a second `CefKeyEvent` with `type = KEYEVENT_CHAR` and `character` / `unmodified_character` set from `event->text().at(0).unicode()`

- [x] 3. Implement `keyReleaseEvent` in `src/cef_osr_widget.cpp`
  - [x] 3.1 Mirror the structure of `keyPressEvent` but set `type = KEYEVENT_KEYUP`
  - [x] 3.2 Call `host->SendKeyEvent(keyEvent)` — no `KEYEVENT_CHAR` is needed for key release

- [x] 4. Declare `leaveEvent` override in `src/cef_osr_widget.h`
  - [x] 4.1 Add `void leaveEvent(QEvent* event) override;` to the `protected` section of `CefOSRWidget`, alongside the other event overrides

- [x] 5. Implement `leaveEvent` in `src/cef_osr_widget.cpp`
  - [x] 5.1 Add `CefOSRWidget::leaveEvent(QEvent* event)` with the same `m_inputEnabled` and null-handler guards used by the other event handlers
  - [x] 5.2 Construct a `CefMouseEvent` with `x = 0`, `y = 0`, `modifiers = 0`
  - [x] 5.3 Call `host->SendMouseMoveEvent(cefEvent, true)` to notify CEF that the cursor has left the widget

- [x] 6. Write exploratory tests (run against unfixed code to confirm bug)
  - [x] 6.1 Write a test that synthesizes a `QKeyEvent` press and asserts `SendKeyEvent` is called with `KEYEVENT_RAWKEYDOWN` — expect failure on unfixed code
  - [x] 6.2 Write a test that synthesizes a `QKeyEvent` release and asserts `SendKeyEvent` is called with `KEYEVENT_KEYUP` — expect failure on unfixed code
  - [x] 6.3 Write a test that synthesizes a printable key press and asserts two `SendKeyEvent` calls (`KEYEVENT_RAWKEYDOWN` + `KEYEVENT_CHAR`) — expect failure on unfixed code
  - [x] 6.4 Write a test that synthesizes a `QLeaveEvent` and asserts `SendMouseMoveEvent` is called with `mouseLeave = true` — expect failure on unfixed code

- [x] 7. Write fix-checking unit tests (run against fixed code — Property 1 and Property 2)
  - [x] 7.1 Test `keyPressEvent` with a non-printable key: assert exactly one `SendKeyEvent(KEYEVENT_RAWKEYDOWN)` call
  - [x] 7.2 Test `keyPressEvent` with a printable key: assert two `SendKeyEvent` calls in order (`KEYEVENT_RAWKEYDOWN`, then `KEYEVENT_CHAR`)
  - [x] 7.3 Test `keyReleaseEvent`: assert exactly one `SendKeyEvent(KEYEVENT_KEYUP)` call
  - [x] 7.4 Test `leaveEvent`: assert exactly one `SendMouseMoveEvent` call with `mouseLeave = true`
  - [x] 7.5 Test all four new paths with `m_inputEnabled = false`: assert zero CEF calls
  - [x] 7.6 Test all four new paths with null `m_handler`: assert zero CEF calls and no crash

- [x] 8. Write preservation tests (run against fixed code — Property 3)
  - [x] 8.1 Test `mousePressEvent` still calls `SendMouseClickEvent(_, MBT_LEFT, false, 1)`
  - [x] 8.2 Test `mouseReleaseEvent` still calls `SendMouseClickEvent(_, MBT_LEFT, true, 1)`
  - [x] 8.3 Test `mouseMoveEvent` still calls `SendMouseMoveEvent(_, false)`
  - [x] 8.4 Test `wheelEvent` still calls `SendMouseWheelEvent` with the correct Y delta
  - [x] 8.5 Test that all existing mouse events produce zero CEF calls when `m_inputEnabled = false`

- [-] 9. Build and verify
  - [ ] 9.1 Compile the project and confirm zero new warnings or errors
  - [ ] 9.2 Run the full test suite and confirm all tests pass
