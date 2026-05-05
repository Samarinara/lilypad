# Requirements Document

## Introduction

This feature replaces the always-visible, full-width URL bar in the Lilypad browser's top bar with a compact circular search button. When the user clicks the button, it expands with a fluid animation into a full-width search/URL input field, while the other top-bar items (the hamburger menu) slide out of the way. When the input loses focus (or the user presses Escape), the bar animates back to the compact button state. All navigation and URL-sync behaviour of the existing `UrlBar` component is preserved.

## Glossary

- **Search_Button**: The compact circular button displaying a search icon that replaces the full-width URL bar in its collapsed state.
- **Url_Bar**: The expanded text input field used to enter URLs or search queries, equivalent in function to the existing `UrlBar` component.
- **Top_Bar**: The horizontal bar at the top of the application window (`topBarBackground` / `topBar` in `Main.qml`) that contains the Search_Button / Url_Bar and the Hamburger_Menu.
- **Hamburger_Menu**: The `HamburgerMenu` component located at the right end of the Top_Bar.
- **Expand_Animation**: The animated transition from the collapsed Search_Button state to the expanded Url_Bar state.
- **Collapse_Animation**: The animated transition from the expanded Url_Bar state back to the collapsed Search_Button state.
- **Active_Tab**: The currently visible browser tab whose URL is reflected in the Url_Bar.

---

## Requirements

### Requirement 1: Collapsed State — Search Button

**User Story:** As a user, I want to see a compact circular search button in the top bar when I am not actively entering a URL, so that the top bar feels uncluttered and gives more visual space to the page content.

#### Acceptance Criteria

1. THE Search_Button SHALL be displayed as a circle with a diameter of 40 px in the Top_Bar when the Url_Bar is not expanded.
2. THE Search_Button SHALL display a search icon (🔍) centred within the button.
3. THE Search_Button SHALL be positioned in the center of the Top_Bar, occupying the same horizontal region as the collapsed Url_Bar.
4. WHILE the application is in dark mode, THE Search_Button SHALL use `Theme.darkEmerald` as its background colour.
5. WHILE the application is in light mode, THE Search_Button SHALL use `Theme.darkEmerald` as its background colour.
6. WHEN the user hovers over the Search_Button, THE Search_Button SHALL change its background colour to the theme hover colour (`Theme.darkHover` in dark mode, `Theme.lightHover` in light mode).

---

### Requirement 2: Expand Animation — Button to Search Bar

**User Story:** As a user, I want clicking the search button to smoothly expand it into a full-width URL bar, so that the transition feels polished and intentional.

#### Acceptance Criteria

1. WHEN the user clicks the Search_Button, THE Url_Bar SHALL expand from the Search_Button's circular shape to fill the available width of the Top_Bar.
2. WHEN the Expand_Animation begins, THE Hamburger_Menu SHALL slide out of the visible Top_Bar area toward the right edge of the window.
3. THE Expand_Animation SHALL complete within 250 ms using an easing curve of `Easing.OutCubic`.
4. WHEN the Expand_Animation completes, THE Url_Bar SHALL receive keyboard focus automatically.
5. WHEN the Expand_Animation completes, THE Url_Bar SHALL select all existing text so the user can immediately type a new URL.
6. THE Expand_Animation SHALL animate the width of the Url_Bar from 40 px to the full available width of the Top_Bar.
7. THE Expand_Animation SHALL animate the border-radius of the Url_Bar background from 20 px (fully circular) to 6 px (rounded rectangle).

---

### Requirement 3: Collapse Animation — Search Bar Back to Button

**User Story:** As a user, I want the URL bar to smoothly collapse back to the search button when I am done entering a URL, so that the top bar returns to its uncluttered state without a jarring jump.

#### Acceptance Criteria

