// ============================================================================
// TAB_ENTRY_WIDGET.CPP
// ============================================================================
// This file contains the implementation of TabEntryWidget — the visual
// component for a single tab in the sidebar. It handles layouts,
// displaying the favicon/title/loading state, and mouse interactions.
// ============================================================================

// ============================================================================
// INCLUDES
// ============================================================================
// Qt widgets and utilities:
// ============================================================================
#include "tab_entry_widget.h"

#include <QHBoxLayout>   // Horizontal layout for tab contents
#include <QLabel>       // Text/image display widget
#include <QMouseEvent>  // Mouse events
#include <QMovie>      // Animated loading spinner
#include <QPushButton> // Close button
#include <QFontMetrics> // For text truncation (elision)
#include <QPainter>   // For custom painting (if needed)
#include <QSizePolicy> // Widget size policy

// ============================================================================
// CONSTRUCTOR
// ============================================================================
// Build the tab widget with all its components:
// favicon, title, and close button.
// ============================================================================
TabEntryWidget::TabEntryWidget(int tabId, QWidget* parent)
    : QFrame(parent)
    , m_tabId(tabId)
{
    // ============================================================================
    // BASIC SETUP
    // ============================================================================
    // Fixed height ensures all tabs look consistent.
    // No frame (border) keeps it clean.
    // Pointing hand cursor indicates clickable.
    setFixedHeight(36);
    setFrameShape(QFrame::NoFrame);
    setCursor(Qt::PointingHandCursor);

    // ============================================================================
    // LAYOUT
    // ============================================================================
    // Create horizontal layout: [favicon] [title] [close button]
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);  // Small padding
    layout->setSpacing(4);                // Space between items

    // ============================================================================
    // FAVICON LABEL
    // ============================================================================
    // The tiny icon representing the website.
    // Fixed 16x16 size like Chrome's favicons.
    m_faviconLabel = new QLabel(this);
    m_faviconLabel->setFixedSize(16, 16);
    m_faviconLabel->setScaledContents(true);

    // ============================================================================
    // TITLE LABEL
    // ============================================================================
    // The text showing the title or URL.
    // Expands to fill available space.
    m_titleLabel = new QLabel(this);
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_titleLabel->setTextFormat(Qt::PlainText);

    // ============================================================================
    // CLOSE BUTTON
    // ============================================================================
    // The X button to close this tab.
    // Small, flat (no border), arrow cursor.
    m_closeBtn = new QPushButton(QStringLiteral("×"), this);
    m_closeBtn->setFixedSize(16, 16);
    m_closeBtn->setFlat(true);
    m_closeBtn->setCursor(Qt::ArrowCursor);

    // When clicked, emit signal to close the tab
    connect(m_closeBtn, &QPushButton::clicked, this, [this]() {
        emit closeClicked(m_tabId);
    });

    // ============================================================================
    // ADD TO LAYOUT
    // ============================================================================
    // Add widgets in order: favicon, title (expands), close button
    layout->addWidget(m_faviconLabel);
    layout->addWidget(m_titleLabel, 1);  // Stretch factor 1 = expand
    layout->addWidget(m_closeBtn);

    setLayout(layout);

    // ============================================================================
    // DEFAULT FAVICON
    // ============================================================================
    // Show a placeholder until real favicon loads.
    // Gray square as placeholder.
    QPixmap placeholder(16, 16);
    placeholder.fill(Qt::gray);
    m_faviconLabel->setPixmap(placeholder);
}

// ============================================================================
// DESTRUCTOR
// ============================================================================
// Default destructor - Qt handles child widget cleanup.
// ============================================================================
TabEntryWidget::~TabEntryWidget() = default;

// ============================================================================
// METHOD: setActive
// ============================================================================
// Set whether this tab is the active (visible) tab.
// We update the background color accordingly.
// ============================================================================
void TabEntryWidget::setActive(bool active)
{
    m_active = active;
    updateBackground();
}

// ============================================================================
// METHOD: setTitle
// ============================================================================
// Set the page title to display.
// ============================================================================
void TabEntryWidget::setTitle(const QString& title)
{
    m_title = title;
    updateTitleDisplay();
}

// ============================================================================
// METHOD: setUrl
// ============================================================================
// Set the URL (used as fallback when title is empty).
// ============================================================================
void TabEntryWidget::setUrl(const QString& url)
{
    m_url = url;
    updateTitleDisplay();
}

