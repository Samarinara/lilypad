// ============================================================================
// TAB_ENTRY.CPP
// ============================================================================
// This file contains the implementation of TabEntry's methods:
// activate() and deactivate().
//
// These methods control whether the tab's web view is visible
// and active, or hidden in the background. This is important for browser
// performance — Chrome hides background tabs so they don't consume
// resources rendering content the user can't see.
// ============================================================================

// ============================================================================
// INCLUDE WHAT WE NEED
// ============================================================================
// We need the TabEntry header to get the class definition.
// That's all we need for these simple methods.
// ============================================================================
#include "tab_entry.h"
#include <QWebEngineView>

// ============================================================================
// METHOD: activate()
// ============================================================================
// Called when this tab becomes the active (visible) tab.
// We need to show the view so it renders and receives input.
// ============================================================================
void TabEntry::activate() {
    // Show the container widget so the web view becomes visible.
    if (container) {
        container->show();
    }
}

// ============================================================================
// METHOD: deactivate()
// ============================================================================
// Called when switching AWAY from this tab (another tab becomes active).
// We hide the view so it doesn't waste resources.
// ============================================================================
void TabEntry::deactivate() {
    // Hide the container widget to save resources.
    if (container) {
        container->hide();
    }
}
