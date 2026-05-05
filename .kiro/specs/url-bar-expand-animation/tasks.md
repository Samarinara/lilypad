# Tasks

## Task List

- [x] 1. Create SearchButton.qml component
  - [x] 1.1 Create `qml/SearchButton.qml` with a 40×40 circular button using `Theme.darkEmerald` background and radius 20
  - [x] 1.2 Add centred "🔍" icon label
  - [x] 1.3 Add hover color binding (`Theme.darkHover` / `Theme.lightHover` based on `darkMode`)
  - [x] 1.4 Add `expandRequested` signal
  - [x] 1.5 Add `Keys.onSpacePressed` and `Keys.onReturnPressed` handlers that emit `expandRequested`
  - [x] 1.6 Set `activeFocusOnTab: true` on the button

- [x] 2. Update UrlBar.qml for expand/collapse support
  - [x] 2.1 Add `expanded` boolean property (default `false`)
  - [x] 2.2 Add `currentUrl` string property synced to the text field
  - [x] 2.3 Bind the text input and security icon `visible` to `expanded`
  - [x] 2.4 Keep all existing navigation logic (`onAccepted`, `Keys.onEscapePressed`, `navigateRequested` signal) unchanged
  - [x] 2.5 Remove the hardcoded `height: 40` (height managed by container)

- [x] 3. Create UrlBarContainer.qml component
  - [x] 3.1 Create `qml/UrlBarContainer.qml` with `expanded`, `darkMode`, and `currentUrl` properties
  - [x] 3.2 Embed `SearchButton` and `UrlBar` as children; show/hide based on `expanded`
  - [x] 3.3 Implement `expand()` function: set `expanded = true`, animate width from 40 to `parent.width`, animate background radius from 20 to 6, duration 250 ms, `Easing.OutCubic`
  - [x] 3.4 Implement `collapse()` function: set `expanded = false`, animate width from current to 40, animate background radius from 6 to 20, duration 200 ms, `Easing.InCubic`
  - [x] 3.5 Connect `SearchButton.expandRequested` to `expand()`
  - [x] 3.6 Connect `UrlBar.navigateRequested` to emit the container's own `navigateRequested` signal and call `collapse()`
  - [x] 3.7 Connect `UrlBar.onActiveFocusChanged` (focus lost) to `collapse()`
  - [x] 3.8 After expand animation completes, call `urlBar.forceActiveFocus()` then `urlBar.selectAll()`
  - [x] 3.9 Expose a `hamburgerOffset` property (or equivalent) that `Main.qml` can bind to for the hamburger slide-out

- [x] 4. Update Main.qml
  - [x] 4.1 Replace the `UrlBar` item in the `RowLayout` with `UrlBarContainer`
  - [x] 4.2 Add a `Behavior` on `HamburgerMenu`'s layout offset so it slides right (250 ms `OutCubic`) when `urlBarContainer.expanded === true` and slides back (200 ms `InCubic`) on collapse
  - [x] 4.3 Add a `Shortcut` for `Ctrl+L` that calls `urlBarContainer.expand()` when the bar is collapsed
  - [x] 4.4 Propagate `darkMode` and `currentUrl` (active tab URL) to `UrlBarContainer`
  - [x] 4.5 Connect `UrlBarContainer.navigateRequested` to `navigateToUrl()`
  - [x] 4.6 Update the `Connections` block for `tabManager.onActiveTabChanged` to set `urlBarContainer.currentUrl` instead of `urlBar.text`
  - [x] 4.7 Update the `WebEngineView.onUrlChanged` handler to set `urlBarContainer.currentUrl`

- [x] 5. Write property-based tests
  - [x] 5.1 Create `tests/qml/tst_urlbar_expand_animation.qml`
  - [x] 5.2 Write Property 1 test: for 100+ random strings, verify all text is selected after expand (`selectionStart === 0`, `selectionEnd === text.length`) — tag: `Feature: url-bar-expand-animation, Property 1`
  - [x] 5.3 Write Property 2 test: for 100+ random non-empty strings, verify `navigateRequested` is emitted on Enter — tag: `Feature: url-bar-expand-animation, Property 2`
  - [x] 5.4 Write Property 3 test: for 100+ random URL strings, verify `urlBar.text === currentUrl` in both expanded and collapsed states — tag: `Feature: url-bar-expand-animation, Property 3`
  - [x] 5.5 Write Property 4 test: for 100+ random URL strings (mix of https:// and non-https), verify security icon text is "🔒" iff URL starts with `https://` — tag: `Feature: url-bar-expand-animation, Property 4`

- [x] 6. Write example-based unit tests
  - [x] 6.1 Test SearchButton: diameter 40, radius 20, icon "🔍", background color `Theme.darkEmerald` in both modes (Req 1.1–1.5)
  - [x] 6.2 Test SearchButton hover colors: `Theme.lightHover` (light) / `Theme.darkHover` (dark) (Req 1.6)
  - [x] 6.3 Test SearchButton keyboard: Space and Enter emit `expandRequested`; `activeFocusOnTab: true` (Req 7.3, 7.4)
  - [x] 6.4 Test expand animation config: duration 250 ms, `Easing.OutCubic`; collapsed width 40, expanded width fills parent; collapsed radius 20, expanded radius 6 (Req 2.3, 2.6, 2.7)
  - [x] 6.5 Test collapse animation config: duration 200 ms, `Easing.InCubic`; width returns to 40; radius returns to 20 (Req 3.3, 3.5, 3.6)
  - [x] 6.6 Test visibility: security icon hidden when collapsed; TextField hidden when collapsed (Req 3.7, 5.3)
  - [x] 6.7 Test hamburger slide: offset applied when expanded, restored when collapsed (Req 2.2, 3.4)
  - [x] 6.8 Test Ctrl+L shortcut triggers expand (Req 7.1)
  - [x] 6.9 Test empty text + Enter → no `navigateRequested` signal (Req 4.5)
  - [x] 6.10 Test focus loss → collapse; Escape → collapse; `navigateRequested` → collapse (Req 3.1, 3.2, 4.2)
  - [x] 6.11 Test UrlBar theme colors: background, text, border (focused/unfocused, dark/light) (Req 6.1–6.6)

- [x] 7. Update existing tests and verify build
  - [x] 7.1 Update `tests/qml/tst_urlbar_props.qml` if the `UrlBar` public API changed (e.g., new `expanded` property)
  - [x] 7.2 Run the full QML test suite and confirm all existing tests still pass
  - [x] 7.3 Add `SearchButton.qml` and `UrlBarContainer.qml` to `resources.qrc`
  - [x] 7.4 Add `SearchButton` and `UrlBarContainer` exports to `qml/LilypadTheme/qmldir` if needed, or ensure they are accessible from `Main.qml`
