#pragma once
#include "include/cef_client.h"
#include "include/cef_life_span_handler.h"
#include "include/cef_render_handler.h"
#include <QImage>
#include <QObject>

class CefHandler : public QObject, public CefClient,
                   public CefLifeSpanHandler, public CefRenderHandler {
    Q_OBJECT
public:
    explicit CefHandler(QWidget* widget, QObject* parent = nullptr);
    ~CefHandler();

    CefRefPtr<CefBrowser> GetBrowser() const { return m_browser; }

    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    CefRefPtr<CefRenderHandler> GetRenderHandler() override { return this; }

    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

    void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
    void OnPaint(CefRefPtr<CefBrowser> browser,
                 PaintElementType type,
                 const RectList& dirtyRects,
                 const void* buffer,
                 int width,
                 int height) override;

    QImage* getImage() { return &m_image; }
    bool isDirty() { return m_dirty.exchange(true); }
    void clearDirty() { m_dirty = false; }

private:
    CefRefPtr<CefBrowser> m_browser;
    QWidget* m_widget;
    QImage m_image;
    std::atomic<bool> m_dirty{false};
    int m_width = 0;
    int m_height = 0;

    IMPLEMENT_REFCOUNTING(CefHandler);
};