// ============================================================================
// METHOD: updateTitleDisplay
// ============================================================================
// Update the title label with proper text and truncation.
//
// We show the title if available, otherwise the URL.
// If text is too long, we elide (truncate with "...").
// ============================================================================
void TabEntryWidget::updateTitleDisplay()
{
    // Use title unless empty, then use URL
    const QString& text = m_title.isEmpty() ? m_url : m_title;

    // ============================================================================
    // TEXT ELISION (TRUNCATION)
    // ============================================================================
    // If title is too long, we don't want it to push other
    // widgets around. We truncate and add "..." at the end.
    //
    // QFontMetrics measures text width, so we can truncate.
    QFontMetrics fm(m_titleLabel->font());

    // Get available width
    int availableWidth = m_titleLabel->width();
    if (availableWidth <= 0) {
        // Widget not laid out yet - use reasonable default
        availableWidth = 140;
    }

    // Elide the text if needed (Qt::ElideRight = add "..." at end)
    const QString elided = fm.elidedText(text, Qt::ElideRight, availableWidth);
    m_titleLabel->setText(elided);
}

// ============================================================================
// METHOD: setFavicon
// ============================================================================
// Set the favicon image.
//
// We cache the pixmap so we can restore it after loading ends.
// If currently loading, we don't replace the spinner.
// ============================================================================
void TabEntryWidget::setFavicon(const QPixmap& pixmap)
{
    // Always cache what we receive
    m_faviconPixmap = pixmap;

    // Don't replace spinner while loading
    if (m_loading) {
        return;
    }

    // Update display
    if (pixmap.isNull()) {
        // Show placeholder gray square
        QPixmap placeholder(16, 16);
        placeholder.fill(Qt::gray);
        m_faviconLabel->setPixmap(placeholder);
    } else {
        // Scale to fit and display
        m_faviconLabel->setPixmap(pixmap.scaled(16, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    }
    m_faviconLabel->setText(QString());
}

// ============================================================================
// METHOD: setLoading
// ============================================================================
// Set whether the page is currently loading.
// When loading, show spinner. When done, restore favicon.
// ============================================================================
void TabEntryWidget::setLoading(bool loading)
{
    m_loading = loading;

    if (loading) {
        // Show a simple spinner: the Unicode character "⟳" (rotating arrow)
        m_faviconLabel->setPixmap(QPixmap());
        m_faviconLabel->setText(QStringLiteral("⟳"));
    } else {
        // Restore favicon (or placeholder if none)
        m_faviconLabel->setText(QString());
        if (m_faviconPixmap.isNull()) {
            QPixmap placeholder(16, 16);
            placeholder.fill(Qt::gray);
            m_faviconLabel->setPixmap(placeholder);
        } else {
            m_faviconLabel->setPixmap(
                m_faviconPixmap.scaled(16, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        }
    }
}

// ============================================================================
// METHOD: updateBackground
// ============================================================================
// Update the background color based on state:
// - Active: highlight color (like selected tab in Chrome)
// - Not active, hover: semi-transparent highlight
// - Not active, not hover: no background
// ============================================================================
void TabEntryWidget::updateBackground()
{
    if (m_active) {
        // Active tab gets the highlight color
        QPalette pal = palette();
        pal.setColor(QPalette::Window, palette().color(QPalette::Highlight));
        setPalette(pal);
        setAutoFillBackground(true);
    } else {
        // Not active - no background
        setAutoFillBackground(false);
        setPalette(QPalette());
    }
}

// ============================================================================
// MOUSE EVENT HANDLERS
// ============================================================================
// These handle mouse interactions on the tab widget.
// ============================================================================

// ============================================================================
// METHOD: mousePressEvent
// ============================================================================
// Called when the widget is clicked.
// We emit "clicked" to switch to this tab.
// ============================================================================
void TabEntryWidget::mousePressEvent(QMouseEvent* event)
{
    // Emit signal to switch tab
    emit clicked(m_tabId);

    // Let base class handle it too (important for proper event handling)
    QFrame::mousePressEvent(event);
}

// ============================================================================
// METHOD: enterEvent
// ============================================================================
// Called when mouse enters the widget (hover).
// We show a light highlight unless active.
// ============================================================================
void TabEntryWidget::enterEvent(QEnterEvent* event)
{
    if (!m_active) {
        // Light hover highlight
        QPalette pal = palette();
        QColor hoverColor = palette().color(QPalette::Highlight);
        hoverColor.setAlpha(80);  // Semi-transparent
        pal.setColor(QPalette::Window, hoverColor);
        setPalette(pal);
        setAutoFillBackground(true);
    }
    // Let base class handle it
    QFrame::enterEvent(event);
}

// ============================================================================
// METHOD: leaveEvent
// ============================================================================
// Called when mouse leaves the widget.
// We restore background based on active state.
// ============================================================================
void TabEntryWidget::leaveEvent(QEvent* event)
{
    // Update to proper background (active or not)
    updateBackground();
    // Let base class handle it
    QFrame::leaveEvent(event);
}