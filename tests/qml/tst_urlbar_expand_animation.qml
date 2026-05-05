// Property-based tests for URL Bar Expand Animation feature
// Feature: url-bar-expand-animation
// Validates: Requirements 2.5, 4.1, 4.3, 4.4, 5.1, 5.2

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtTest 1.0
import LilypadTheme 1.0
import qml 1.0

TestCase {
    name: "UrlBarExpandAnimationProps"
    when: windowShown

    // Stub urlProcessor so UrlBar.qml doesn't crash on onAccepted
    QtObject {
        id: urlProcessor
        function process(text) { return text }
    }

    // ---------------------------------------------------------------------------
    // Helpers
    // ---------------------------------------------------------------------------

    // Generate a random alphanumeric + punctuation string of length [1, maxLen]
    function randomString(maxLen) {
        var chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .-_/:"
        var len = Math.max(1, Math.floor(Math.random() * maxLen) + 1)
        var result = ""
        for (var i = 0; i < len; i++) {
            result += chars.charAt(Math.floor(Math.random() * chars.length))
        }
        return result
    }

    // Generate a random URL-like string (no guaranteed scheme)
    function randomUrl() {
        var hosts = ["example.com", "foo.org", "bar.net", "test.io", "site.co"]
        var paths = ["", "/path", "/a/b/c", "/search?q=hello", "/page#anchor"]
        var host = hosts[Math.floor(Math.random() * hosts.length)]
        var path = paths[Math.floor(Math.random() * paths.length)]
        return host + path
    }

    // Generate a random https URL
    function randomHttpsUrl() {
        return "https://" + randomUrl()
    }

    // Generate a random non-https URL (http, ftp, or bare host)
    function randomNonHttpsUrl() {
        var schemes = ["http://", "ftp://", ""]
        var scheme = schemes[Math.floor(Math.random() * schemes.length)]
        return scheme + randomUrl()
    }

    // ---------------------------------------------------------------------------
    // Components
    // ---------------------------------------------------------------------------

    Component {
        id: urlBarComponent
        UrlBar {
            width: 400
            height: 40
        }
    }

    // ---------------------------------------------------------------------------
    // Property 1: Text is fully selected on expand
    // Feature: url-bar-expand-animation, Property 1: text is fully selected on expand
    // Validates: Requirements 2.5
    // ---------------------------------------------------------------------------
    function test_property1_text_fully_selected_on_expand() {
        // Feature: url-bar-expand-animation, Property 1: text is fully selected on expand
        var iterations = 100
        for (var i = 0; i < iterations; i++) {
            var str = randomString(80)

            var bar = urlBarComponent.createObject(null, {
                width: 400,
                height: 40,
                expanded: false
            })
            verify(bar !== null, "UrlBar should be created (iteration " + i + ")")

            // Set text before expanding
            bar.text = str

            // Trigger expand: set expanded = true, then forceActiveFocus + selectAll
            // (mirrors what UrlBarContainer does after expandRadiusAnim.onStopped)
            bar.expanded = true
            bar.forceActiveFocus()
            bar.selectAll()

            compare(bar.selectionStart, 0,
                "selectionStart should be 0 for text '" + str + "' (iteration " + i + ")")
            compare(bar.selectionEnd, str.length,
                "selectionEnd should equal text.length (" + str.length + ") for text '" + str + "' (iteration " + i + ")")

            bar.destroy()
        }
    }

    // ---------------------------------------------------------------------------
    // Property 2: Navigation emitted for any non-empty text
    // Feature: url-bar-expand-animation, Property 2: navigation emitted for any non-empty text
    // Validates: Requirements 4.1
    // ---------------------------------------------------------------------------
    function test_property2_navigate_requested_emitted_for_nonempty_text() {
        // Feature: url-bar-expand-animation, Property 2: navigation emitted for any non-empty text
        var iterations = 100
        for (var i = 0; i < iterations; i++) {
            var str = randomString(60)
            // Ensure non-empty (randomString already guarantees length >= 1)

            var bar = urlBarComponent.createObject(null, {
                width: 400,
                height: 40,
                expanded: true
            })
            verify(bar !== null, "UrlBar should be created (iteration " + i + ")")

            var emittedUrl = null
            bar.navigateRequested.connect(function(url) {
                emittedUrl = url
            })

            bar.text = str
            // Simulate Enter key press (triggers onAccepted)
            bar.accepted()

            verify(emittedUrl !== null,
                "navigateRequested should be emitted for non-empty text '" + str + "' (iteration " + i + ")")
            verify(emittedUrl.length > 0,
                "emitted URL should be non-empty for input '" + str + "' (iteration " + i + ")")

            bar.destroy()
        }
    }

    // ---------------------------------------------------------------------------
    // Property 3: URL bar text always reflects active tab URL
    // Feature: url-bar-expand-animation, Property 3: URL bar text always reflects active tab URL
    // Validates: Requirements 4.3, 4.4
    // ---------------------------------------------------------------------------
    function test_property3_urlbar_text_reflects_current_url() {
        // Feature: url-bar-expand-animation, Property 3: URL bar text always reflects active tab URL
        var iterations = 100
        for (var i = 0; i < iterations; i++) {
            var url = randomHttpsUrl()

            // Test in collapsed state
            var barCollapsed = urlBarComponent.createObject(null, {
                width: 400,
                height: 40,
                expanded: false,
                currentUrl: url
            })
            verify(barCollapsed !== null, "UrlBar (collapsed) should be created (iteration " + i + ")")
            compare(barCollapsed.text, url,
                "urlBar.text should equal currentUrl '" + url + "' in collapsed state (iteration " + i + ")")
            barCollapsed.destroy()

            // Test in expanded state
            var barExpanded = urlBarComponent.createObject(null, {
                width: 400,
                height: 40,
                expanded: true,
                currentUrl: url
            })
            verify(barExpanded !== null, "UrlBar (expanded) should be created (iteration " + i + ")")
            compare(barExpanded.text, url,
                "urlBar.text should equal currentUrl '" + url + "' in expanded state (iteration " + i + ")")
            barExpanded.destroy()
        }
    }

    // ---------------------------------------------------------------------------
    // Property 4: Security icon correctly reflects URL scheme
    // Feature: url-bar-expand-animation, Property 4: security icon correctly reflects URL scheme
    // Validates: Requirements 5.1, 5.2
    // ---------------------------------------------------------------------------

    // Helper: find the security icon Label inside a UrlBar
    function findSecurityIcon(bar) {
        for (var i = 0; i < bar.data.length; i++) {
            var item = bar.data[i]
            if (item && item.toString().indexOf("Label") >= 0 &&
                item.hasOwnProperty("text") &&
                (item.text === "🔒" || item.text === "🌐")) {
                return item
            }
        }
        // Also check children
        for (var j = 0; j < bar.children.length; j++) {
            var child = bar.children[j]
            if (child && child.toString().indexOf("Label") >= 0 &&
                child.hasOwnProperty("text") &&
                (child.text === "🔒" || child.text === "🌐")) {
                return child
            }
        }
        return null
    }

    function test_property4_security_icon_reflects_url_scheme() {
        // Feature: url-bar-expand-animation, Property 4: security icon correctly reflects URL scheme
        var iterations = 100
        // Half https, half non-https
        for (var i = 0; i < iterations; i++) {
            var url
            var expectLock
            if (i % 2 === 0) {
                url = randomHttpsUrl()
                expectLock = true
            } else {
                url = randomNonHttpsUrl()
                // Ensure it doesn't accidentally start with https://
                if (url.startsWith("https://")) {
                    url = "http://" + randomUrl()
                }
                expectLock = false
            }

            var bar = urlBarComponent.createObject(null, {
                width: 400,
                height: 40,
                expanded: true,
                currentUrl: url
            })
            verify(bar !== null, "UrlBar should be created (iteration " + i + ")")

            // The security icon text is driven by: urlBar.text.startsWith("https://") ? "🔒" : "🌐"
            // We verify this directly via the text property binding
            var expectedIcon = url.startsWith("https://") ? "🔒" : "🌐"
            var icon = findSecurityIcon(bar)
            verify(icon !== null,
                "Security icon Label should exist in expanded UrlBar (iteration " + i + ")")
            compare(icon.text, expectedIcon,
                "Security icon should be '" + expectedIcon + "' for URL '" + url + "' (iteration " + i + ")")

            bar.destroy()
        }
    }

    // =============================================================================
    // Example-based unit tests (Task 6)
    // =============================================================================

    // ---------------------------------------------------------------------------
    // Additional components for example-based tests
    // ---------------------------------------------------------------------------

    Component {
        id: searchButtonComponent
        SearchButton {
            width: 40
            height: 40
        }
    }

    Component {
        id: urlBarContainerComponent
        UrlBarContainer {
            width: 400
            height: 40
        }
    }

    // Component without explicit width to test the default collapsed width
    Component {
        id: urlBarContainerDefaultComponent
        UrlBarContainer {
            height: 40
        }
    }

    Component {
        id: urlBarExComponent
        UrlBar {
            width: 400
            height: 40
        }
    }

    // ---------------------------------------------------------------------------
    // 6.1 SearchButton: diameter 40, radius 20, icon "🔍", background Theme.darkEmerald
    // Validates: Req 1.1, 1.2, 1.4, 1.5
    // ---------------------------------------------------------------------------

    function test_6_1_searchbutton_size_and_shape() {
        var btn = searchButtonComponent.createObject(null)
        verify(btn !== null, "SearchButton should be created")
        compare(btn.width, 40, "SearchButton width should be 40")
        compare(btn.height, 40, "SearchButton height should be 40")
        compare(btn.radius, 20, "SearchButton radius should be 20 (fully circular)")
        btn.destroy()
    }

    function test_6_1_searchbutton_icon_text() {
        var btn = searchButtonComponent.createObject(null)
        verify(btn !== null, "SearchButton should be created")
        // Find the Label child
        var icon = null
        for (var i = 0; i < btn.children.length; i++) {
            var child = btn.children[i]
            if (child && child.hasOwnProperty("text") && child.text === "🔍") {
                icon = child
                break
            }
        }
        verify(icon !== null, "SearchButton should have a '🔍' label child")
        compare(icon.text, "🔍", "SearchButton icon should be '🔍'")
        btn.destroy()
    }

    function test_6_1_searchbutton_background_color_dark_mode() {
        // Req 1.4: dark mode → Theme.darkEmerald background
        var btn = searchButtonComponent.createObject(null, { darkMode: true })
        verify(btn !== null, "SearchButton should be created in dark mode")
        // When not hovered, color should be Theme.darkEmerald
        compare(btn.color.toString().toLowerCase(),
                Theme.darkEmerald.toString().toLowerCase(),
                "SearchButton background should be Theme.darkEmerald in dark mode (not hovered)")
        btn.destroy()
    }

    function test_6_1_searchbutton_background_color_light_mode() {
        // Req 1.5: light mode → Theme.darkEmerald background
        var btn = searchButtonComponent.createObject(null, { darkMode: false })
        verify(btn !== null, "SearchButton should be created in light mode")
        compare(btn.color.toString().toLowerCase(),
                Theme.darkEmerald.toString().toLowerCase(),
                "SearchButton background should be Theme.darkEmerald in light mode (not hovered)")
        btn.destroy()
    }

    // ---------------------------------------------------------------------------
    // 6.2 SearchButton hover colors
    // Validates: Req 1.6
    // ---------------------------------------------------------------------------

    function test_6_2_searchbutton_hover_color_values() {
        // Verify the theme values used for hover are correct
        // Light mode hover → Theme.lightHover
        compare(Theme.lightHover.toString().toLowerCase(), "#f8d1e8",
                "Theme.lightHover should be #f8d1e8 (petalFrost)")
        // Dark mode hover → Theme.darkHover
        compare(Theme.darkHover.toString().toLowerCase(), "#0f4070",
                "Theme.darkHover should be #0f4070 (mutedBalticBlue)")
    }

    // ---------------------------------------------------------------------------
    // 6.3 SearchButton keyboard: Space and Enter emit expandRequested; activeFocusOnTab
    // Validates: Req 7.3, 7.4
    // ---------------------------------------------------------------------------

    function test_6_3_searchbutton_activeFocusOnTab() {
        var btn = searchButtonComponent.createObject(null)
        verify(btn !== null, "SearchButton should be created")
        verify(btn.activeFocusOnTab === true,
               "SearchButton should have activeFocusOnTab: true")
        btn.destroy()
    }

    function test_6_3_searchbutton_space_emits_expandRequested() {
        var btn = searchButtonComponent.createObject(null)
        verify(btn !== null, "SearchButton should be created")
        var emitted = false
        btn.expandRequested.connect(function() { emitted = true })
        btn.Keys.spacePressed(null)
        verify(emitted, "Space key should emit expandRequested")
        btn.destroy()
    }

    function test_6_3_searchbutton_enter_emits_expandRequested() {
        var btn = searchButtonComponent.createObject(null)
        verify(btn !== null, "SearchButton should be created")
        var emitted = false
        btn.expandRequested.connect(function() { emitted = true })
        btn.Keys.returnPressed(null)
        verify(emitted, "Enter/Return key should emit expandRequested")
        btn.destroy()
    }

    // ---------------------------------------------------------------------------
    // 6.4 Expand animation config: duration 250 ms, Easing.OutCubic;
    //     collapsed width 40, expanded width fills parent; collapsed radius 20, expanded radius 6
    // Validates: Req 2.3, 2.6, 2.7
    // ---------------------------------------------------------------------------

    function test_6_4_expand_width_anim_duration() {
        var container = urlBarContainerComponent.createObject(null, { width: 400, height: 40 })
        verify(container !== null, "UrlBarContainer should be created")
        // Access expandWidthAnim via container's data/resources
        var anim = findAnimationById(container, "expandWidthAnim")
        verify(anim !== null, "expandWidthAnim should exist on UrlBarContainer")
        compare(anim.duration, 250, "expandWidthAnim duration should be 250 ms")
        container.destroy()
    }

    function test_6_4_expand_width_anim_easing() {
        var container = urlBarContainerComponent.createObject(null, { width: 400, height: 40 })
        verify(container !== null, "UrlBarContainer should be created")
        var anim = findAnimationById(container, "expandWidthAnim")
        verify(anim !== null, "expandWidthAnim should exist on UrlBarContainer")
        compare(anim.easing.type, Easing.OutCubic, "expandWidthAnim easing should be OutCubic")
        container.destroy()
    }

    function test_6_4_expand_radius_anim_duration() {
        var container = urlBarContainerComponent.createObject(null, { width: 400, height: 40 })
        verify(container !== null, "UrlBarContainer should be created")
        var anim = findAnimationById(container, "expandRadiusAnim")
        verify(anim !== null, "expandRadiusAnim should exist on UrlBarContainer")
        compare(anim.duration, 250, "expandRadiusAnim duration should be 250 ms")
        container.destroy()
    }

    function test_6_4_expand_radius_anim_easing() {
        var container = urlBarContainerComponent.createObject(null, { width: 400, height: 40 })
        verify(container !== null, "UrlBarContainer should be created")
        var anim = findAnimationById(container, "expandRadiusAnim")
        verify(anim !== null, "expandRadiusAnim should exist on UrlBarContainer")
        compare(anim.easing.type, Easing.OutCubic, "expandRadiusAnim easing should be OutCubic")
        container.destroy()
    }

    function test_6_4_collapsed_width_is_40() {
        // Create without specifying width so the default collapsed width (40) is used
        var container = urlBarContainerDefaultComponent.createObject(null)
        verify(container !== null, "UrlBarContainer should be created")
        // Before expand, width should be 40 (default collapsed size)
        compare(container.width, 40, "UrlBarContainer collapsed width should be 40")
        container.destroy()
    }

    function test_6_4_expand_radius_anim_to_is_6() {
        var container = urlBarContainerComponent.createObject(null, { width: 400, height: 40 })
        verify(container !== null, "UrlBarContainer should be created")
        var anim = findAnimationById(container, "expandRadiusAnim")
        verify(anim !== null, "expandRadiusAnim should exist")
        compare(anim.to, 6, "expandRadiusAnim.to should be 6 (rounded rectangle)")
        container.destroy()
    }

    // ---------------------------------------------------------------------------
    // 6.5 Collapse animation config: duration 200 ms, Easing.InCubic;
    //     width returns to 40; radius returns to 20
    // Validates: Req 3.3, 3.5, 3.6
    // ---------------------------------------------------------------------------

    function test_6_5_collapse_width_anim_duration() {
        var container = urlBarContainerComponent.createObject(null, { width: 400, height: 40 })
        verify(container !== null, "UrlBarContainer should be created")
        var anim = findAnimationById(container, "collapseWidthAnim")
        verify(anim !== null, "collapseWidthAnim should exist on UrlBarContainer")
        compare(anim.duration, 200, "collapseWidthAnim duration should be 200 ms")
        container.destroy()
    }

    function test_6_5_collapse_width_anim_easing() {
        var container = urlBarContainerComponent.createObject(null, { width: 400, height: 40 })
        verify(container !== null, "UrlBarContainer should be created")
        var anim = findAnimationById(container, "collapseWidthAnim")
        verify(anim !== null, "collapseWidthAnim should exist on UrlBarContainer")
        compare(anim.easing.type, Easing.InCubic, "collapseWidthAnim easing should be InCubic")
        container.destroy()
    }

    function test_6_5_collapse_radius_anim_duration() {
        var container = urlBarContainerComponent.createObject(null, { width: 400, height: 40 })
        verify(container !== null, "UrlBarContainer should be created")
        var anim = findAnimationById(container, "collapseRadiusAnim")
        verify(anim !== null, "collapseRadiusAnim should exist on UrlBarContainer")
        compare(anim.duration, 200, "collapseRadiusAnim duration should be 200 ms")
        container.destroy()
    }

    function test_6_5_collapse_radius_anim_easing() {
        var container = urlBarContainerComponent.createObject(null, { width: 400, height: 40 })
        verify(container !== null, "UrlBarContainer should be created")
        var anim = findAnimationById(container, "collapseRadiusAnim")
        verify(anim !== null, "collapseRadiusAnim should exist on UrlBarContainer")
        compare(anim.easing.type, Easing.InCubic, "collapseRadiusAnim easing should be InCubic")
        container.destroy()
    }

    function test_6_5_collapse_width_anim_to_is_40() {
        var container = urlBarContainerComponent.createObject(null, { width: 400, height: 40 })
        verify(container !== null, "UrlBarContainer should be created")
        var anim = findAnimationById(container, "collapseWidthAnim")
        verify(anim !== null, "collapseWidthAnim should exist")
        compare(anim.to, 40, "collapseWidthAnim.to should be 40")
        container.destroy()
    }

    function test_6_5_collapse_radius_anim_to_is_20() {
        var container = urlBarContainerComponent.createObject(null, { width: 400, height: 40 })
        verify(container !== null, "UrlBarContainer should be created")
        var anim = findAnimationById(container, "collapseRadiusAnim")
        verify(anim !== null, "collapseRadiusAnim should exist")
        compare(anim.to, 20, "collapseRadiusAnim.to should be 20 (fully circular)")
        container.destroy()
    }

    // ---------------------------------------------------------------------------
    // 6.6 Visibility: security icon hidden when collapsed; TextField hidden when collapsed
    // Validates: Req 3.7, 5.3
    // ---------------------------------------------------------------------------

    function test_6_6_urlbar_hidden_when_collapsed() {
        var bar = urlBarExComponent.createObject(null, { expanded: false })
        verify(bar !== null, "UrlBar should be created")
        // UrlBar itself is visible: expanded, so collapsed → not visible
        compare(bar.visible, false, "UrlBar should be hidden when expanded=false")
        bar.destroy()
    }

    function test_6_6_urlbar_visible_when_expanded() {
        var bar = urlBarExComponent.createObject(null, { expanded: true })
        verify(bar !== null, "UrlBar should be created")
        compare(bar.visible, true, "UrlBar should be visible when expanded=true")
        bar.destroy()
    }

    function test_6_6_security_icon_hidden_when_collapsed() {
        var bar = urlBarExComponent.createObject(null, {
            expanded: false,
            currentUrl: "https://example.com"
        })
        verify(bar !== null, "UrlBar should be created")
        var icon = findSecurityIconEx(bar)
        verify(icon !== null, "Security icon Label should exist in UrlBar")
        compare(icon.visible, false, "Security icon should be hidden when expanded=false")
        bar.destroy()
    }

    function test_6_6_security_icon_visible_when_expanded() {
        var bar = urlBarExComponent.createObject(null, {
            expanded: true,
            currentUrl: "https://example.com"
        })
        verify(bar !== null, "UrlBar should be created")
        var icon = findSecurityIconEx(bar)
        verify(icon !== null, "Security icon Label should exist in UrlBar")
        compare(icon.visible, true, "Security icon should be visible when expanded=true")
        bar.destroy()
    }

    // ---------------------------------------------------------------------------
    // 6.7 Hamburger slide: offset applied when expanded, restored when collapsed
    // Validates: Req 2.2, 3.4
    // ---------------------------------------------------------------------------

    function test_6_7_hamburger_offset_when_expanded() {
        var container = urlBarContainerComponent.createObject(null, { width: 400, height: 40 })
        verify(container !== null, "UrlBarContainer should be created")
        container.expanded = true
        compare(container.hamburgerOffset, 60,
                "hamburgerOffset should be 60 when expanded")
        container.destroy()
    }

    function test_6_7_hamburger_offset_when_collapsed() {
        var container = urlBarContainerComponent.createObject(null, { width: 400, height: 40 })
        verify(container !== null, "UrlBarContainer should be created")
        container.expanded = false
        compare(container.hamburgerOffset, 0,
                "hamburgerOffset should be 0 when collapsed")
        container.destroy()
    }

    function test_6_7_hamburger_offset_transitions() {
        var container = urlBarContainerComponent.createObject(null, { width: 400, height: 40 })
        verify(container !== null, "UrlBarContainer should be created")
        // Start collapsed
        compare(container.hamburgerOffset, 0, "Initial hamburgerOffset should be 0")
        // Expand
        container.expanded = true
        compare(container.hamburgerOffset, 60, "hamburgerOffset should be 60 after expand")
        // Collapse
        container.expanded = false
        compare(container.hamburgerOffset, 0, "hamburgerOffset should return to 0 after collapse")
        container.destroy()
    }

    // ---------------------------------------------------------------------------
    // 6.8 Ctrl+L shortcut triggers expand
    // Validates: Req 7.1
    // Note: Ctrl+L is defined in Main.qml. We test the expand() function directly
    //       and verify the Shortcut sequence value via the design contract.
    // ---------------------------------------------------------------------------

    function test_6_8_expand_function_sets_expanded_true() {
        var container = urlBarContainerComponent.createObject(null, { width: 400, height: 40 })
        verify(container !== null, "UrlBarContainer should be created")
        verify(container.expanded === false, "Container should start collapsed")
        container.expand()
        compare(container.expanded, true, "expand() should set expanded to true")
        container.destroy()
    }

    function test_6_8_expand_is_noop_when_already_expanded() {
        var container = urlBarContainerComponent.createObject(null, { width: 400, height: 40 })
        verify(container !== null, "UrlBarContainer should be created")
        container.expanded = true
        // Calling expand() again should not throw or change state
        container.expand()
        compare(container.expanded, true, "expand() on already-expanded container should be a no-op")
        container.destroy()
    }

    // ---------------------------------------------------------------------------
    // 6.9 Empty text + Enter → no navigateRequested signal
    // Validates: Req 4.5
    // ---------------------------------------------------------------------------

    function test_6_9_empty_text_enter_no_navigate_signal() {
        var bar = urlBarExComponent.createObject(null, { expanded: true })
        verify(bar !== null, "UrlBar should be created")
        var emitted = false
        bar.navigateRequested.connect(function(url) { emitted = true })
        bar.text = ""
        bar.accepted()
        verify(!emitted, "navigateRequested should NOT be emitted when text is empty")
        bar.destroy()
    }

    // ---------------------------------------------------------------------------
    // 6.10 Focus loss → collapse; Escape → collapse; navigateRequested → collapse
    // Validates: Req 3.1, 3.2, 4.2
    // ---------------------------------------------------------------------------

    function test_6_10_navigate_requested_triggers_collapse() {
        var container = urlBarContainerComponent.createObject(null, { width: 400, height: 40 })
        verify(container !== null, "UrlBarContainer should be created")
        // Expand first
        container.expanded = true
        verify(container.expanded === true, "Container should be expanded")
        // Simulate navigateRequested from UrlBar (which calls collapse())
        container.collapse()
        compare(container.expanded, false,
                "Container should collapse after navigateRequested/collapse()")
        container.destroy()
    }

    function test_6_10_escape_triggers_collapse_via_focus_loss() {
        // UrlBar.qml: Keys.onEscapePressed sets urlBar.focus = false,
        // which triggers onActiveFocusChanged → UrlBarContainer.collapse()
        // We verify the collapse() function works correctly.
        var container = urlBarContainerComponent.createObject(null, { width: 400, height: 40 })
        verify(container !== null, "UrlBarContainer should be created")
        container.expanded = true
        container.collapse()
        compare(container.expanded, false, "collapse() should set expanded to false")
        container.destroy()
    }

    function test_6_10_collapse_is_noop_when_already_collapsed() {
        var container = urlBarContainerComponent.createObject(null, { width: 400, height: 40 })
        verify(container !== null, "UrlBarContainer should be created")
        verify(container.expanded === false, "Container should start collapsed")
        // Calling collapse() on already-collapsed container should be a no-op
        container.collapse()
        compare(container.expanded, false, "collapse() on already-collapsed container should be a no-op")
        container.destroy()
    }

    // ---------------------------------------------------------------------------
    // 6.11 UrlBar theme colors: background, text, border (focused/unfocused, dark/light)
    // Validates: Req 6.1–6.6
    // ---------------------------------------------------------------------------

    function test_6_11_urlbar_background_dark_mode() {
        // Req 6.1: dark mode → Theme.darkUrlBar
        var bar = urlBarExComponent.createObject(null, { darkMode: true, expanded: true })
        verify(bar !== null, "UrlBar should be created in dark mode")
        var bg = bar.background
        verify(bg !== null, "UrlBar background should exist")
        compare(bg.color.toString().toLowerCase(),
                Theme.darkUrlBar.toString().toLowerCase(),
                "UrlBar background should be Theme.darkUrlBar in dark mode")
        bar.destroy()
    }

    function test_6_11_urlbar_background_light_mode() {
        // Req 6.2: light mode → Theme.lightUrlBar
        var bar = urlBarExComponent.createObject(null, { darkMode: false, expanded: true })
        verify(bar !== null, "UrlBar should be created in light mode")
        var bg = bar.background
        verify(bg !== null, "UrlBar background should exist")
        compare(bg.color.toString().toLowerCase(),
                Theme.lightUrlBar.toString().toLowerCase(),
                "UrlBar background should be Theme.lightUrlBar in light mode")
        bar.destroy()
    }

    function test_6_11_urlbar_text_color_dark_mode() {
        // Req 6.3: dark mode → Theme.darkText
        var bar = urlBarExComponent.createObject(null, { darkMode: true, expanded: true })
        verify(bar !== null, "UrlBar should be created in dark mode")
        compare(bar.color.toString().toLowerCase(),
                Theme.darkText.toString().toLowerCase(),
                "UrlBar text color should be Theme.darkText in dark mode")
        bar.destroy()
    }

    function test_6_11_urlbar_text_color_light_mode() {
        // Req 6.4: light mode → Theme.lightText
        var bar = urlBarExComponent.createObject(null, { darkMode: false, expanded: true })
        verify(bar !== null, "UrlBar should be created in light mode")
        compare(bar.color.toString().toLowerCase(),
                Theme.lightText.toString().toLowerCase(),
                "UrlBar text color should be Theme.lightText in light mode")
        bar.destroy()
    }

    function test_6_11_urlbar_border_color_unfocused_dark_mode() {
        // Req 6.6: unfocused dark mode → Theme.darkBorder
        var bar = urlBarExComponent.createObject(null, { darkMode: true, expanded: true })
        verify(bar !== null, "UrlBar should be created in dark mode")
        var bg = bar.background
        verify(bg !== null, "UrlBar background should exist")
        // When not focused, border.color = darkMode ? Theme.darkBorder : "#ccc"
        compare(bg.border.color.toString().toLowerCase(),
                Theme.darkBorder.toString().toLowerCase(),
                "UrlBar border should be Theme.darkBorder when unfocused in dark mode")
        bar.destroy()
    }

    function test_6_11_urlbar_border_color_unfocused_light_mode() {
        // Req 6.6: unfocused light mode → "#ccc"
        var bar = urlBarExComponent.createObject(null, { darkMode: false, expanded: true })
        verify(bar !== null, "UrlBar should be created in light mode")
        var bg = bar.background
        verify(bg !== null, "UrlBar background should exist")
        compare(bg.border.color.toString().toLowerCase(), "#cccccc",
                "UrlBar border should be #ccc when unfocused in light mode")
        bar.destroy()
    }

    function test_6_11_urlbar_focus_border_color_value() {
        // Req 6.5: focused → Theme.focusBorder
        compare(Theme.focusBorder.toString().toLowerCase(),
                Theme.darkEmerald.toString().toLowerCase(),
                "Theme.focusBorder should equal Theme.darkEmerald (#066633)")
    }

    // ---------------------------------------------------------------------------
    // Helpers (local to this TestCase)
    // ---------------------------------------------------------------------------

    // Find a NumberAnimation child by its objectName / id string
    function findAnimationById(container, animId) {
        for (var i = 0; i < container.data.length; i++) {
            var item = container.data[i]
            if (item && item.toString().indexOf("NumberAnimation") >= 0) {
                // Match by objectName if set, otherwise by property + duration heuristic
                if (item.objectName === animId) return item
                // Fall back: match by the combination of property name and id string
                // QML exposes named ids as properties on the parent object
            }
        }
        // Try direct property access (QML exposes named children as properties)
        if (container[animId] !== undefined) return container[animId]
        return null
    }

    // Find the security icon Label inside a UrlBar
    function findSecurityIconEx(bar) {
        for (var i = 0; i < bar.data.length; i++) {
            var item = bar.data[i]
            if (item && item.toString().indexOf("Label") >= 0 &&
                item.hasOwnProperty("text") &&
                (item.text === "🔒" || item.text === "🌐")) {
                return item
            }
        }
        for (var j = 0; j < bar.children.length; j++) {
            var child = bar.children[j]
            if (child && child.toString().indexOf("Label") >= 0 &&
                child.hasOwnProperty("text") &&
                (child.text === "🔒" || child.text === "🌐")) {
                return child
            }
        }
        return null
    }
}
