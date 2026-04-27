// Fix-checking unit tests for the input-passthrough bug fix.
//
// These tests run against the FIXED code and are expected to PASS.
// They verify that:
//   (a) keyPressEvent with a non-printable key sends exactly one
//       SendKeyEvent(KEYEVENT_RAWKEYDOWN) call  (Property 1 / sub-task 7.1)
//   (b) keyPressEvent with a printable key sends two SendKeyEvent calls in
//       order: KEYEVENT_RAWKEYDOWN then KEYEVENT_CHAR  (Property 1 / 7.2)
//   (c) keyReleaseEvent sends exactly one SendKeyEvent(KEYEVENT_KEYUP) call
//       (Property 1 / 7.3)
//   (d) leaveEvent sends exactly one SendMouseMoveEvent call with
//       mouseLeave = true  (Property 2 / 7.4)
//   (e) All four paths produce zero CEF calls when m_inputEnabled = false
//       (Property 3 / 7.5)
//   (f) All four paths produce zero CEF calls and no crash when m_handler
//       is null  (Property 3 / 7.6)
//
// Mock strategy
// -------------
// CefOSRWidget calls m_handler->GetBrowser()->GetHost()->SendKeyEvent(...)
// and ->SendMouseMoveEvent(...).  We need to intercept at the CefBrowserHost
// level.  Because CefBrowserHost and CefBrowser are abstract CEF classes with
// many pure virtual methods, we provide minimal recording subclasses that stub
// every pure virtual with an empty body and record only the calls we care
// about.
//
// CefHandler is a QObject + multiple CEF interfaces with IMPLEMENT_REFCOUNTING,
// which makes it non-trivially subclassable.  Instead we bypass CefHandler
// entirely: CefOSRWidget::setHandler() accepts a raw CefHandler*, so we
// create a thin TestCefHandler subclass that overrides only GetBrowser() to
// return our RecordingCefBrowser.
//
// Validates: Requirements 2.1, 2.2, 2.3, 3.1, 3.5

#include <QtTest/QtTest>
#include <QKeyEvent>
#include <QCoreApplication>

#include "cef_osr_widget.h"
#include "cef_handler.h"

// CEF headers needed for the mock classes
#include "include/cef_browser.h"
#include "include/cef_client.h"
#include "include/cef_drag_data.h"
#include "include/cef_navigation_entry.h"
#include "include/cef_registration.h"
#include "include/cef_request_context.h"

// ---------------------------------------------------------------------------
// RecordingCefBrowserHost
// ---------------------------------------------------------------------------
// Records every SendKeyEvent and SendMouseMoveEvent call.
// All other pure virtual methods are stubbed with empty / default-return
// implementations so the class can be instantiated.

struct KeyEventRecord {
    cef_key_event_type_t type;
    int                  windows_key_code;
    uint16_t             character;
};

struct MouseMoveRecord {
    bool mouseLeave;
};

class RecordingCefBrowserHost : public CefBrowserHost {
public:
    std::vector<KeyEventRecord>   keyEvents;
    std::vector<MouseMoveRecord>  mouseMoveEvents;

    // ---- The two methods we actually care about ----
    void SendKeyEvent(const CefKeyEvent& event) override {
        keyEvents.push_back({event.type,
                             event.windows_key_code,
                             event.character});
    }

    void SendMouseMoveEvent(const CefMouseEvent& /*event*/,
                            bool mouseLeave) override {
        mouseMoveEvents.push_back({mouseLeave});
    }

