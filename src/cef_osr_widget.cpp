#include "cef_osr_widget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>

CefOSRWidget::CefOSRWidget(QWidget* parent) : QWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

CefOSRWidget::~CefOSRWidget() {}

void CefOSRWidget::setInputEnabled(bool enabled) {
    m_inputEnabled = enabled;
}

void CefOSRWidget::setRenderEnabled(bool enabled) {
    m_renderEnabled = enabled;
}

void CefOSRWidget::paintEvent(QPaintEvent* event) {
    if (!m_renderEnabled) return;
    QPainter painter(this);
    painter.fillRect(rect(), palette().window());
}

void CefOSRWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_handler && m_handler->GetBrowser()) {
        m_handler->GetBrowser()->GetHost()->WasResized();
    }
}

void CefOSRWidget::mousePressEvent(QMouseEvent* event) {
    if (!m_inputEnabled) return;
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
    if (!m_inputEnabled) return;
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
    if (!m_inputEnabled) return;
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
    if (!m_inputEnabled) return;
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
    if (!m_inputEnabled) return;
    if (m_handler && m_handler->GetBrowser()) {
        CefRefPtr<CefBrowserHost> host = m_handler->GetBrowser()->GetHost();

        uint32_t modifiers = 0;
        Qt::KeyboardModifiers qtMods = event->modifiers();
        if (qtMods & Qt::ShiftModifier)   modifiers |= EVENTFLAG_SHIFT_DOWN;
        if (qtMods & Qt::ControlModifier) modifiers |= EVENTFLAG_CONTROL_DOWN;
        if (qtMods & Qt::AltModifier)     modifiers |= EVENTFLAG_ALT_DOWN;

        CefKeyEvent keyEvent;
        keyEvent.type             = KEYEVENT_RAWKEYDOWN;
        keyEvent.windows_key_code = static_cast<int>(event->nativeVirtualKey());
        keyEvent.native_key_code  = static_cast<int>(event->nativeScanCode());
        keyEvent.modifiers        = modifiers;
        host->SendKeyEvent(keyEvent);

        if (!event->text().isEmpty()) {
            uint16_t ch = event->text().at(0).unicode();
            CefKeyEvent charEvent;
            charEvent.type                  = KEYEVENT_CHAR;
            charEvent.windows_key_code      = static_cast<int>(event->nativeVirtualKey());
            charEvent.native_key_code       = static_cast<int>(event->nativeScanCode());
            charEvent.modifiers             = modifiers;
            charEvent.character             = ch;
            charEvent.unmodified_character  = ch;
            host->SendKeyEvent(charEvent);
        }
    }
}

void CefOSRWidget::keyReleaseEvent(QKeyEvent* event) {
    if (!m_inputEnabled) return;
    if (m_handler && m_handler->GetBrowser()) {
        CefRefPtr<CefBrowserHost> host = m_handler->GetBrowser()->GetHost();

        uint32_t modifiers = 0;
        Qt::KeyboardModifiers qtMods = event->modifiers();
        if (qtMods & Qt::ShiftModifier)   modifiers |= EVENTFLAG_SHIFT_DOWN;
        if (qtMods & Qt::ControlModifier) modifiers |= EVENTFLAG_CONTROL_DOWN;
        if (qtMods & Qt::AltModifier)     modifiers |= EVENTFLAG_ALT_DOWN;

        CefKeyEvent keyEvent;
        keyEvent.type             = KEYEVENT_KEYUP;
        keyEvent.windows_key_code = static_cast<int>(event->nativeVirtualKey());
        keyEvent.native_key_code  = static_cast<int>(event->nativeScanCode());
        keyEvent.modifiers        = modifiers;
        host->SendKeyEvent(keyEvent);
    }
}

void CefOSRWidget::leaveEvent(QEvent* event) {
    if (!m_inputEnabled) return;
    if (m_handler && m_handler->GetBrowser()) {
        CefRefPtr<CefBrowserHost> host = m_handler->GetBrowser()->GetHost();
        CefMouseEvent cefEvent;
        cefEvent.x = 0;
        cefEvent.y = 0;
        cefEvent.modifiers = 0;
        host->SendMouseMoveEvent(cefEvent, true);
    }
}