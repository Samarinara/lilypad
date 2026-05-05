# Requirements Document

## Introduction

This feature delivers a comprehensive UI polish and UX improvement pass for the Lilypad browser's Qt/QML interface. The goal is to refine visual consistency, improve micro-interactions, and align all components with the established Lilypad color palette (darkEmerald, petalFrost, softLinen, balticBlue, shadowGrey). Changes span six QML files: `Main.qml`, `TabPanel.qml`, `TabEntry.qml`, `UrlBar.qml`, `HamburgerMenu.qml`, and `Theme.qml`.

## Glossary

- **TopBar**: The `RowLayout` in `Main.qml` that contains the URL bar and hamburger menu button.
- **TabPanel**: The sidebar `Rectangle` in `TabPanel.qml` that lists open tabs and the new-tab button.
- **TabEntry**: The individual tab row `Rectangle` in `TabEntry.qml`.
- **UrlBar**: The `TextField` in `UrlBar.qml` used for URL input and navigation.
- **HamburgerMenu**: The `Button` in `HamburgerMenu.qml` that opens the application menu.
- **NewTabButton**: The `Button` at the bottom of `TabPanel` that creates a new tab.
- **FaviconPlaceholder**: A colored `Rectangle` containing an initial letter, shown when no favicon image is available.
- **LoadingIndicator**: An animated rotating visual element shown while a tab is loading.
- **Theme**: The singleton `QtObject` in `Theme.qml` that defines all color constants.
- **ActiveTab**: The `TabEntry` whose `tabId` matches `tabManager.activeTabId`.
- **AccentBar**: A 3px-wide colored strip on the left edge of the `ActiveTab`.
- **FocusBorder**: The colored border applied to `UrlBar` when it has active keyboard focus.
- **darkEmerald**: `#066633` — primary brand accent color.
- **petalFrost**: `#f8d1e8` — light-mode hover and active surface color.
- **softLinen**: `#e8ebe4` — light-mode background color.
- **balticBlue**: `#145c9e` — dark-mode hover and active surface color.
- **shadowGrey**: `#211a1d` — dark-mode background color.
- **mutedBalticBlue**: A desaturated variant of balticBlue used for dark-mode hover states to avoid full-saturation contrast.

---

## Requirements

### Requirement 1: Top Bar Background and Border

**User Story:** As a user, I want the top bar to have a distinct background and a subtle bottom border, so that it is visually separated from the web content area below it.

#### Acceptance Criteria

1. THE TopBar SHALL display a background color of `Theme.softLinen` in light mode and `Theme.shadowGrey` in dark mode.
2. THE TopBar SHALL display a 1px bottom border in color `Theme.lightBorder` in light mode and `Theme.darkBorder` in dark mode.
3. THE TopBar SHALL apply 4px of top padding so the URL bar does not sit flush against the window edge.

---

### Requirement 2: URL Bar Corner Radius and Placeholder

**User Story:** As a user, I want the URL bar to have rounded corners and a descriptive placeholder, so that it looks polished and communicates its purpose clearly.

#### Acceptance Criteria

1. THE UrlBar background `Rectangle` SHALL have a `radius` of 6.
2. THE UrlBar SHALL display placeholder text "Search or enter address".

---

### Requirement 3: URL Bar Focus Behavior

**User Story:** As a user, I want the URL bar to select all text when I click it and to animate its border color on focus, so that editing is fast and the focus state is visually clear.

#### Acceptance Criteria

1. WHEN the UrlBar receives active focus, THE UrlBar SHALL select all text in the field.
2. WHEN the UrlBar gains or loses active focus, THE UrlBar background border color SHALL transition using a `Behavior` animation with a duration of 80ms.

---

### Requirement 4: URL Bar Security Icon

**User Story:** As a user, I want to see a lock icon for HTTPS pages and a globe icon for HTTP pages in the URL bar, so that I can quickly assess the security of the current page.

#### Acceptance Criteria

1. WHEN the UrlBar text begins with "https://", THE UrlBar SHALL display a lock icon ("🔒") to the left of the text input area.
2. WHEN the UrlBar text does not begin with "https://", THE UrlBar SHALL display a globe icon ("🌐") to the left of the text input area.
3. THE UrlBar left padding SHALL be increased to accommodate the icon so that the icon and text input do not overlap.

---

### Requirement 5: Active Tab Accent Bar

**User Story:** As a user, I want the active tab to show a colored left-side accent bar, so that I can immediately identify which tab is currently selected.

#### Acceptance Criteria

1. WHEN a TabEntry `isActive` is `true`, THE TabEntry SHALL display a 3px-wide `Rectangle` on its left edge colored `Theme.darkEmerald`.
2. WHEN a TabEntry `isActive` is `false`, THE TabEntry SHALL not display the accent bar.

---

### Requirement 6: Active Tab Background Color

**User Story:** As a user, I want the active tab to use the brand palette colors instead of a generic grey, so that the active state is visually consistent with the rest of the UI.