    // ---- Stubs for all other pure virtual methods ----
    CefRefPtr<CefBrowser> GetBrowser() override { return nullptr; }
    void CloseBrowser(bool) override {}
    bool TryCloseBrowser() override { return false; }
    bool IsReadyToBeClosed() override { return false; }
    void SetFocus(bool) override {}
    CefWindowHandle GetWindowHandle() override { return CefWindowHandle{}; }
    CefWindowHandle GetOpenerWindowHandle() override { return CefWindowHandle{}; }
    int GetOpenerIdentifier() override { return 0; }
    bool HasView() override { return false; }
    CefRefPtr<CefClient> GetClient() override { return nullptr; }
    CefRefPtr<CefRequestContext> GetRequestContext() override { return nullptr; }
    bool CanZoom(cef_zoom_command_t) override { return false; }
    void Zoom(cef_zoom_command_t) override {}
    double GetDefaultZoomLevel() override { return 0.0; }
    double GetZoomLevel() override { return 0.0; }
    void SetZoomLevel(double) override {}
    void RunFileDialog(FileDialogMode, const CefString&, const CefString&,
                       const std::vector<CefString>&,
                       CefRefPtr<CefRunFileDialogCallback>) override {}
    void StartDownload(const CefString&) override {}
    void DownloadImage(const CefString&, bool, uint32_t, bool,
                       CefRefPtr<CefDownloadImageCallback>) override {}
    void Print() override {}
    void PrintToPDF(const CefString&, const CefPdfPrintSettings&,
                    CefRefPtr<CefPdfPrintCallback>) override {}
    void Find(const CefString&, bool, bool, bool) override {}
    void StopFinding(bool) override {}
    void ShowDevTools(const CefWindowInfo&, CefRefPtr<CefClient>,
                      const CefBrowserSettings&, const CefPoint&) override {}
    void CloseDevTools() override {}
    bool HasDevTools() override { return false; }
    bool SendDevToolsMessage(const void*, size_t) override { return false; }
    int ExecuteDevToolsMethod(int, const CefString&,
                              CefRefPtr<CefDictionaryValue>) override { return 0; }
    CefRefPtr<CefRegistration> AddDevToolsMessageObserver(
        CefRefPtr<CefDevToolsMessageObserver>) override { return nullptr; }
    void GetNavigationEntries(CefRefPtr<CefNavigationEntryVisitor>,
                              bool) override {}
    void ReplaceMisspelling(const CefString&) override {}
    void AddWordToDictionary(const CefString&) override {}
    bool IsWindowRenderingDisabled() override { return false; }
    void WasResized() override {}
    void WasHidden(bool) override {}
    void NotifyScreenInfoChanged() override {}
    void Invalidate(PaintElementType) override {}
    void SendExternalBeginFrame() override {}
    void SendMouseClickEvent(const CefMouseEvent&, MouseButtonType,
                             bool, int) override {}
    void SendMouseWheelEvent(const CefMouseEvent&, int, int) override {}
    void SendTouchEvent(const CefTouchEvent&) override {}
    void SendCaptureLostEvent() override {}
    void NotifyMoveOrResizeStarted() override {}
    int GetWindowlessFrameRate() override { return 30; }
    void SetWindowlessFrameRate(int) override {}
    void ImeSetComposition(const CefString&,
                           const std::vector<CefCompositionUnderline>&,
                           const CefRange&, const CefRange&) override {}
    void ImeCommitText(const CefString&, const CefRange&, int) override {}
    void ImeFinishComposingText(bool) override {}
    void ImeCancelComposition() override {}
    void DragTargetDragEnter(CefRefPtr<CefDragData>, const CefMouseEvent&,
                             DragOperationsMask) override {}
    void DragTargetDragOver(const CefMouseEvent&, DragOperationsMask) override {}
    void DragTargetDragLeave() override {}
    void DragTargetDrop(const CefMouseEvent&) override {}
    void DragSourceEndedAt(int, int, DragOperationsMask) override {}
    void DragSourceSystemDragEnded() override {}
    CefRefPtr<CefNavigationEntry> GetVisibleNavigationEntry() override { return nullptr; }
    void SetAccessibilityState(cef_state_t) override {}
    void SetAutoResizeEnabled(bool, const CefSize&, const CefSize&) override {}
    void SetAudioMuted(bool) override {}
    bool IsAudioMuted() override { return false; }
    bool IsFullscreen() override { return false; }
    void ExitFullscreen(bool) override {}
    bool CanExecuteChromeCommand(int) override { return false; }
    void ExecuteChromeCommand(int, cef_window_open_disposition_t) override {}
    bool IsRenderProcessUnresponsive() override { return false; }
    cef_runtime_style_t GetRuntimeStyle() override { return CEF_RUNTIME_STYLE_DEFAULT; }
    void SetAxViewportCollapse(bool) override {}

