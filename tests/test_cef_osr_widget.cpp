#include <QtTest/QtTest>
#include "cef_osr_widget.h"

// Tests for CefOSRWidget input and render flags.
// These tests do NOT require a real CEF instance — they test only the
// Qt-level flag state and behavior of CefOSRWidget.
//
// Note: CefOSRWidget::m_handler is null throughout these tests, which is
// intentional. The flag accessors (inputEnabled/renderEnabled) and setters
// operate independently of the handler pointer.
//
// Validates: Requirements 9.1, 9.2, 9.3

class TestCefOSRWidget : public QObject {
    Q_OBJECT

private slots:
    // Verify inputEnabled() returns true by default (Req 9.1, 9.2)
    void testInputEnabledDefault();

    // Verify renderEnabled() returns true by default (Req 9.3)
    void testRenderEnabledDefault();

    // Verify setInputEnabled(false) disables input (Req 9.1, 9.2)
    void testSetInputEnabledFalse();

    // Verify setInputEnabled(true) re-enables input after disabling (Req 9.4)
    void testSetInputEnabledTrue();

    // Verify setRenderEnabled(false) disables rendering (Req 9.3)
    void testSetRenderEnabledFalse();

    // Verify setRenderEnabled(true) re-enables rendering after disabling (Req 9.3)
    void testSetRenderEnabledTrue();
};

void TestCefOSRWidget::testInputEnabledDefault()
{
    CefOSRWidget widget;
    QVERIFY(widget.inputEnabled() == true);
}

void TestCefOSRWidget::testRenderEnabledDefault()
{
    CefOSRWidget widget;
    QVERIFY(widget.renderEnabled() == true);
}

void TestCefOSRWidget::testSetInputEnabledFalse()
{
    CefOSRWidget widget;
    widget.setInputEnabled(false);
    QVERIFY(widget.inputEnabled() == false);
}

void TestCefOSRWidget::testSetInputEnabledTrue()
{
    CefOSRWidget widget;
    widget.setInputEnabled(false);
    QVERIFY(widget.inputEnabled() == false);
    widget.setInputEnabled(true);
    QVERIFY(widget.inputEnabled() == true);
}

void TestCefOSRWidget::testSetRenderEnabledFalse()
{
    CefOSRWidget widget;
    widget.setRenderEnabled(false);
    QVERIFY(widget.renderEnabled() == false);
}

void TestCefOSRWidget::testSetRenderEnabledTrue()
{
    CefOSRWidget widget;
    widget.setRenderEnabled(false);
    QVERIFY(widget.renderEnabled() == false);
    widget.setRenderEnabled(true);
    QVERIFY(widget.renderEnabled() == true);
}

QTEST_MAIN(TestCefOSRWidget)
#include "test_cef_osr_widget.moc"
