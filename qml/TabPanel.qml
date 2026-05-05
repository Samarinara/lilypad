import QtQuick 2.15
import QtQuick.Controls 2.15
import LilypadTheme

Rectangle {
    id: root
    property bool darkMode: false

    color: darkMode ? Theme.darkBg : Theme.lightBg
    border.color: darkMode ? Theme.darkBorder : Theme.lightBorder
    border.width: 1

    Column {
        anchors.fill: parent
        spacing: 2

        // Scrollable tab list
        ScrollView {
            id: scrollView
            width: parent.width
            height: parent.height - newTabButton.height
            clip: true

            ListView {
                id: tabList
                model: tabManager
                anchors.fill: parent
                spacing: 2

                delegate: TabEntry {
                    width: tabList.width
                    tabEntry: model.tabEntry
                    isActive: model.tabId === tabManager.activeTabId
                    darkMode: root.darkMode

                    onTabClicked: {
                        tabManager.switchToTab(model.tabId)
                    }

                    onTabClosed: {
                        tabManager.closeTab(model.tabId)
                    }
                }
            }
        }

        // Divider between tab list and new tab button
        Rectangle {
            width: parent.width
            height: 1
            color: root.darkMode ? Theme.darkBorder : Theme.lightBorder
        }

        // New Tab button
        Button {
            id: newTabButton
            text: "+ New Tab"
            width: parent.width
            height: 48
            enabled: tabManager.tabCount() < 50

            contentItem: Label {
                text: parent.text
                color: Theme.darkEmerald
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                color: parent.hovered ? (root.darkMode ? Theme.darkHover : Theme.lightHover) : (root.darkMode ? Theme.darkBg : Theme.lightBg)
                border.color: root.darkMode ? Theme.darkBorder : Theme.lightBorder

                // 3px left accent border
                Rectangle {
                    width: 3
                    anchors {
                        left: parent.left
                        top: parent.top
                        bottom: parent.bottom
                    }
                    color: Theme.darkEmerald
                }
            }

            onClicked: {
                tabManager.createTab("https://polli.page")
            }
        }
    }
}
