// Preservation tests for the input-passthrough bug fix.
//
// These tests run against the FIXED code and are expected to PASS.
// They verify Property 3: that all existing mouse event forwarding paths
// are UNCHANGED by the fix.
//
// Sub-tasks covered:
//   8.1 — mousePressEvent still calls SendMouseClickEvent(_, MBT_LEFT, false, 1)
//   8.2 — mouseReleaseEvent still calls SendMouseClickEvent(_, MBT_LEFT, true, 1)
//   8.3 — mouseMoveEvent still calls SendMouseMoveEvent(_, false)
//   8.4 — wheelEvent still calls SendMouseWheelEvent with the correct Y delta
//   8.5 — All existing mouse events produce zero CEF calls when m_inputEnabled = false
//
// Mock strategy
// -------------
// We define PreservationRecordingCefBrowserHost, which records all four
// mouse-related CEF calls: SendMouseClickEvent, SendMouseMoveEvent,
// SendMouseWheelEvent, and (for completeness) SendKeyEvent.
// This is distinct from the RecordingCefBrowserHost in
// test_input_passthrough_fix_checking.cpp, which only records key and
// mouse-move events.
//
// Validates: Requirements 3.2, 3.3, 3.4, 3.1

#include <QtTest/QtTest>
#include <QMouseEvent>
#include <QWheelEvent>
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
// Recorded call structures
// ---------------------------------------------------------------------------

struct MouseClickRecord {
    CefMouseEvent      event;
    CefBrowserHost::MouseButtonType buttonType;
    bool               mouseUp;
    int                clickCount;
};

struct MouseMoveRecord {
    CefMouseEvent event;
    bool          mouseLeave;
};

struct MouseWheelRecord {
    CefMouseEvent event;
    int           deltaX;
    int           deltaY;
};

// ---------------------------------------------------------------------------
// PreservationRecordingCefBrowserHost
// ---------------------------------------------------------------------------
// Records SendMouseClickEvent, SendMouseMoveEvent, and SendMouseWheelEvent.
// All other pure virtual methods are stubbed.

class PreservationRecordingCefBrowserHost : public CefBrowserHost {
public:
    std::vector<MouseClickRecord> mouseClickEvents;
    std::vector<MouseMoveRecord>  mouseMoveEvents;
    std::vector<MouseWheelRecord> mouseWheelEvents;

    // ---- The three methods we care about for preservation ----
    void SendMouseClickEvent(const CefMouseEvent& event,
                             MouseButtonType buttonType,
                             bool mouseUp,
                             int clickCount) override {
        mouseClickEvents.push_back({event, buttonType, mouseUp, clickCount});
    }

    void SendMouseMoveEvent(const CefMouseEvent& event,
                            bool mouseLeave) override {
        mouseMoveEvents.push_back({event, mouseLeave});
    }

    void SendMouseWheelEvent(const CefMouseEvent& event,
                             int deltaX,
                             int deltaY) override {
        mouseWheelEvents.push_back({event, deltaX, deltaY});
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
    void SendKeyEvent(const CefKeyEvent&) override {}
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

    IMPLEMENT_REFCOUNTING(PreservationRecordingCefBrowserHost);
};

// ---------------------------------------------------------------------------
// PreservationRecordingCefBrowser
// ---------------------------------------------------------------------------
// Returns our PreservationRecordingCefBrowserHost from GetHost().

class PreservationRecordingCefBrowser : public CefBrowser {
public:
    explicit PreservationRecordingCefBrowser()
        : m_host(new PreservationRecordingCefBrowserHost()) {}

    PreservationRecordingCefBrowserHost* host() { return m_host.get(); }

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
    CefRefPtr<PreservationRecordingCefBrowserHost> m_host;
    IMPLEMENT_REFCOUNTING(PreservationRecordingCefBrowser);
};

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------

static CefRefPtr<PreservationRecordingCefBrowserHost>
attachPreservationHandler(CefOSRWidget& widget, CefHandler& handler)
{
    CefRefPtr<PreservationRecordingCefBrowser> browser =
        new PreservationRecordingCefBrowser();
    handler.setBrowserForTesting(browser);
    widget.setHandler(&handler);
    return browser->host();
}

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------

class TestInputPassthroughPreservation : public QObject {
    Q_OBJECT

private slots:
    // 8.1 — mousePressEvent still calls SendMouseClickEvent(_, MBT_LEFT, false, 1)
    void testMousePress_callsSendMouseClickEvent_notUp();