    IMPLEMENT_REFCOUNTING(RecordingCefBrowserHost);
};

// ---------------------------------------------------------------------------
// RecordingCefBrowser
// ---------------------------------------------------------------------------
// Returns our RecordingCefBrowserHost from GetHost().
// All other pure virtual methods are stubbed.

class RecordingCefBrowser : public CefBrowser {
public:
    explicit RecordingCefBrowser()
        : m_host(new RecordingCefBrowserHost()) {}

    RecordingCefBrowserHost* host() { return m_host.get(); }

    // ---- The method CefOSRWidget uses ----
    CefRefPtr<CefBrowserHost> GetHost() override { return m_host; }

    // ---- Stubs ----
    bool IsValid() override { return true; }
    bool CanGoBack() override { return false; }
    void GoBack() override {}
    bool CanGoForward() override { return false; }
    void GoForward() override {}
    bool IsLoading() override { return false; }
    void Reload() override {}
    void ReloadIgnoreCache() override {}
    void StopLoad() override {}
    int GetIdentifier() override { return 1; }
    bool IsSame(CefRefPtr<CefBrowser>) override { return false; }
    bool IsPopup() override { return false; }
    bool HasDocument() override { return false; }
    CefRefPtr<CefFrame> GetMainFrame() override { return nullptr; }
    CefRefPtr<CefFrame> GetFocusedFrame() override { return nullptr; }
    CefRefPtr<CefFrame> GetFrameByIdentifier(const CefString&) override { return nullptr; }
    CefRefPtr<CefFrame> GetFrameByName(const CefString&) override { return nullptr; }
    size_t GetFrameCount() override { return 0; }
    void GetFrameIdentifiers(std::vector<CefString>&) override {}
    void GetFrameNames(std::vector<CefString>&) override {}

private:
    CefRefPtr<RecordingCefBrowserHost> m_host;
    IMPLEMENT_REFCOUNTING(RecordingCefBrowser);
};

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------

// Helper: create a CefHandler with a RecordingCefBrowser injected via
// setBrowserForTesting().  Returns the recording host for inspection.
// The caller owns the CefHandler (stack-allocated) and must ensure it
// outlives the CefOSRWidget it is attached to.
static CefRefPtr<RecordingCefBrowserHost>
attachRecordingHandler(CefOSRWidget& widget, CefHandler& handler)
{
    CefRefPtr<RecordingCefBrowser> browser = new RecordingCefBrowser();
    handler.setBrowserForTesting(browser);
    widget.setHandler(&handler);
    return browser->host();
}

class TestInputPassthroughFixChecking : public QObject {
    Q_OBJECT

private slots:
    // 7.1 — Non-printable key press → exactly one KEYEVENT_RAWKEYDOWN call
    void testNonPrintableKeyPress_sendsRawKeyDown();

    // 7.2 — Printable key press → KEYEVENT_RAWKEYDOWN then KEYEVENT_CHAR
    void testPrintableKeyPress_sendsRawKeyDownThenChar();

    // 7.3 — Key release → exactly one KEYEVENT_KEYUP call
    void testKeyRelease_sendsKeyUp();

    // 7.4 — Leave event → exactly one SendMouseMoveEvent(mouseLeave=true)
    void testLeaveEvent_sendsMouseLeave();

    // 7.5 — All four paths with m_inputEnabled = false → zero CEF calls
    void testInputDisabled_nonPrintableKeyPress_noCefCalls();
    void testInputDisabled_printableKeyPress_noCefCalls();
    void testInputDisabled_keyRelease_noCefCalls();
    void testInputDisabled_leaveEvent_noCefCalls();

