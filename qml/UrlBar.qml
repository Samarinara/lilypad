import QtQuick 2.15
import QtQuick.Controls 2.15
import LilypadTheme

TextField {
    id: urlBar
    property bool darkMode: false
    property bool expanded: false
    property string currentUrl: ""

    visible: expanded

    placeholderText: "Search or enter address"
    font.pixelSize: 14
    color: darkMode ? Theme.darkText : Theme.lightText
    placeholderTextColor: darkMode ? "#888" : "#999"
    padding: 8
    leftPadding: 28

    // Sync currentUrl into the text field whenever it changes from outside
    onCurrentUrlChanged: {
        text = currentUrl
    }

    background: Rectangle {
        color: urlBar.darkMode ? Theme.darkUrlBar : Theme.lightUrlBar
        border.color: urlBar.activeFocus ? Theme.focusBorder : (urlBar.darkMode ? Theme.darkBorder : "#ccc")
        border.width: 1
        radius: 6

        Behavior on border.color { ColorAnimation { duration: 80 } }
    }

    Label {
        id: securityIcon
        visible: urlBar.expanded
        text: urlBar.text.startsWith("https://") ? "🔒" : "🌐"
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 6
        font.pixelSize: 14
    }

    signal navigateRequested(string url)

    // Select all text when the field receives focus
    onActiveFocusChanged: if (activeFocus) selectAll()

    // Handle Enter key
    onAccepted: {
        if (text.length > 0) {
            var processed = urlProcessor.process(text)
            navigateRequested(processed.toString())
        }
    }

    // Clear focus on Escape
    Keys.onEscapePressed: {
        urlBar.focus = false
    }
}