#### Acceptance Criteria

1. WHEN a TabEntry `isActive` is `true` and `darkMode` is `false`, THE TabEntry background SHALL use `Theme.petalFrost`.
2. WHEN a TabEntry `isActive` is `true` and `darkMode` is `true`, THE TabEntry background SHALL use `Theme.balticBlue`.

---

### Requirement 7: Tab Background Color Transition

**User Story:** As a user, I want tab background colors to animate smoothly when the active state or hover state changes, so that the UI feels fluid rather than abrupt.

#### Acceptance Criteria

1. WHEN the TabEntry background color changes due to `isActive` or hover state, THE TabEntry background color SHALL transition using a `Behavior` animation with a duration of 90ms.

---

### Requirement 8: Favicon Placeholder

**User Story:** As a user, I want to see a colored initial-letter placeholder when a tab has no favicon, so that the tab list is visually informative even before favicons load.

#### Acceptance Criteria

1. WHEN a TabEntry has no favicon image available and `tabEntry.isLoading` is `false`, THE TabEntry SHALL display a FaviconPlaceholder: a 16×16 `Rectangle` with `radius` 3, background color `Theme.darkEmerald`, containing the first character of the tab title in white at `font.pixelSize` 10.
2. WHEN a TabEntry `tabEntry.isLoading` is `true`, THE TabEntry SHALL display the LoadingIndicator instead of the FaviconPlaceholder.
3. WHEN a TabEntry has a favicon image available, THE TabEntry SHALL display the favicon image instead of the FaviconPlaceholder.

---

### Requirement 9: Animated Loading Indicator

**User Story:** As a user, I want to see a smooth animated spinner instead of a static text character while a tab is loading, so that the loading state is clearly communicated.

#### Acceptance Criteria

1. WHEN `tabEntry.isLoading` is `true`, THE LoadingIndicator SHALL display a `Rectangle` arc or border-based ring styled in `Theme.darkEmerald` in place of the "⟳" text label.
2. THE LoadingIndicator SHALL rotate continuously using a `RotationAnimator` with a duration of 800ms per full rotation and `loops` set to `Animation.Infinite`.

---

### Requirement 10: New Tab Button Styling

**User Story:** As a user, I want the New Tab button to feel like a distinct interactive control, so that it is easy to find and clearly actionable.

#### Acceptance Criteria

1. THE NewTabButton SHALL have a height of 48px.
2. THE NewTabButton SHALL display a left border of 3px colored `Theme.darkEmerald`.
3. THE NewTabButton text color SHALL be `Theme.darkEmerald` in both light and dark mode.

---

### Requirement 11: Tab List and New Tab Button Divider

**User Story:** As a user, I want a visual divider between the tab list and the New Tab button, so that the two areas are clearly separated.

#### Acceptance Criteria

1. THE TabPanel SHALL display a 1px horizontal `Rectangle` divider between the bottom of the tab list scroll area and the NewTabButton, colored `Theme.lightBorder` in light mode and `Theme.darkBorder` in dark mode.

---

### Requirement 12: Hamburger Menu Hover State

**User Story:** As a user, I want the hamburger menu button to show a hover background consistent with the tab hover color, so that interactive elements feel visually unified.

#### Acceptance Criteria

1. WHEN the HamburgerMenu button is hovered and `darkMode` is `false`, THE HamburgerMenu background SHALL use `Theme.petalFrost`.
2. WHEN the HamburgerMenu button is hovered and `darkMode` is `true`, THE HamburgerMenu background SHALL use `Theme.mutedBalticBlue`.

---

### Requirement 13: Hamburger Menu Additional Items

**User Story:** As a user, I want "New Tab" and "Close Tab" actions in the hamburger menu, so that I can manage tabs without using the sidebar.

#### Acceptance Criteria

1. THE HamburgerMenu `Menu` SHALL contain a "New Tab" `MenuItem` that, when triggered, calls `tabManager.createTab("https://polli.page")`.
2. THE HamburgerMenu `Menu` SHALL contain a "Close Tab" `MenuItem` that, when triggered, calls `tabManager.closeTab(tabManager.activeTabId)`.
3. THE "New Tab" and "Close Tab" `MenuItem` backgrounds SHALL use `Theme.petalFrost` on hover in light mode and `Theme.mutedBalticBlue` on hover in dark mode.

---

### Requirement 14: Dark Mode Color Alignment

**User Story:** As a developer, I want the dark mode URL bar color aligned with shadowGrey and a muted dark hover color defined in the theme, so that dark mode surfaces are visually cohesive.

#### Acceptance Criteria

1. THE Theme SHALL define a `mutedBalticBlue` color property with value `#0f4070` (a desaturated, darker variant of balticBlue).
2. THE Theme `darkUrlBar` property SHALL be updated to `Theme.shadowGrey` (`#211a1d`) to align with the dark background surface.
3. THE Theme `darkHover` property SHALL reference `Theme.mutedBalticBlue` instead of `Theme.balticBlue`.
