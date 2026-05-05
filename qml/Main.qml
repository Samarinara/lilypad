import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtWebEngine
import LilypadTheme

ApplicationWindow {
    id: window
    title: "Lilypad"
    width: 1200
    height: 800
    visible: true

    property bool darkMode: false

    // Function to navigate active tab
    function navigateToUrl(url) {
        console.log("Navigating to: " + url)
        tabManager.loadUrlForActiveTab(url)
    }

    // Ctrl+L shortcut: expand the URL bar when it is collapsed (Req 7.1)
    Shortcut {
        sequence: "Ctrl+L"
        onActivated: {
            if (!urlBarContainer.expanded) {
                urlBarContainer.expand()
            }
        }
    }

    // Top bar with URL bar and hamburger menu
    Rectangle {
        id: topBarBackground
        height: 44
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        color: window.darkMode ? Theme.shadowGrey : Theme.softLinen

        // 1px bottom border separating the top bar from the content area
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: window.darkMode ? Theme.darkBorder : Theme.lightBorder
        }

        RowLayout {
            id: topBar
            height: 40
            anchors.top: parent.top
            anchors.topMargin: 4
            anchors.left: parent.left
            anchors.right: parent.right

            // 4.1: UrlBarContainer replaces the old UrlBar item
            UrlBarContainer {
                id: urlBarContainer
                Layout.fillWidth: true
                darkMode: window.darkMode
                // 4.4: currentUrl is propagated from the active tab (set imperatively below)

                // 4.5: Connect navigateRequested to navigateToUrl()
                onNavigateRequested: (url) => {
                    navigateToUrl(url)
                }
            }

            HamburgerMenu {
                id: hamburgerMenu
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                darkMode: window.darkMode

                // 4.2: Slide the hamburger right when the URL bar expands, back on collapse.
                // hamburgerOffset is exposed by UrlBarContainer (positive when expanded, 0 when collapsed).
                // We bind Layout.leftMargin to the offset so the item shifts right within the RowLayout.
                Layout.leftMargin: urlBarContainer.hamburgerOffset

                // Animate the left-margin change:
                //   expand  → 250 ms OutCubic (matches Expand_Animation)
                //   collapse → 200 ms InCubic  (matches Collapse_Animation)
                Behavior on Layout.leftMargin {
                    NumberAnimation {
                        duration: urlBarContainer.expanded ? 250 : 200
                        easing.type: urlBarContainer.expanded ? Easing.OutCubic : Easing.InCubic
                    }
                }

                onDarkModeToggled: (enabled) => {
                    window.darkMode = enabled
                }
            }
        }
    }

    // Main content area
    Row {
        anchors.top: topBarBackground.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        // Tab Panel (sidebar)
        TabPanel {
            id: tabPanel
            width: 220
            height: parent.height
            darkMode: window.darkMode
        }

        // Web content area
        Item {
            width: parent.width - tabPanel.width
            height: parent.height

            // Repeater for WebEngineViews - only active tab's Loader is active
            Repeater {
                id: webViewRepeater
                model: tabManager

                delegate: Item {
                    id: delegateItem
                    width: parent.width
                    height: parent.height
                    visible: model.tabId === tabManager.activeTabId

                    // Track the tab's URL for URL bar sync when switching tabs
                    property string tabUrl: model.url

                    Loader {
                        id: webLoader
                        anchors.fill: parent
                        // Only instantiate the WebEngineView for the active tab,
                        // preventing Chromium renderer processes for background tabs
                        active: model.tabId === tabManager.activeTabId

                        sourceComponent: Component {
                            WebEngineView {
                                anchors.fill: parent
                                url: model.url

                                // Connect signals to TabManager for tab data updates
                                onTitleChanged: {
                                    tabManager.onTitleChanged(model.tabId, title)
                                }

                                onIconChanged: {
                                    tabManager.onFaviconChanged(model.tabId, icon)
                                }

                                onLoadingChanged: {
                                    tabManager.onLoadingStateChanged(model.tabId, loading)
                                }

                                // 4.7: Update urlBarContainer.currentUrl instead of urlBar.text
                                onUrlChanged: {
                                    // Only update TabManager if URL actually differs from model
                                    if (url !== model.url) {
                                        tabManager.onUrlChanged(model.tabId, url)
                                    }
                                    if (model.tabId === tabManager.activeTabId) {
                                        urlBarContainer.currentUrl = url
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Update URL bar when switching tabs — handles both the case where
            // the WebEngineView is already loaded (re-activation) and where it
            // hasn't been loaded yet (first activation, URL comes from the model)
            Connections {
                target: tabManager
                // 4.6: Set urlBarContainer.currentUrl instead of urlBar.text
                function onActiveTabChanged(oldTabId, newTabId) {
                    var activeTab = tabManager.activeTab()
                    if (activeTab) {
                        urlBarContainer.currentUrl = activeTab.url
                    }
                }
            }
        }
    }

    // Initialize with first tab
    Component.onCompleted: {
        if (tabManager.tabCount() === 0) {
            tabManager.createTab("https://polli.page")
        }
    }
}
