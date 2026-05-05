import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import LilypadTheme

Rectangle {
    id: root

    property var tabEntry: null
    property bool isActive: false
    property bool darkMode: false

    signal tabClicked()
    signal tabClosed()

    height: 44
    color: {
        if (isActive) return darkMode ? Theme.balticBlue : Theme.petalFrost
        if (hoverHandler.hovered) return darkMode ? Theme.darkHover : Theme.lightHover
        return darkMode ? Theme.darkSurface : Theme.lightSurface
    }
    border.color: isActive ? (darkMode ? Theme.darkBorder : "#aaa") : "transparent"
    border.width: isActive ? 2 : 0

    Behavior on color { ColorAnimation { duration: 90 } }

    Rectangle {
        id: accentBar
        width: 3
        anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
        color: Theme.darkEmerald
        visible: isActive
    }

    HoverHandler {
        id: hoverHandler
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        Item {
            id: tabContent
            Layout.fillWidth: true
            Layout.fillHeight: true

            RowLayout {
                anchors.fill: parent
                spacing: 8

                // Favicon area — three mutually exclusive children
                Item {
                    id: faviconArea
                    Layout.preferredWidth: 16
                    Layout.preferredHeight: 16

                    // Sub-task 5.3: LoadingIndicator — visible when tabEntry.isLoading
                    Rectangle {
                        id: loadingIndicator
                        anchors.centerIn: parent
                        width: 14
                        height: 14
                        radius: 7
                        color: "transparent"
                        border.color: Theme.darkEmerald
                        border.width: 2
                        visible: tabEntry && tabEntry.isLoading

                        // Leave one quarter of the ring transparent to create arc effect
                        Rectangle {
                            width: 4
                            height: 4
                            color: "transparent"
                            anchors.top: parent.top
                            anchors.right: parent.right
                        }

                        RotationAnimator {
                            target: loadingIndicator
                            from: 0
                            to: 360
                            duration: 800
                            loops: Animation.Infinite
                            running: loadingIndicator.visible
                        }
                    }

                    // Sub-task 5.4: Favicon Image — visible when ready and not loading
                    Image {
                        id: faviconImage
                        anchors.centerIn: parent
                        width: 16
                        height: 16
                        source: (tabEntry && tabEntry.favicon) ? tabEntry.favicon : ""
                        visible: status === Image.Ready && !(tabEntry && tabEntry.isLoading)
                        fillMode: Image.PreserveAspectFit
                    }

                    // Sub-task 5.2: FaviconPlaceholder — visible when no favicon and not loading
                    Rectangle {
                        id: faviconPlaceholder
                        anchors.centerIn: parent
                        width: 16
                        height: 16
                        radius: 3
                        color: Theme.darkEmerald
                        visible: !(tabEntry && tabEntry.isLoading) && faviconImage.status !== Image.Ready

                        Text {
                            anchors.centerIn: parent
                            text: (tabEntry && tabEntry.title && tabEntry.title.length > 0)
                                  ? tabEntry.title[0].toUpperCase()
                                  : "?"
                            color: "white"
                            font.pixelSize: 10
                        }
                    }
                }

                // Title
                Label {
                    id: titleLabel
                    text: tabEntry ? (tabEntry.title || tabEntry.url || "") : ""
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    font.pixelSize: 14
                    color: darkMode ? Theme.darkText : Theme.lightText

                    Connections {
                        target: tabEntry
                        function onTitleChanged() {
                            titleLabel.text = tabEntry.title || tabEntry.url || ""
                        }
                        function onUrlChanged() {
                            if (!tabEntry.title) {
                                titleLabel.text = tabEntry.url || ""
                            }
                        }
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: root.tabClicked()
            }
        }

        // Close button
        Button {
            id: closeBtn
            text: "×"
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            flat: true
            visible: isActive || hoverHandler.hovered

            contentItem: Label {
                text: parent.text
                color: root.darkMode ? Theme.darkText : Theme.lightText
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                color: parent.hovered ? Theme.destructive : "transparent"
                radius: 2
            }

            onClicked: root.tabClosed()
        }
    }
}
