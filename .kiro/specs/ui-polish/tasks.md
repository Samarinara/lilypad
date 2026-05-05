# Tasks

## Task List

- [x] 1. Update Theme.qml with new color properties
  - [x] 1.1 Add `mutedBalticBlue` color property with value `#0f4070`
  - [x] 1.2 Update `darkUrlBar` to reference `Theme.shadowGrey`
  - [x] 1.3 Update `darkHover` to reference `Theme.mutedBalticBlue`

- [x] 2. Polish Main.qml top bar
  - [x] 2.1 Add a background `Rectangle` behind the `RowLayout` with `color: window.darkMode ? Theme.shadowGrey : Theme.softLinen`
  - [x] 2.2 Add a 1px bottom-border `Rectangle` colored `Theme.darkBorder` / `Theme.lightBorder` per mode
  - [x] 2.3 Apply 4px top padding to the `RowLayout` so the URL bar does not sit flush against the window edge

- [x] 3. Polish UrlBar.qml
  - [x] 3.1 Set `background.radius` to `6`
  - [x] 3.2 Change `placeholderText` to `"Search or enter address"`
  - [x] 3.3 Add `onActiveFocusChanged: if (activeFocus) selectAll()` to select all text on focus
  - [x] 3.4 Add `Behavior on border.color { ColorAnimation { duration: 80 } }` inside the background `Rectangle`
  - [x] 3.5 Add a `Label` (id: `securityIcon`) anchored to the left of the text input area showing `"🔒"` when text starts with `"https://"` and `"🌐"` otherwise
  - [x] 3.6 Set `leftPadding` to `28` to prevent icon/text overlap

- [x] 4. Polish TabEntry.qml — accent bar and active colors
  - [x] 4.1 Add a 3px-wide `Rectangle` (id: `accentBar`) anchored to the left edge, colored `Theme.darkEmerald`, `visible: isActive`
  - [x] 4.2 Update the root `Rectangle` `color` binding to use `Theme.petalFrost` (light active) and `Theme.balticBlue` (dark active)
  - [x] 4.3 Add `Behavior on color { ColorAnimation { duration: 90 } }` to the root `Rectangle`

- [x] 5. Polish TabEntry.qml — favicon area
  - [x] 5.1 Replace the `faviconLabel` `Label` with a 16×16 `Item` (id: `faviconArea`) containing three mutually exclusive children
  - [x] 5.2 Add the `FaviconPlaceholder` child: a `Rectangle` (16×16, radius 3, color `Theme.darkEmerald`) with a `Text` child showing `tabEntry.title[0].toUpperCase()` (or `"?"` fallback) in white at `font.pixelSize: 10`; visible when no favicon and not loading
  - [x] 5.3 Add the `LoadingIndicator` child: a `Rectangle` ring styled in `Theme.darkEmerald` with a `RotationAnimator` (duration 800, loops `Animation.Infinite`); visible when `tabEntry.isLoading`
  - [x] 5.4 Add the favicon `Image` child; visible when `image.status === Image.Ready` and not loading; falls back to placeholder on load error

- [x] 6. Polish TabPanel.qml
  - [x] 6.1 Add a 1px horizontal `Rectangle` divider between the `ScrollView` and the `Button`, colored `Theme.lightBorder` / `Theme.darkBorder` per mode
  - [x] 6.2 Update `newTabButton` height to `48`
  - [x] 6.3 Add a 3px left-border `Rectangle` to `newTabButton` colored `Theme.darkEmerald`
  - [x] 6.4 Set `newTabButton` text color to `Theme.darkEmerald` unconditionally (both modes)

- [x] 7. Polish HamburgerMenu.qml
  - [x] 7.1 Update the background `Rectangle` hover color to `Theme.petalFrost` (light) / `Theme.mutedBalticBlue` (dark)
  - [x] 7.2 Add a "New Tab" `MenuItem` that calls `tabManager.createTab("https://polli.page")` on trigger
  - [x] 7.3 Add a "Close Tab" `MenuItem` that calls `tabManager.closeTab(tabManager.activeTabId)` on trigger
  - [x] 7.4 Style both new `MenuItem` backgrounds with `Theme.petalFrost` hover (light) / `Theme.mutedBalticBlue` hover (dark)

- [x] 8. Write example-based tests
  - [x] 8.1 Write QML `TestCase` tests for Theme property values (Requirements 14.1–14.3)
  - [x] 8.2 Write QML `TestCase` tests for UrlBar static properties: radius, placeholder, leftPadding, Behavior duration (Requirements 2.1, 2.2, 3.2, 4.3)
  - [x] 8.3 Write QML `TestCase` tests for TopBar background, border, and padding (Requirements 1.1–1.3)
  - [x] 8.4 Write QML `TestCase` tests for TabEntry active background colors and Behavior duration (Requirements 6.1, 6.2, 7.1)
  - [x] 8.5 Write QML `TestCase` tests for LoadingIndicator structure and RotationAnimator (Requirements 9.1, 9.2)
  - [x] 8.6 Write QML `TestCase` tests for NewTabButton height, left border, and text color (Requirements 10.1–10.3)
  - [x] 8.7 Write QML `TestCase` tests for TabPanel divider (Requirement 11.1)
  - [x] 8.8 Write QML `TestCase` tests for HamburgerMenu hover colors and menu item triggers (Requirements 12.1, 12.2, 13.1–13.3)

- [x] 9. Write property-based tests
  - [x] 9.1 Write property test for Property 1: security icon reflects URL scheme (Requirements 4.1, 4.2)
  - [x] 9.2 Write property test for Property 2: URL bar selects all text on focus (Requirement 3.1)
  - [x] 9.3 Write property test for Property 3: accent bar visibility matches isActive (Requirements 5.1, 5.2)
  - [x] 9.4 Write property test for Property 4: favicon placeholder shows correct initial (Requirement 8.1)
  - [x] 9.5 Write property test for Property 5: favicon display priority ordering (Requirements 8.1, 8.2, 8.3)
