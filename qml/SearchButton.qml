import QtQuick 2.15
import QtQuick.Controls 2.15
import LilypadTheme

Rectangle {
    id: searchButton

    property bool darkMode: false

    signal expandRequested()

    width: 40
    height: 40
    radius: 20
    color: hoverHandler.containsMouse
        ? (darkMode ? Theme.darkHover : Theme.lightHover)
        : Theme.darkEmerald

    activeFocusOnTab: true

    Behavior on color { ColorAnimation { duration: 80 } }

    Label {
        anchors.centerIn: parent
        text: "🔍"
        font.pixelSize: 18
    }

    HoverHandler {
        id: hoverHandler
    }

    TapHandler {
        onTapped: searchButton.expandRequested()
    }

    Keys.onSpacePressed: searchButton.expandRequested()
    Keys.onReturnPressed: searchButton.expandRequested()

    // Keyboard focus indicator
    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: "transparent"
        border.color: Theme.focusBorder
        border.width: searchButton.activeFocus ? 2 : 0
    }
}
