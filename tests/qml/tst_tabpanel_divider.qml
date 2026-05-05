// Tests for TabPanel divider between tab list and New Tab button
// Validates: Requirement 11.1

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtTest 1.0
import LilypadTheme 1.0
import qml 1.0

TestCase {
    name: "TabPanelDivider"
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

    // Helper: find the divider Rectangle (height=1, not the button, inside the Column)
    // The Column in TabPanel has: ScrollView, divider Rectangle (h=1), Button
    function findDividerRect(item) {
        if (!item) return null
        // Look for a Column child
        for (var i = 0; i < item.children.length; i++) {
            var child = item.children[i]
            if (child.toString().indexOf("Column") >= 0) {
                // Inside the Column, find the Rectangle with height=1 that is not a button
                for (var j = 0; j < child.children.length; j++) {
                    var colChild = child.children[j]
                    if (colChild.toString().indexOf("Rectangle") >= 0 &&
                        colChild.hasOwnProperty("height") && colChild.height === 1) {
                        return colChild
                    }
                }
            }
        }
        return null
    }

    // Req 11.1: Divider Rectangle height === 1
    function test_divider_height() {
        var panel = tabPanelComponent.createObject(null, { darkMode: false })
        verify(panel !== null, "TabPanel should be created")
        var divider = findDividerRect(panel)
        verify(divider !== null, "Divider Rectangle (height=1) should exist in TabPanel Column")
        compare(divider.height, 1)
        panel.destroy()
    }

    // Req 11.1: Divider color === Theme.lightBorder in light mode
    function test_divider_color_light_mode() {
        var panel = tabPanelComponent.createObject(null, { darkMode: false })
        verify(panel !== null, "TabPanel should be created")
        var divider = findDividerRect(panel)
        verify(divider !== null, "Divider Rectangle should exist")
        compare(divider.color.toString().toLowerCase(),
                Theme.lightBorder.toString().toLowerCase())
        panel.destroy()
    }

    // Req 11.1: Divider color === Theme.darkBorder in dark mode
    function test_divider_color_dark_mode() {
        var panel = tabPanelComponent.createObject(null, { darkMode: true })
        verify(panel !== null, "TabPanel should be created")
        var divider = findDividerRect(panel)
        verify(divider !== null, "Divider Rectangle should exist")
        compare(divider.color.toString().toLowerCase(),
                Theme.darkBorder.toString().toLowerCase())
        panel.destroy()
    }
}
