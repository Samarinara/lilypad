// ============================================================================
// TAB_ENTRY_WIDGET.H
// ============================================================================
// This file defines TabEntryWidget — the visual representation of ONE tab
// shown in the sidebar (TabPanel). Think of it as the "tab card" you see
// in Chrome's tab strip: it shows the favicon, title, and has a close button.
//
// Each TabEntryWidget displays:
// - Favicon (small website icon)
// - Title or URL (the text shown)
// - Current state (active/inactive, loading/idle)
// - Close button (the X to close the tab)
//
// When clicked, it emits a signal to switch to that tab.
// When close button is clicked, it emits a signal to close that tab.
// ============================================================================

#ifndef LILYPAD_TAB_ENTRY_WIDGET_H
#define LILYPAD_TAB_ENTRY_WIDGET_H

// ============================================================================
// QT INCLUDES
// ============================================================================
// Qt framework:
// - QFrame: Base widget class (we're a framed widget)
// - QPixmap: For favicon images
// ============================================================================
#include <QFrame>   // Base widget with frame border support
#include <QPixmap>  // Image for favicons

// ============================================================================
// QT FORWARD DECLARATIONS
// ============================================================================
// We need these classes but not full definitions:
// ============================================================================
class QLabel;        // Text/image display widget
class QPushButton;  // Clickable button
class QMovie;       // Animated image (loading spinner)
class QHBoxLayout;  // Horizontal layout

// ============================================================================
// CLASS: TabEntryWidget
// ============================================================================
// A widget that represents one tab in the sidebar.
//
// We inherit from QFrame because we want:
// - A frame (border) around each tab
// - Fixed height for consistency
// - Mouse event handling (click to switch tab)
//
// ANALOGY: Think of each tab in Chrome's tab bar.
// When you hover, the tab highlights. When you click,
// the tab becomes active. When you click X, the tab closes.
// We replicate this behavior here.
// ============================================================================
class TabEntryWidget : public QFrame {
    Q_OBJECT  // Enable Qt meta-object system

public:
    // ============================================================================
    // CONSTRUCTOR
    // ============================================================================
    // Create a widget for the given tab.
    //
    // Parameters:
    //   tabId  - ID of the tab this widget represents
    //   parent - Parent widget (for cleanup hierarchy)
    // ============================================================================
    explicit TabEntryWidget(int tabId, QWidget* parent = nullptr);

    // ============================================================================
    // DESTRUCTOR
    // ============================================================================
    // Clean up. Defaulted because we have no special cleanup.
    // ============================================================================
    ~TabEntryWidget();

    // ============================================================================
    // GET TAB ID
    // ============================================================================
    // Get the tab ID this widget represents.
    // ============================================================================
    int tabId() const { return m_tabId; }

    // ============================================================================
    // SETTERS
    // ============================================================================
    // These methods update the display based on tab state.
    // ============================================================================

    // Set whether this tab is active (currently visible).
    void setActive(bool active);

    // Set the page title to display.
    void setTitle(const QString& title);

    // Set the favicon image.
    void setFavicon(const QPixmap& pixmap);

    // Set whether the page is loading (show spinner if true).
    void setLoading(bool loading);

    // Set the URL to display if title is empty.
    // In browsers, if a page has no title, we show the URL.
    void setUrl(const QString& url);

    // ============================================================================
    // SIGNALS
    // ============================================================================
    // Qt signals emitted when user interacts with this widget.
    // ============================================================================

    // Emitted when the widget is clicked (to switch to this tab).
    // Parameter: tabId of the clicked tab.
    signals:
        void clicked(int tabId);

    // Emitted when the close button is clicked.
    void closeClicked(int tabId);

protected:
    // ============================================================================
    // MOUSE EVENT HANDLERS
    // ============================================================================
    // These are called when mouse events happen on this widget.
    // We override them to emit signals.
    // ============================================================================

    // Called when mouse is pressed on the widget.
    // We emit "clicked" to switch to this tab.
    void mousePressEvent(QMouseEvent* event) override;

    // Called when mouse enters the widget (hover started).
    // We show hover highlight.
    void enterEvent(QEnterEvent* event) override;

    // Called when mouse leaves the widget (hover ended).
    // We remove hover highlight.
    void leaveEvent(QEvent* event) override;

private:
    // ============================================================================
    // PRIVATE DATA MEMBERS
    // ============================================================================

    // The tab ID this widget represents.
    int m_tabId;

    // Current URL (used as fallback when title is empty).
    QString m_url;

    // Current title from the page.
    QString m_title;

    // Is this the active tab?
    bool m_active = false;

    // Is the page currently loading?
    bool m_loading = false;

    // ============================================================================
    // VISUAL COMPONENTS
    // ============================================================================
    // The Qt widgets that make up our visual appearance:

    // Favicon display (small icon label).
    QLabel* m_faviconLabel;

    // Title/text display label.
    QLabel* m_titleLabel;

    // Close button (the X).
    QPushButton* m_closeBtn;

    // Animation for loading spinner (nullptr when not loading).
    QMovie* m_loadingSpinner = nullptr;

    // Cached favicon pixmap.
    // We store this so we can restore it after loading ends.
    // The spinner replaces the favicon during loading.
    QPixmap m_faviconPixmap;

    // ============================================================================
    // PRIVATE HELPER METHODS
    // ============================================================================

    // Update the title label text (handle elision/truncation).
    void updateTitleDisplay();

    // Update the background color based on active/hover state.
    void updateBackground();
};

#endif // LILYPAD_TAB_ENTRY_WIDGET_H