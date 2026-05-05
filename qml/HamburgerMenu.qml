import QtQuick 2.15
import QtQuick.Controls 2.15
import LilypadTheme

Button {
    id: root
    property bool darkMode: false
    signal darkModeToggled(bool enabled)

    text: "☰"
    flat: true
    width: 40
    height: 40

    contentItem: Label {
        text: parent.text
        color: root.darkMode ? Theme.darkText : Theme.lightText
        font.pixelSize: 16
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        color: parent.hovered
            ? (root.darkMode ? Theme.mutedBalticBlue : Theme.petalFrost)
            : "transparent"
        radius: 2
    }

    onClicked: menu.open()

    Menu {
        id: menu
        x: root.width - width
        y: root.height

        MenuItem {
            text: "Dark Mode"
            checkable: true
            checked: root.darkMode
            onToggled: root.darkModeToggled(checked)

            contentItem: Label {
                text: parent.text
                color: root.darkMode ? Theme.darkText : Theme.lightText
            }

            background: Rectangle {
                color: parent.hovered
                    ? (root.darkMode ? Theme.mutedBalticBlue : Theme.petalFrost)
                    : "transparent"
            }
        }

        MenuItem {
            text: "New Tab"
            onTriggered: tabManager.createTab("https://polli.page")

            contentItem: Label {
                text: parent.text
                color: root.darkMode ? Theme.darkText : Theme.lightText
            }

            background: Rectangle {
                color: parent.hovered
                    ? (root.darkMode ? Theme.mutedBalticBlue : Theme.petalFrost)
                    : "transparent"
            }
        }

        MenuItem {
            text: "Close Tab"
            onTriggered: tabManager.closeTab(tabManager.activeTabId)

            contentItem: Label {
                text: parent.text
                color: root.darkMode ? Theme.darkText : Theme.lightText
            }

            background: Rectangle {
                color: parent.hovered
                    ? (root.darkMode ? Theme.mutedBalticBlue : Theme.petalFrost)
                    : "transparent"
            }
        }
    }
}