    // 7.6 — All four paths with null m_handler → zero CEF calls, no crash
    void testNullHandler_nonPrintableKeyPress_noCrashNoCefCalls();
    void testNullHandler_printableKeyPress_noCrashNoCefCalls();
    void testNullHandler_keyRelease_noCrashNoCefCalls();
    void testNullHandler_leaveEvent_noCrashNoCefCalls();
};

// ---------------------------------------------------------------------------
// 7.1 — Non-printable key press
// ---------------------------------------------------------------------------
void TestInputPassthroughFixChecking::testNonPrintableKeyPress_sendsRawKeyDown()
{
    // Validates: Requirements 2.1
    CefHandler handler(nullptr);
    CefOSRWidget widget;
    CefRefPtr<RecordingCefBrowserHost> host = attachRecordingHandler(widget, handler);

    // Qt::Key_Return is non-printable (text() is empty)
    QKeyEvent press(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, QString());
    QCoreApplication::sendEvent(&widget, &press);

    QCOMPARE(host->keyEvents.size(), std::size_t(1));
    QCOMPARE(host->keyEvents[0].type, KEYEVENT_RAWKEYDOWN);
}

// ---------------------------------------------------------------------------
// 7.2 — Printable key press
// ---------------------------------------------------------------------------
void TestInputPassthroughFixChecking::testPrintableKeyPress_sendsRawKeyDownThenChar()
{
    // Validates: Requirements 2.1
    CefHandler handler(nullptr);
    CefOSRWidget widget;
    CefRefPtr<RecordingCefBrowserHost> host = attachRecordingHandler(widget, handler);

    // Qt::Key_H with text "h" is printable
    QKeyEvent press(QEvent::KeyPress, Qt::Key_H, Qt::NoModifier,
                    QStringLiteral("h"));
    QCoreApplication::sendEvent(&widget, &press);

    QCOMPARE(host->keyEvents.size(), std::size_t(2));
    QCOMPARE(host->keyEvents[0].type, KEYEVENT_RAWKEYDOWN);
    QCOMPARE(host->keyEvents[1].type, KEYEVENT_CHAR);
    // The character field must match the unicode value of 'h'
    QCOMPARE(host->keyEvents[1].character,
             static_cast<uint16_t>(QStringLiteral("h").at(0).unicode()));
}

// ---------------------------------------------------------------------------
// 7.3 — Key release
// ---------------------------------------------------------------------------
void TestInputPassthroughFixChecking::testKeyRelease_sendsKeyUp()
{
    // Validates: Requirements 2.2
    CefHandler handler(nullptr);
    CefOSRWidget widget;
    CefRefPtr<RecordingCefBrowserHost> host = attachRecordingHandler(widget, handler);

    QKeyEvent release(QEvent::KeyRelease, Qt::Key_Return, Qt::NoModifier,
                      QString());
    QCoreApplication::sendEvent(&widget, &release);

    QCOMPARE(host->keyEvents.size(), std::size_t(1));
    QCOMPARE(host->keyEvents[0].type, KEYEVENT_KEYUP);
}

// ---------------------------------------------------------------------------
// 7.4 — Leave event
// ---------------------------------------------------------------------------
void TestInputPassthroughFixChecking::testLeaveEvent_sendsMouseLeave()
{
    // Validates: Requirements 2.3
    CefHandler handler(nullptr);
    CefOSRWidget widget;
    CefRefPtr<RecordingCefBrowserHost> host = attachRecordingHandler(widget, handler);

    QEvent leave(QEvent::Leave);
    QCoreApplication::sendEvent(&widget, &leave);

    QCOMPARE(host->mouseMoveEvents.size(), std::size_t(1));
    QVERIFY(host->mouseMoveEvents[0].mouseLeave == true);
}

// ---------------------------------------------------------------------------
// 7.5 — m_inputEnabled = false → zero CEF calls
// ---------------------------------------------------------------------------
void TestInputPassthroughFixChecking::testInputDisabled_nonPrintableKeyPress_noCefCalls()
{
    // Validates: Requirements 3.1
    CefHandler handler(nullptr);
    CefOSRWidget widget;
    CefRefPtr<RecordingCefBrowserHost> host = attachRecordingHandler(widget, handler);
    widget.setInputEnabled(false);

    QKeyEvent press(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, QString());
    QCoreApplication::sendEvent(&widget, &press);

    QCOMPARE(host->keyEvents.size(), std::size_t(0));
}

