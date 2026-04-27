#include "tab_entry_widget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QMovie>
#include <QPushButton>
#include <QFontMetrics>
#include <QPainter>
#include <QSizePolicy>

TabEntryWidget::TabEntryWidget(int tabId, QWidget* parent)
    : QFrame(parent)
    , m_tabId(tabId)
{
    setFixedHeight(36);
    setFrameShape(QFrame::NoFrame);
    setCursor(Qt::PointingHandCursor);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    // Favicon label: fixed 16x16
    m_faviconLabel = new QLabel(this);
    m_faviconLabel->setFixedSize(16, 16);
    m_faviconLabel->setScaledContents(true);

    // Title label: expands horizontally, plain text, elided
    m_titleLabel = new QLabel(this);
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_titleLabel->setTextFormat(Qt::PlainText);

    // Close button: fixed 16x16, flat, "×"
    m_closeBtn = new QPushButton(QStringLiteral("×"), this);
    m_closeBtn->setFixedSize(16, 16);
    m_closeBtn->setFlat(true);
    m_closeBtn->setCursor(Qt::ArrowCursor);
    connect(m_closeBtn, &QPushButton::clicked, this, [this]() {
        emit closeClicked(m_tabId);
    });

    layout->addWidget(m_faviconLabel);
    layout->addWidget(m_titleLabel, 1);
    layout->addWidget(m_closeBtn);

    setLayout(layout);

    // Show a default placeholder favicon initially
    QPixmap placeholder(16, 16);
    placeholder.fill(Qt::gray);
    m_faviconLabel->setPixmap(placeholder);
}

TabEntryWidget::~TabEntryWidget() = default;

void TabEntryWidget::setActive(bool active)
{
    m_active = active;
    updateBackground();
}

void TabEntryWidget::setTitle(const QString& title)
{
    m_title = title;
    updateTitleDisplay();
}

void TabEntryWidget::setUrl(const QString& url)
{
    m_url = url;
    updateTitleDisplay();
}

void TabEntryWidget::updateTitleDisplay()
{
    const QString& text = m_title.isEmpty() ? m_url : m_title;

    // Elide text to fit the label width
    QFontMetrics fm(m_titleLabel->font());
    int availableWidth = m_titleLabel->width();
    if (availableWidth <= 0) {
        // Widget not yet laid out — use a reasonable default
        availableWidth = 140;
    }
    const QString elided = fm.elidedText(text, Qt::ElideRight, availableWidth);
    m_titleLabel->setText(elided);
}

void TabEntryWidget::setFavicon(const QPixmap& pixmap)
{
    // Cache the pixmap regardless of loading state
    m_faviconPixmap = pixmap;

    // If currently loading, don't replace the spinner
    if (m_loading) {
        return;
    }

    if (pixmap.isNull()) {
        // Show a gray placeholder
        QPixmap placeholder(16, 16);
        placeholder.fill(Qt::gray);
        m_faviconLabel->setPixmap(placeholder);
    } else {
        m_faviconLabel->setPixmap(pixmap.scaled(16, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    }
    m_faviconLabel->setText(QString());
}

void TabEntryWidget::setLoading(bool loading)
{
    m_loading = loading;

    if (loading) {
        // Show a simple text spinner in the favicon label
        m_faviconLabel->setPixmap(QPixmap());
        m_faviconLabel->setText(QStringLiteral("⟳"));
    } else {
        // Restore favicon or placeholder
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

void TabEntryWidget::updateBackground()
{
    if (m_active) {
        QPalette pal = palette();
        pal.setColor(QPalette::Window, palette().color(QPalette::Highlight));
        setPalette(pal);
        setAutoFillBackground(true);
    } else {
        setAutoFillBackground(false);
        setPalette(QPalette());
    }
}

void TabEntryWidget::mousePressEvent(QMouseEvent* event)
{
    emit clicked(m_tabId);
    QFrame::mousePressEvent(event);
}

void TabEntryWidget::enterEvent(QEnterEvent* event)
{
    if (!m_active) {
        // Apply a lighter hover background
        QPalette pal = palette();
        QColor hoverColor = palette().color(QPalette::Highlight);
        hoverColor.setAlpha(80);
        pal.setColor(QPalette::Window, hoverColor);
        setPalette(pal);
        setAutoFillBackground(true);
    }
    QFrame::enterEvent(event);
}

void TabEntryWidget::leaveEvent(QEvent* event)
{
    // Restore background based on active state
    updateBackground();
    QFrame::leaveEvent(event);
}
