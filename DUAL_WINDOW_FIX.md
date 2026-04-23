# Lilypad CEF Browser - Dual Window Fix via OSR

## Problem Statement

The Lilypad Qt/CEF browser launches with **two windows**:
1. A blank window with the URL bar (Qt QMainWindow)
2. A separate window containing the webpage content (CEF-created window)

The URL bar correctly controls the page, but both should be in a **single unified window**.

## Root Cause

On Linux/X11, CEF's `SetAsChild()` function does not properly embed the browser window into a Qt widget hierarchy. Instead, CEF creates its own top-level X11 window.

## Solution: Off-Screen Rendering (OSR)

CEF renders the webpage to a memory buffer instead of a window. Qt paints this buffer using a custom render handler.

## Architecture After Fix

```
main.cpp
├── CEF initialization (CefInitialize)
├── QApplication
└── MainWindow (QMainWindow)
    ├── QLineEdit (URL bar)
    └── CefOSRWidget (QWidget)
        ├── Receives pixel buffer via CefRenderHandler
        └── Paints in QWidget::paintEvent()
```

## Implementation

### 1. Create CefOSRHandler class

Create `src/cef_osr_handler.h`:

```cpp
#pragma once
#include "include/cef_render_handler.h"
#include <QImage>
#include <QMutex>

class CefOSRHandler : public CefRenderHandler {
public:
    CefOSRHandler();

    bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
    void OnPaint(CefRefPtr<CefBrowser> browser,
                 PaintElementType type,
                 const RectList& dirtyRects,
                 const void* buffer,
                 int width,
                 int height) override;

    QImage* getImage() { QMutexLocker locker(&m_mutex); return &m_image; }
    bool isDirty() { return m_dirty; }
    void clearDirty() { m_dirty = false; }

private:
    QImage m_image;
    QMutex m_mutex;
    bool m_dirty = false;
    int m_width = 0;
    int m_height = 0;

    IMPLEMENT_REFCOUNTING(CefOSRHandler);
};
```

Create `src/cef_osr_handler.cpp`:

```cpp
#include "cef_osr_handler.h"
#include <QPainter>

CefOSRHandler::CefOSRHandler() {}

bool CefOSRHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) {
    rect.Set(0, 0, m_width, m_height);
    return true;
}

void CefOSRHandler::OnPaint(CefRefPtr<CefBrowser> browser,
                            PaintElementType type,
                            const RectList& dirtyRects,
                            const void* buffer,
                            int width,
                            int height) {
    if (type == PET_VIEW) {
        QMutexLocker locker(&m_mutex);
        if (m_width != width || m_height != height) {
            m_image = QImage(buffer, width, height, QImage::Format_ARGB32);
            m_width = width;
            m_height = height;
        } else {
            m_image = QImage(const_cast<void*>(buffer), width, height, QImage::Format_ARGB32);
        }
        m_dirty = true;
    }
}
```

### 2. Update CefHandler to use OSR

Update `src/cef_handler.h`:

```cpp
#pragma once
#include "include/cef_client.h"
#include "include/cef_life_span_handler.h"
#include "include/cef_render_handler.h"
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

    bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
    void OnPaint(CefRefPtr<CefBrowser> browser,
                 PaintElementType type,
                 const RectList& dirtyRects,
                 const void* buffer,
                 int width,
                 int height) override;

    QImage* getImage() { return &m_image; }
    bool isDirty() { return m_dirty.test_and_set(); }
    void clearDirty() { m_dirty.clear(); }

private:
    CefRefPtr<CefBrowser> m_browser;
    QWidget* m_widget;
    QImage m_image;
    std::atomic<bool> m_dirty{false};
    int m_width = 0;
    int m_height = 0;

    IMPLEMENT_REFCOUNTING(CefHandler);
};
```

Update `src/cef_handler.cpp`:

```cpp
#include "cef_handler.h"
#include <QPainter>

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

bool CefHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) {
    if (m_widget) {
        rect.Set(0, 0, m_widget->width(), m_widget->height());
    } else {
        rect.Set(0, 0, 800, 600);
    }
    return true;
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
        m_dirty.test_and_set();
        
        if (m_widget) {
            m_widget->update();
        }
    }
}
```

### 3. Create OSR Widget

Create `src/cef_osr_widget.h`:

```cpp
#pragma once
#include <QWidget>
#include "cef_handler.h"

class CefOSRWidget : public QWidget {
    Q_OBJECT
public:
    explicit CefOSRWidget(QWidget* parent = nullptr);
    ~CefOSRWidget();

    void setHandler(CefHandler* handler) { m_handler = handler; }
    CefHandler* handler() { return m_handler; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    CefHandler* m_handler = nullptr;
};
```

Create `src/cef_osr_widget.cpp`:

```cpp
#include "cef_osr_widget.h"
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
        host->SendMouseClickEvent(cefEvent, MBT_LEFT, false);
    }
}

void CefOSRWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (m_handler && m_handler->GetBrowser()) {
        CefRefPtr<CefBrowserHost> host = m_handler->GetBrowser()->GetHost();
        CefMouseEvent cefEvent;
        cefEvent.x = event->position().x();
        cefEvent.y = event->position().y();
        cefEvent.modifiers = 0;
        host->SendMouseClickEvent(cefEvent, MBT_LEFT, true);
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
        m_handler->GetBrowser()->GetHost()->SendKeyEvent(/* translate event */);
    }
}

void CefOSRWidget::keyReleaseEvent(QKeyEvent* event) {
    if (m_handler && m_handler->GetBrowser()) {
        m_handler->GetBrowser()->GetHost()->SendKeyEvent(/* translate event */);
    }
}
```

### 4. Update MainWindow

Update `src/mainwindow.cpp`:

```cpp
#include "mainwindow.h"
#include "cef_osr_widget.h"
#include <QLineEdit>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Lilypad");
    resize(1200, 800);

    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* lay = new QVBoxLayout(central);

    m_urlBar = new QLineEdit("https://polli.page");
    lay->addWidget(m_urlBar);

    m_osrWidget = new CefOSRWidget(central);
    lay->addWidget(m_osrWidget, 1);

    connect(m_urlBar, &QLineEdit::returnPressed, [this](){
        if (m_handler && m_handler->GetBrowser()) {
            m_handler->GetBrowser()->GetMainFrame()->LoadURL(m_urlBar->text().toStdString());
        }
    });
}

void MainWindow::createBrowser(const QString& url) {
    m_handler = new CefHandler(m_osrWidget, this);
    m_osrWidget->setHandler(m_handler);

    CefWindowInfo window_info;
    window_info.SetAsWindowless(m_osrWidget->winId());

    CefBrowserSettings settings;
    settings.windowless_frame_rate = 30;

    CefBrowserHost::CreateBrowser(window_info, m_handler,
                                  url.toStdString(), settings,
                                  nullptr, nullptr);
}
```

Update `src/mainwindow.h`:

```cpp
#pragma once
#include <QMainWindow>
class QLineEdit;
class CefOSRWidget;
class CefHandler;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    void createBrowser(const QString& url);

private:
    QLineEdit* m_urlBar;
    CefOSRWidget* m_osrWidget;
    CefHandler* m_handler;
};
```

### 5. Update CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.21)
project(Lilypad LANGUAGES C CXX)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CEF_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/cef_binary")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CEF_ROOT}/cmake")
find_package(CEF REQUIRED)

add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)

add_executable(lilypad
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/cef_handler.h
    src/cef_handler.cpp
    src/cef_osr_widget.h
    src/cef_osr_widget.cpp
)

target_include_directories(lilypad PRIVATE "${CEF_INCLUDE_PATH}")

target_link_libraries(lilypad PRIVATE
    Qt6::Widgets
    libcef_dll_wrapper
    "${CEF_LIB_RELEASE}"
    ${CEF_STANDARD_LIBS}
)

SET_CEF_TARGET_OUT_DIR()
set_target_properties(lilypad PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CEF_TARGET_OUT_DIR}
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
)
COPY_FILES(lilypad "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
COPY_FILES(lilypad "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")
```

## Files Reference

| File | Purpose |
|------|---------|
| `src/cef_handler.h` | CEF client handler with OSR support |
| `src/cef_handler.cpp` | Handler implementation |
| `src/cef_osr_widget.h` | Qt widget for OSR rendering |
| `src/cef_osr_widget.cpp` | Widget event handling |
| `src/mainwindow.h` | Main window definition |
| `src/mainwindow.cpp` | Window and browser creation |
| `src/main.cpp` | CEF and Qt initialization |
| `CMakeLists.txt` | Build configuration |
| `cef_binary/` | CEF SDK (v146) |