    // 8.2 — mouseReleaseEvent still calls SendMouseClickEvent(_, MBT_LEFT, true, 1)
    void testMouseRelease_callsSendMouseClickEvent_up();

    // 8.3 — mouseMoveEvent still calls SendMouseMoveEvent(_, false)
    void testMouseMove_callsSendMouseMoveEvent_notLeave();

    // 8.4 — wheelEvent still calls SendMouseWheelEvent with the correct Y delta
    void testWheelEvent_callsSendMouseWheelEvent_correctDelta();

    // 8.5 — All existing mouse events produce zero CEF calls when m_inputEnabled = false
    void testInputDisabled_mousePress_noCefCalls();
    void testInputDisabled_mouseRelease_noCefCalls();
    void testInputDisabled_mouseMove_noCefCalls();
    void testInputDisabled_wheelEvent_noCefCalls();
};

// ---------------------------------------------------------------------------
// 8.1 — mousePressEvent
// ---------------------------------------------------------------------------
void TestInputPassthroughPreservation::testMousePress_callsSendMouseClickEvent_notUp()
{
    // Validates: Requirements 3.3
    CefHandler handler(nullptr);
    CefOSRWidget widget;
    CefRefPtr<PreservationRecordingCefBrowserHost> host =
        attachPreservationHandler(widget, handler);

    QMouseEvent press(QEvent::MouseButtonPress,
                      QPointF(10, 20),
                      Qt::LeftButton,
                      Qt::LeftButton,
                      Qt::NoModifier);
    QCoreApplication::sendEvent(&widget, &press);

    QCOMPARE(host->mouseClickEvents.size(), std::size_t(1));
    QCOMPARE(host->mouseClickEvents[0].buttonType, MBT_LEFT);
    QCOMPARE(host->mouseClickEvents[0].mouseUp, false);
    QCOMPARE(host->mouseClickEvents[0].clickCount, 1);
}

// ---------------------------------------------------------------------------
// 8.2 — mouseReleaseEvent
// ---------------------------------------------------------------------------
void TestInputPassthroughPreservation::testMouseRelease_callsSendMouseClickEvent_up()
{
    // Validates: Requirements 3.3
    CefHandler handler(nullptr);
    CefOSRWidget widget;
    CefRefPtr<PreservationRecordingCefBrowserHost> host =
        attachPreservationHandler(widget, handler);

    QMouseEvent release(QEvent::MouseButtonRelease,
                        QPointF(10, 20),
                        Qt::LeftButton,
                        Qt::LeftButton,
                        Qt::NoModifier);
    QCoreApplication::sendEvent(&widget, &release);

    QCOMPARE(host->mouseClickEvents.size(), std::size_t(1));
    QCOMPARE(host->mouseClickEvents[0].buttonType, MBT_LEFT);
    QCOMPARE(host->mouseClickEvents[0].mouseUp, true);
    QCOMPARE(host->mouseClickEvents[0].clickCount, 1);
}

// ---------------------------------------------------------------------------
// 8.3 — mouseMoveEvent
// ---------------------------------------------------------------------------
void TestInputPassthroughPreservation::testMouseMove_callsSendMouseMoveEvent_notLeave()
{
    // Validates: Requirements 3.2
    //
    // Note: Qt only delivers MouseMove events to a widget when either mouse
    // tracking is enabled or a button is held down.  We enable mouse tracking
    // on the widget for this test so that a no-button move is delivered.
    CefHandler handler(nullptr);
    CefOSRWidget widget;
    widget.setMouseTracking(true);
    CefRefPtr<PreservationRecordingCefBrowserHost> host =
        attachPreservationHandler(widget, handler);

    QMouseEvent move(QEvent::MouseMove,
                     QPointF(15, 25),
                     Qt::NoButton,
                     Qt::NoButton,
                     Qt::NoModifier);
    QCoreApplication::sendEvent(&widget, &move);

    QCOMPARE(host->mouseMoveEvents.size(), std::size_t(1));
    QCOMPARE(host->mouseMoveEvents[0].mouseLeave, false);
}

// ---------------------------------------------------------------------------
// 8.4 — wheelEvent
// ---------------------------------------------------------------------------
void TestInputPassthroughPreservation::testWheelEvent_callsSendMouseWheelEvent_correctDelta()
{
    // Validates: Requirements 3.4
    CefHandler handler(nullptr);
    CefOSRWidget widget;
    CefRefPtr<PreservationRecordingCefBrowserHost> host =
        attachPreservationHandler(widget, handler);

    // angleDelta().y() == 120 (one standard notch forward)
    QWheelEvent wheel(QPointF(10, 20),
                      QPointF(10, 20),
                      QPoint(0, 0),
                      QPoint(0, 120),
                      Qt::NoButton,
                      Qt::NoModifier,
                      Qt::NoScrollPhase,
                      false);
    QCoreApplication::sendEvent(&widget, &wheel);

    QCOMPARE(host->mouseWheelEvents.size(), std::size_t(1));
    QCOMPARE(host->mouseWheelEvents[0].deltaX, 0);
    QCOMPARE(host->mouseWheelEvents[0].deltaY, 120);
}

// ---------------------------------------------------------------------------
// 8.5 — m_inputEnabled = false → zero CEF calls for all mouse events
// ---------------------------------------------------------------------------
void TestInputPassthroughPreservation::testInputDisabled_mousePress_noCefCalls()
{
    // Validates: Requirements 3.1
    CefHandler handler(nullptr);
    CefOSRWidget widget;
    CefRefPtr<PreservationRecordingCefBrowserHost> host =
        attachPreservationHandler(widget, handler);
    widget.setInputEnabled(false);

    QMouseEvent press(QEvent::MouseButtonPress,
                      QPointF(10, 20),
                      Qt::LeftButton,
                      Qt::LeftButton,
                      Qt::NoModifier);
    QCoreApplication::sendEvent(&widget, &press);

    QCOMPARE(host->mouseClickEvents.size(), std::size_t(0));
}

void TestInputPassthroughPreservation::testInputDisabled_mouseRelease_noCefCalls()
{
    // Validates: Requirements 3.1
    CefHandler handler(nullptr);
    CefOSRWidget widget;
    CefRefPtr<PreservationRecordingCefBrowserHost> host =
        attachPreservationHandler(widget, handler);
    widget.setInputEnabled(false);

    QMouseEvent release(QEvent::MouseButtonRelease,
                        QPointF(10, 20),
                        Qt::LeftButton,
                        Qt::LeftButton,
                        Qt::NoModifier);
    QCoreApplication::sendEvent(&widget, &release);

    QCOMPARE(host->mouseClickEvents.size(), std::size_t(0));
}

void TestInputPassthroughPreservation::testInputDisabled_mouseMove_noCefCalls()
{
    // Validates: Requirements 3.1
    CefHandler handler(nullptr);
    CefOSRWidget widget;
    widget.setMouseTracking(true);
    CefRefPtr<PreservationRecordingCefBrowserHost> host =
        attachPreservationHandler(widget, handler);
    widget.setInputEnabled(false);

    QMouseEvent move(QEvent::MouseMove,
                     QPointF(15, 25),
                     Qt::NoButton,
                     Qt::NoButton,
                     Qt::NoModifier);
    QCoreApplication::sendEvent(&widget, &move);

    QCOMPARE(host->mouseMoveEvents.size(), std::size_t(0));
}

void TestInputPassthroughPreservation::testInputDisabled_wheelEvent_noCefCalls()
{
    // Validates: Requirements 3.1
    CefHandler handler(nullptr);
    CefOSRWidget widget;
    CefRefPtr<PreservationRecordingCefBrowserHost> host =
        attachPreservationHandler(widget, handler);
    widget.setInputEnabled(false);

    QWheelEvent wheel(QPointF(10, 20),
                      QPointF(10, 20),
                      QPoint(0, 0),
                      QPoint(0, 120),
                      Qt::NoButton,
                      Qt::NoModifier,
                      Qt::NoScrollPhase,
                      false);
    QCoreApplication::sendEvent(&widget, &wheel);

    QCOMPARE(host->mouseWheelEvents.size(), std::size_t(0));
}

QTEST_MAIN(TestInputPassthroughPreservation)
#include "test_input_passthrough_preservation.moc"
