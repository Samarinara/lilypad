import QtQuick 2.15
import QtQuick.Controls 2.15
import LilypadTheme

Item {
    id: urlBarContainer

    // Public API
    property bool darkMode: false
    property bool expanded: false
    property string currentUrl: ""

    // Hamburger offset: parent binds HamburgerMenu.x to this value.
    // When expanded, returns a positive offset to push the hamburger off-screen.
    // When collapsed, returns 0 (hamburger in its natural position).
    property real hamburgerOffset: expanded ? 60 : 0

    signal navigateRequested(string url)

    // Default collapsed size
    width: 40
    height: 40

    // Animated background rectangle (handles radius animation)
    Rectangle {
        id: background
        anchors.fill: parent
        radius: 20
        color: "transparent"
    }

    SearchButton {
        id: searchButton
        anchors.centerIn: parent
        darkMode: urlBarContainer.darkMode
        visible: !urlBarContainer.expanded

        onExpandRequested: urlBarContainer.expand()
    }

    UrlBar {
        id: urlBar
        anchors.fill: parent
        darkMode: urlBarContainer.darkMode
        expanded: urlBarContainer.expanded
        currentUrl: urlBarContainer.currentUrl

        onNavigateRequested: (url) => {
            urlBarContainer.navigateRequested(url)
            urlBarContainer.collapse()
        }

        onActiveFocusChanged: {
            if (!activeFocus && urlBarContainer.expanded) {
                urlBarContainer.collapse()
            }
        }
    }

    // Expand animation: width 40 → parent.width, radius 20 → 6, 250 ms OutCubic
    NumberAnimation {
        id: expandWidthAnim
        objectName: "expandWidthAnim"
        target: urlBarContainer
        property: "width"
        to: urlBarContainer.parent ? urlBarContainer.parent.width : urlBarContainer.width
        duration: 250
        easing.type: Easing.OutCubic
    }

    NumberAnimation {
        id: expandRadiusAnim
        objectName: "expandRadiusAnim"
        target: background
        property: "radius"
        to: 6
        duration: 250
        easing.type: Easing.OutCubic

        onStopped: {
            if (urlBarContainer.expanded) {
                urlBar.forceActiveFocus()
                urlBar.selectAll()
            }
        }
    }

    // Collapse animation: width → 40, radius 6 → 20, 200 ms InCubic
    NumberAnimation {
        id: collapseWidthAnim
        objectName: "collapseWidthAnim"
        target: urlBarContainer
        property: "width"
        to: 40
        duration: 200
        easing.type: Easing.InCubic
    }

    NumberAnimation {
        id: collapseRadiusAnim
        objectName: "collapseRadiusAnim"
        target: background
        property: "radius"
        to: 20
        duration: 200
        easing.type: Easing.InCubic
    }

    // hamburgerOffset is a plain computed property — no Behavior here.
    // Main.qml adds its own Behavior on the HamburgerMenu position so it can
    // use the correct expand (250 ms OutCubic) vs collapse (200 ms InCubic) timing.

    function expand() {
        if (urlBarContainer.expanded) return

        urlBarContainer.expanded = true

        // Stop any running collapse animations
        collapseWidthAnim.stop()
        collapseRadiusAnim.stop()

        // Set animation start values from current state
        expandWidthAnim.from = urlBarContainer.width
        expandRadiusAnim.from = background.radius

        expandWidthAnim.start()
        expandRadiusAnim.start()
    }

    function collapse() {
        if (!urlBarContainer.expanded) return

        urlBarContainer.expanded = false

        // Stop any running expand animations
        expandWidthAnim.stop()
        expandRadiusAnim.stop()

        // Set animation start values from current state
        collapseWidthAnim.from = urlBarContainer.width
        collapseRadiusAnim.from = background.radius

        collapseWidthAnim.start()
        collapseRadiusAnim.start()
    }
}
