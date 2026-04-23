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