1. WHEN the Url_Bar loses keyboard focus, THE Url_Bar SHALL begin the Collapse_Animation.
2. WHEN the user presses the Escape key while the Url_Bar has focus, THE Url_Bar SHALL release keyboard focus, triggering the Collapse_Animation.
3. THE Collapse_Animation SHALL complete within 200 ms using an easing curve of `Easing.InCubic`.
4. WHEN the Collapse_Animation begins, THE Hamburger_Menu SHALL slide back into its original position in the Top_Bar.
5. THE Collapse_Animation SHALL animate the width of the Url_Bar from its expanded width back to 40 px.
6. THE Collapse_Animation SHALL animate the border-radius of the Url_Bar background from 6 px back to 20 px.
7. WHEN the Collapse_Animation completes, THE Search_Button icon SHALL become visible and the text input SHALL be hidden.

---

### Requirement 4: Navigation Behaviour During Expanded State

**User Story:** As a user, I want the expanded URL bar to behave identically to the original URL bar for navigation, so that I do not lose any browsing functionality.

#### Acceptance Criteria

1. WHEN the user presses the Enter key while the Url_Bar is expanded and contains text, THE Url_Bar SHALL emit a `navigateRequested` signal with the processed URL.
2. WHEN a `navigateRequested` signal is emitted, THE Top_Bar SHALL trigger navigation in the Active_Tab and begin the Collapse_Animation.
3. WHEN the Active_Tab URL changes, THE Url_Bar SHALL update its displayed text to reflect the new URL regardless of whether the Url_Bar is expanded or collapsed.
4. WHEN the user switches to a different Active_Tab, THE Url_Bar SHALL update its displayed text to the URL of the newly active tab.
5. IF the Url_Bar contains no text when the user presses Enter, THEN THE Url_Bar SHALL take no navigation action.

---

### Requirement 5: Security Icon

**User Story:** As a user, I want to see a security indicator in the expanded URL bar, so that I can tell at a glance whether the current page uses HTTPS.

#### Acceptance Criteria

1. WHEN the Url_Bar is expanded and the displayed URL begins with `https://`, THE Url_Bar SHALL display a lock icon (🔒) at the left side of the input field.
2. WHEN the Url_Bar is expanded and the displayed URL does not begin with `https://`, THE Url_Bar SHALL display a globe icon (🌐) at the left side of the input field.
3. WHEN the Url_Bar is in the collapsed (Search_Button) state, THE Url_Bar SHALL not display the security icon.

---

### Requirement 6: Theme Consistency

**User Story:** As a user, I want the search button and expanded URL bar to match the application's light and dark themes, so that the UI looks cohesive in both modes.

#### Acceptance Criteria

1. WHILE the application is in dark mode, THE Url_Bar SHALL use `Theme.darkUrlBar` as its background colour when expanded.
2. WHILE the application is in light mode, THE Url_Bar SHALL use `Theme.lightUrlBar` as its background colour when expanded.
3. WHILE the application is in dark mode, THE Url_Bar SHALL use `Theme.darkText` as its text colour.
4. WHILE the application is in light mode, THE Url_Bar SHALL use `Theme.lightText` as its text colour.
5. WHEN the Url_Bar has keyboard focus, THE Url_Bar SHALL display a border with colour `Theme.focusBorder`.
6. WHEN the Url_Bar does not have keyboard focus, THE Url_Bar SHALL display a border with colour `Theme.darkBorder` in dark mode and `#ccc` in light mode.

---

### Requirement 7: Accessibility and Keyboard Navigation

**User Story:** As a user, I want to be able to open the URL bar using the keyboard, so that I can navigate without reaching for the mouse.

#### Acceptance Criteria

1. WHEN the user presses a configurable keyboard shortcut (default: `Ctrl+L`) while the Url_Bar is collapsed, THE Search_Button SHALL trigger the Expand_Animation and give focus to the Url_Bar.
2. WHEN the Url_Bar is expanded and the user presses Escape, THE Url_Bar SHALL release focus and trigger the Collapse_Animation.
3. THE Search_Button SHALL be reachable via Tab key focus traversal when the Url_Bar is collapsed.
4. WHEN the Search_Button has keyboard focus and the user presses Space or Enter, THE Search_Button SHALL trigger the Expand_Animation.
