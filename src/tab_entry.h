// ============================================================================
// TAB_ENTRY.H
// ============================================================================
// This file defines the TabEntry structure — a simple data container that holds
// all the information for ONE browser tab in our Lilypad browser.
//
// Think of TabEntry like the "tab" concept in Chrome: each tab has a title,
// URL (web address), favicon (little website icon), and a reference to the actual
// browser showing that web page.
// ============================================================================

#ifndef LILYPAD_TAB_ENTRY_H
#define LILYPAD_TAB_ENTRY_H

// ============================================================================
// QT INCLUDES
// ============================================================================
// Qt is a cross-platform GUI framework. It handles windows, widgets, layouts,
// signals/slots, and more. Lilypad uses Qt for its UI.
//
// These includes give us QString (text strings) and QPixmap (images).
// ============================================================================

#include <QString>   // Qt's string class — like std::string but with Unicode support
#include <QPixmap>   // Qt's image class — for favicons (little website icons)
#include <QWebEngineView>

// ============================================================================
// STRUCT: TabEntry
// ============================================================================
// A struct in C++ is essentially a class where members are public by default.
// We use struct here because TabEntry is just a data container — it doesn't
// have complex behavior, just data and two simple methods.
//
// In a web browser, each tab needs to track:
// - Unique ID (tabId)
// - Page title shown in tab
// - Current URL (web address)
// - Favicon (small icon from website)
// - Loading state (spinning wheel while page loads)
// - View (the QWebEngineView showing the web page)
// ============================================================================
struct TabEntry {
    // ------------------------------------------------------------------------
    // MEMBER VARIABLES
    // ------------------------------------------------------------------------
    // These are the "fields" or "properties" of each tab.
    // ------------------------------------------------------------------------

    // Unique identifier for this tab. Assigned by TabManager when created.
    // Like Chrome's tab ID — used to find and operate on specific tabs.
    // Starts at -1 (invalid) until properly assigned.
    int tabId = -1;

    // The page title, displayed in the tab bar. This typically comes from
    // the web page's <title> HTML tag. Example: "Google" for google.com
    QString title;

    // The current URL (web address) being displayed in this tab.
    // Examples: "https://google.com", "https://youtube.com/watch?v=..."
    QString url;

    // The favicon — a small image (usually 16x16 or 32x32 pixels) that
    // represents the website. Loaded from the website's /favicon.ico
    // or specified in the page's <link rel="icon">.
    QPixmap favicon;

    // Whether the page is currently loading. When true, we show a
    // spinning animation in the tab to indicate activity.
    // This is the same "loading spinner" you see in browser tabs.
    bool isLoading = false;

    // ============================================================================
    // QWebEngineView: THE BROWSER VIEW
    // ============================================================================
    // This is the Qt WebEngine view that displays the web page.
    // QWebEngineView is a QWidget that renders web content using Chromium.
    //
    // Unlike CEF which required separate handler classes and manual embedding,
    // QWebEngineView is a native Qt widget that can be directly added to layouts.
    // ============================================================================
    QWebEngineView* view = nullptr;

    // ============================================================================
    // Qt CONTAINER WIDGET
    // ============================================================================
    // The Qt widget that contains the web view.
    // This allows us to show/hide the tab in the layout.
    // ============================================================================
    QWidget* container = nullptr;

    // ------------------------------------------------------------------------
    // METHODS
    // ------------------------------------------------------------------------
    // Two simple methods to activate/deactivate this tab.
    // Called when switching between tabs.
    // ------------------------------------------------------------------------

    // Mark this tab as active (visible, receiving input).
    // When a tab becomes active, we show its container.
    void activate();

    // Mark this tab as inactive (hidden, not receiving input).
    // When switching away from a tab, we hide its container.
    void deactivate();
};
#endif // LILYPAD_TAB_ENTRY_H
