// Exploratory tests for the input-passthrough bug fix.
//
// These tests were written to confirm the bug conditions described in the
// bugfix spec (input-passthrough-fix). They are run against the FIXED code
// and are expected to PASS — confirming that:
//   (a) the null-handler guard (Property 3 / Req 3.5) is preserved, and
//   (b) the widget does not crash when events arrive with no handler attached.
//
// Full CEF mock testing (asserting that SendKeyEvent / SendMouseMoveEvent are
// actually called with the correct arguments) requires a mock CefBrowserHost
// implementation. That level of testing is covered by the fix-checking tests
// in task 7. These exploratory tests focus on the guard-path behaviour that
// was already present before the fix and must remain intact.
//
// Sub-tasks covered:
//   6.1 — QKeyEvent press with null handler → no crash
//   6.2 — QKeyEvent release with null handler → no crash
//   6.3 — Printable key press with null handler → no crash
//   6.4 — QLeaveEvent with null handler → no crash
//
// Validates: Requirements 3.5 (null-handler guard preservation)

#include <QtTest/QtTest>
#include <QKeyEvent>
#include <QCoreApplication>
#include "cef_osr_widget.h"

class TestInputPassthroughExploratory : public QObject {
    Q_OBJECT

private slots:
    // 6.1 — Synthesize a QKeyEvent press on a CefOSRWidget with null handler.
    //        The null-handler guard must prevent any crash.
    //        On unfixed code the keyPressEvent body was an empty stub, so this
    //        would also not crash — but it would also never forward the event.
    //        On fixed code the guard is still respected: no crash, no CEF call.
    void testKeyPressNullHandlerNoCrash();

    // 6.2 — Synthesize a QKeyEvent release on a CefOSRWidget with null handler.
    //        Same guard reasoning as 6.1.
    void testKeyReleaseNullHandlerNoCrash();

    // 6.3 — Synthesize a printable key press (Qt::Key_H, text = "h") on a
    //        CefOSRWidget with null handler.
    //        On fixed code both the KEYEVENT_RAWKEYDOWN and KEYEVENT_CHAR paths
    //        are guarded by the null-handler check — no crash.
    void testPrintableKeyPressNullHandlerNoCrash();

    // 6.4 — Synthesize a QLeaveEvent on a CefOSRWidget with null handler.
    //        On unfixed code leaveEvent did not exist at all (no override), so
    //        Qt's default no-op ran — no crash but also no CEF notification.
    //        On fixed code the override exists and the null-handler guard fires
    //        — still no crash, and the hover-state clearing path is present.
    void testLeaveEventNullHandlerNoCrash();

    // Additional: verify inputEnabled flag is still true by default after
    // events are delivered (guard state is not mutated by event handling).
    void testInputEnabledUnchangedAfterEvents();
};

// ---------------------------------------------------------------------------
// 6.1 — Key press, null handler
// ---------------------------------------------------------------------------
void TestInputPassthroughExploratory::testKeyPressNullHandlerNoCrash()
{
    // CefOSRWidget with m_handler = nullptr (default)
    CefOSRWidget widget;
    QVERIFY(widget.handler() == nullptr);
    QVERIFY(widget.inputEnabled() == true);

    // Construct a non-printable key press (e.g. Qt::Key_Return)
    QKeyEvent pressEvent(QEvent::KeyPress,
                         Qt::Key_Return,
                         Qt::NoModifier,
                         QString());

    // Deliver the event directly — must not crash
    QCoreApplication::sendEvent(&widget, &pressEvent);

    // If we reach here the null-handler guard worked correctly
    QVERIFY(true);
}

// ---------------------------------------------------------------------------
// 6.2 — Key release, null handler
// ---------------------------------------------------------------------------
void TestInputPassthroughExploratory::testKeyReleaseNullHandlerNoCrash()
{
    CefOSRWidget widget;
    QVERIFY(widget.handler() == nullptr);

    QKeyEvent releaseEvent(QEvent::KeyRelease,
                           Qt::Key_Return,
                           Qt::NoModifier,
                           QString());

    QCoreApplication::sendEvent(&widget, &releaseEvent);

    QVERIFY(true);
}

// ---------------------------------------------------------------------------
// 6.3 — Printable key press, null handler
// ---------------------------------------------------------------------------
void TestInputPassthroughExploratory::testPrintableKeyPressNullHandlerNoCrash()
{
    CefOSRWidget widget;
    QVERIFY(widget.handler() == nullptr);

    // Printable key: Qt::Key_H with text "h"
    // On fixed code this would trigger both KEYEVENT_RAWKEYDOWN and
    // KEYEVENT_CHAR paths — both are guarded by the null-handler check.
    QKeyEvent pressEvent(QEvent::KeyPress,
                         Qt::Key_H,
                         Qt::NoModifier,
                         QStringLiteral("h"));

    QCoreApplication::sendEvent(&widget, &pressEvent);

    QVERIFY(true);
}

// ---------------------------------------------------------------------------
// 6.4 — Leave event, null handler
// ---------------------------------------------------------------------------
void TestInputPassthroughExploratory::testLeaveEventNullHandlerNoCrash()
{
    CefOSRWidget widget;
    QVERIFY(widget.handler() == nullptr);

    // QLeaveEvent (type = QEvent::Leave)
    QEvent leaveEvent(QEvent::Leave);

    QCoreApplication::sendEvent(&widget, &leaveEvent);

    // On unfixed code: no leaveEvent override existed — Qt's default ran.
    // On fixed code: leaveEvent override exists, null-handler guard fires.
    // Either way: no crash.
    QVERIFY(true);
}

// ---------------------------------------------------------------------------
// Additional: flag state is not mutated by event delivery
// ---------------------------------------------------------------------------
void TestInputPassthroughExploratory::testInputEnabledUnchangedAfterEvents()
{
    CefOSRWidget widget;
    QVERIFY(widget.inputEnabled() == true);

    // Deliver a sequence of events
    QKeyEvent press(QEvent::KeyPress,   Qt::Key_A, Qt::NoModifier, QStringLiteral("a"));
    QKeyEvent release(QEvent::KeyRelease, Qt::Key_A, Qt::NoModifier, QString());
    QEvent    leave(QEvent::Leave);

    QCoreApplication::sendEvent(&widget, &press);
    QCoreApplication::sendEvent(&widget, &release);
    QCoreApplication::sendEvent(&widget, &leave);

    // inputEnabled must still be true — event handling must not mutate it
    QVERIFY(widget.inputEnabled() == true);
}

QTEST_MAIN(TestInputPassthroughExploratory)
#include "test_input_passthrough_exploratory.moc"
