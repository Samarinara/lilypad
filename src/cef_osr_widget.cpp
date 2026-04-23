#include "cef_osr_widget.h"
#include <QImage>
#include <QPainter>
#include <QMouseEvent>

CefOSRWidget::CefOSRWidget(QWidget* parent) : QWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

CefOSRWidget::~CefOSRWidget() {}

void CefOSRWidget::paintEvent(QPaintEvent* event) {
    if (!m_handler) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!m_handler->getImage()->isNull()) {
        QImage img = m_handler->getImage()->scaled(size(), Qt::IgnoreAspectRatio,
                                                  Qt::SmoothTransformation);
        painter.drawImage(rect(), img);
        m_handler->clearDirty();
    } else {
        painter.fillRect(rect(), palette().window());
    }
}

void CefOSRWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_handler && m_handler->GetBrowser()) {
        m_handler->GetBrowser()->GetHost()->WasResized();
    }
}

void CefOSRWidget::mousePressEvent(QMouseEvent* event) {
    if (m_handler && m_handler->GetBrowser()) {
        CefRefPtr<CefBrowserHost> host = m_handler->GetBrowser()->GetHost();
        CefMouseEvent cefEvent;
        cefEvent.x = event->position().x();
        cefEvent.y = event->position().y();
        cefEvent.modifiers = 0;
        host->SendMouseClickEvent(cefEvent, MBT_LEFT, false, 1);
    }
}

void CefOSRWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (m_handler && m_handler->GetBrowser()) {
        CefRefPtr<CefBrowserHost> host = m_handler->GetBrowser()->GetHost();
        CefMouseEvent cefEvent;
        cefEvent.x = event->position().x();
        cefEvent.y = event->position().y();
        cefEvent.modifiers = 0;
        host->SendMouseClickEvent(cefEvent, MBT_LEFT, true, 1);
    }
}

void CefOSRWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_handler && m_handler->GetBrowser()) {
        CefRefPtr<CefBrowserHost> host = m_handler->GetBrowser()->GetHost();
        CefMouseEvent cefEvent;
        cefEvent.x = event->position().x();
        cefEvent.y = event->position().y();
        cefEvent.modifiers = 0;
        host->SendMouseMoveEvent(cefEvent, false);
    }
}

void CefOSRWidget::wheelEvent(QWheelEvent* event) {
    if (m_handler && m_handler->GetBrowser()) {
        CefRefPtr<CefBrowserHost> host = m_handler->GetBrowser()->GetHost();
        CefMouseEvent cefEvent;
        cefEvent.x = event->position().x();
        cefEvent.y = event->position().y();
        cefEvent.modifiers = 0;
        host->SendMouseWheelEvent(cefEvent, 0, event->angleDelta().y());
    }
}

void CefOSRWidget::keyPressEvent(QKeyEvent* event) {
    if (m_handler && m_handler->GetBrowser()) {
    }
}

void CefOSRWidget::keyReleaseEvent(QKeyEvent* event) {
    if (m_handler && m_handler->GetBrowser()) {
    }
}