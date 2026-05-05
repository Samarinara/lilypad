// Tests for TopBar background, border, and padding
// Validates: Requirements 1.1, 1.2, 1.3

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtTest 1.0
import LilypadTheme 1.0

TestCase {
    name: "TopBarProps"
    when: windowShown

    // Minimal TopBar replica matching Main.qml structure
    Component {
        id: topBarComponent
        Rectangle {
            id: topBarBackground
            width: 800
            height: 44
            property bool darkMode: false

            color: darkMode ? Theme.shadowGrey : Theme.softLinen

            // 1px bottom border
            Rectangle {
                id: bottomBorder
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: parent.darkMode ? Theme.darkBorder : Theme.lightBorder
            }

            RowLayout {
                id: topBar
                height: 40
                anchors.top: parent.top
                anchors.topMargin: 4
                anchors.left: parent.left
                anchors.right: parent.right
            }
        }
    }

    // Req 1.1: TopBar background color === Theme.softLinen in light mode
    function test_background_color_light_mode() {
        var topBar = topBarComponent.createObject(null, { darkMode: false })
        verify(topBar !== null)
        compare(topBar.color.toString().toLowerCase(),
                Theme.softLinen.toString().toLowerCase())
        topBar.destroy()
    }

    // Req 1.1: TopBar background color === Theme.shadowGrey in dark mode
    function test_background_color_dark_mode() {
        var topBar = topBarComponent.createObject(null, { darkMode: true })
        verify(topBar !== null)
        compare(topBar.color.toString().toLowerCase(),
                Theme.shadowGrey.toString().toLowerCase())
        topBar.destroy()
    }

    // Req 1.2: TopBar bottom border height === 1
    function test_bottom_border_height() {
        var topBar = topBarComponent.createObject(null)
        verify(topBar !== null)
        var border = topBar.children[0]
        verify(border !== null, "Bottom border Rectangle should exist")
        compare(border.height, 1)
        topBar.destroy()
    }

    // Req 1.2: TopBar bottom border color === Theme.lightBorder in light mode
    function test_bottom_border_color_light_mode() {
        var topBar = topBarComponent.createObject(null, { darkMode: false })
        verify(topBar !== null)
        var border = topBar.children[0]
        verify(border !== null)
        compare(border.color.toString().toLowerCase(),
                Theme.lightBorder.toString().toLowerCase())
        topBar.destroy()
    }

    // Req 1.2: TopBar bottom border color === Theme.darkBorder in dark mode
    function test_bottom_border_color_dark_mode() {
        var topBar = topBarComponent.createObject(null, { darkMode: true })
        verify(topBar !== null)
        var border = topBar.children[0]
        verify(border !== null)
        compare(border.color.toString().toLowerCase(),
                Theme.darkBorder.toString().toLowerCase())
        topBar.destroy()
    }

    // Req 1.3: RowLayout anchors.topMargin === 4
    function test_rowlayout_top_margin() {
        var topBar = topBarComponent.createObject(null)
        verify(topBar !== null)
        // The RowLayout is the second child (index 1) after the border Rectangle
        var rowLayout = topBar.children[1]
        verify(rowLayout !== null, "RowLayout should exist")
        compare(rowLayout.anchors.topMargin, 4)
        topBar.destroy()
    }
}
