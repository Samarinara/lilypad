// Tests for NewTabButton height, left border, and text color
// Validates: Requirements 10.1, 10.2, 10.3

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtTest 1.0
import LilypadTheme 1.0
import qml 1.0

TestCase {
    name: "NewTabButton"
    when: windowShown

    // Mock tabManager so TabPanel.qml doesn't crash
    QtObject {
        id: tabManager
        property int activeTabId: 1
        function createTab(url) { return 1 }
        function closeTab(id) {}
        function tabCount() { return 1 }
        function switchToTab(id) {}
    }

    Component {
        id: tabPanelComponent
        TabPanel {
            width: 220
            height: 400
        }
    }

    // Helper: find Button with id "newTabButton" by walking children
    function findNewTabButton(item) {
        if (!item) return null
        if (item.toString().indexOf("Button") >= 0 &&
            item.hasOwnProperty("text") && item.text === "+ New Tab") {
            return item
        }
        for (var i = 0; i < item.children.length; i++) {
            var result = findNewTabButton(item.children[i])
            if (result) return result
        }
        return null
    }

    // Helper: find the left-border Rectangle inside the button background
    function findLeftBorderRect(buttonBackground) {
        if (!buttonBackground) return null
        for (var i = 0; i < buttonBackground.children.length; i++) {
            var child = buttonBackground.children[i]
            if (child.toString().indexOf("Rectangle") >= 0 &&
                child.hasOwnProperty("width") && child.width === 3) {
                return child
            }
        }
        return null
    }

    // Req 10.1: newTabButton height === 48
    function test_newtab_button_height() {
        var panel = tabPanelComponent.createObject(null, { darkMode: false })
        verify(panel !== null, "TabPanel should be created")
        var btn = findNewTabButton(panel)
        verify(btn !== null, "newTabButton should exist in TabPanel")
        compare(btn.height, 48)
        panel.destroy()
    }

    // Req 10.2: Left border Rectangle width === 3
    function test_left_border_width() {
        var panel = tabPanelComponent.createObject(null, { darkMode: false })
        verify(panel !== null, "TabPanel should be created")
        var btn = findNewTabButton(panel)
        verify(btn !== null, "newTabButton should exist")
        var bg = btn.background
        verify(bg !== null, "newTabButton background should exist")
        var leftBorder = findLeftBorderRect(bg)
        verify(leftBorder !== null, "Left border Rectangle (width=3) should exist in button background")
        compare(leftBorder.width, 3)
        panel.destroy()
    }

    // Req 10.2: Left border Rectangle color === Theme.darkEmerald
    function test_left_border_color() {
        var panel = tabPanelComponent.createObject(null, { darkMode: false })
        verify(panel !== null, "TabPanel should be created")
        var btn = findNewTabButton(panel)
        verify(btn !== null, "newTabButton should exist")
        var bg = btn.background
        verify(bg !== null, "newTabButton background should exist")
        var leftBorder = findLeftBorderRect(bg)
        verify(leftBorder !== null, "Left border Rectangle should exist")
        compare(leftBorder.color.toString().toLowerCase(),
                Theme.darkEmerald.toString().toLowerCase())
        panel.destroy()
    }

    // Req 10.3: newTabButton contentItem text color === Theme.darkEmerald in light mode
    function test_text_color_light_mode() {
        var panel = tabPanelComponent.createObject(null, { darkMode: false })
        verify(panel !== null, "TabPanel should be created")
        var btn = findNewTabButton(panel)
        verify(btn !== null, "newTabButton should exist")
        var contentLabel = btn.contentItem
        verify(contentLabel !== null, "newTabButton contentItem should exist")
        compare(contentLabel.color.toString().toLowerCase(),
                Theme.darkEmerald.toString().toLowerCase())
        panel.destroy()
    }

    // Req 10.3: newTabButton contentItem text color === Theme.darkEmerald in dark mode
    function test_text_color_dark_mode() {
        var panel = tabPanelComponent.createObject(null, { darkMode: true })
        verify(panel !== null, "TabPanel should be created")
        var btn = findNewTabButton(panel)
        verify(btn !== null, "newTabButton should exist")
        var contentLabel = btn.contentItem
        verify(contentLabel !== null, "newTabButton contentItem should exist")
        compare(contentLabel.color.toString().toLowerCase(),
                Theme.darkEmerald.toString().toLowerCase())
        panel.destroy()
    }
}