void TestInputPassthroughFixChecking::testInputDisabled_printableKeyPress_noCefCalls()
{
    // Validates: Requirements 3.1
    CefHandler handler(nullptr);
    CefOSRWidget widget;
    CefRefPtr<RecordingCefBrowserHost> host = attachRecordingHandler(widget, handler);
    widget.setInputEnabled(false);

    QKeyEvent press(QEvent::KeyPress, Qt::Key_H, Qt::NoModifier,
                    QStringLiteral("h"));
    QCoreApplication::sendEvent(&widget, &press);

    QCOMPARE(host->keyEvents.size(), std::size_t(0));
}

void TestInputPassthroughFixChecking::testInputDisabled_keyRelease_noCefCalls()
{
    // Validates: Requirements 3.1
    CefHandler handler(nullptr);
    CefOSRWidget widget;
    CefRefPtr<RecordingCefBrowserHost> host = attachRecordingHandler(widget, handler);
    widget.setInputEnabled(false);

    QKeyEvent release(QEvent::KeyRelease, Qt::Key_Return, Qt::NoModifier,
                      QString());
    QCoreApplication::sendEvent(&widget, &release);

    QCOMPARE(host->keyEvents.size(), std::size_t(0));
}

void TestInputPassthroughFixChecking::testInputDisabled_leaveEvent_noCefCalls()
{
    // Validates: Requirements 3.1
    CefHandler handler(nullptr);
    CefOSRWidget widget;
    CefRefPtr<RecordingCefBrowserHost> host = attachRecordingHandler(widget, handler);
    widget.setInputEnabled(false);

    QEvent leave(QEvent::Leave);
    QCoreApplication::sendEvent(&widget, &leave);

    QCOMPARE(host->mouseMoveEvents.size(), std::size_t(0));
}

// ---------------------------------------------------------------------------
// 7.6 — null m_handler → zero CEF calls, no crash
// ---------------------------------------------------------------------------
void TestInputPassthroughFixChecking::testNullHandler_nonPrintableKeyPress_noCrashNoCefCalls()
{
    // Validates: Requirements 3.5
    CefOSRWidget widget;
    QVERIFY(widget.handler() == nullptr);

    QKeyEvent press(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, QString());
    QCoreApplication::sendEvent(&widget, &press);

    // Reaching here without a crash is the assertion.
    QVERIFY(true);
}

void TestInputPassthroughFixChecking::testNullHandler_printableKeyPress_noCrashNoCefCalls()
{
    // Validates: Requirements 3.5
    CefOSRWidget widget;
    QVERIFY(widget.handler() == nullptr);

    QKeyEvent press(QEvent::KeyPress, Qt::Key_H, Qt::NoModifier,
                    QStringLiteral("h"));
    QCoreApplication::sendEvent(&widget, &press);

    QVERIFY(true);
}

void TestInputPassthroughFixChecking::testNullHandler_keyRelease_noCrashNoCefCalls()
{
    // Validates: Requirements 3.5
    CefOSRWidget widget;
    QVERIFY(widget.handler() == nullptr);

    QKeyEvent release(QEvent::KeyRelease, Qt::Key_Return, Qt::NoModifier,
                      QString());
    QCoreApplication::sendEvent(&widget, &release);

    QVERIFY(true);
}

void TestInputPassthroughFixChecking::testNullHandler_leaveEvent_noCrashNoCefCalls()
{
    // Validates: Requirements 3.5
    CefOSRWidget widget;
    QVERIFY(widget.handler() == nullptr);

    QEvent leave(QEvent::Leave);
    QCoreApplication::sendEvent(&widget, &leave);

    QVERIFY(true);
}

QTEST_MAIN(TestInputPassthroughFixChecking)
#include "test_input_passthrough_fix_checking.moc"
