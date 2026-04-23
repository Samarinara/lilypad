#include "cef_handler.h"
#include <QPainter>
#include <QWidget>

CefHandler::CefHandler(QWidget* widget, QObject* parent)
    : QObject(parent), m_widget(widget) {}

CefHandler::~CefHandler() {}

void CefHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    m_browser = browser;
}

void CefHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
    if (browser->IsSame(m_browser))
        m_browser = nullptr;
}

void CefHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) {
    if (m_widget) {
        rect.Set(0, 0, m_widget->width(), m_widget->height());
    } else {
        rect.Set(0, 0, 800, 600);
    }
}

void CefHandler::OnPaint(CefRefPtr<CefBrowser> browser,
                        PaintElementType type,
                        const RectList& dirtyRects,
                        const void* buffer,
                        int width,
                        int height) {
    if (type == PET_VIEW) {
        m_image = QImage(static_cast<const uchar*>(buffer), width, height,
                        width * 4, QImage::Format_ARGB32).copy();
        m_width = width;
        m_height = height;
        m_dirty.exchange(true);

        if (m_widget) {
            m_widget->update();
        }
    }
}