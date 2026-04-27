#pragma once
#include "include/cef_client.h"
#include "include/cef_display_handler.h"
#include "include/cef_life_span_handler.h"
#include "include/cef_load_handler.h"
#include "include/cef_request.h"
#include "include/cef_urlrequest.h"
#include <QObject>
#include <QPixmap>
#include <QPointer>
#include <QWindow>
#include <vector>

class TabManager;

class CefHandler : public QObject, public CefClient,
                   public CefLifeSpanHandler,
                   public CefDisplayHandler, public CefLoadHandler {
    Q_OBJECT
public:
    explicit CefHandler(QWindow* window, QObject* parent = nullptr);
    ~CefHandler();

    int         tabId        = -1;
    TabManager* m_tabManager = nullptr;

    CefRefPtr<CefBrowser> GetBrowser() const { return m_browser; }

    void setBrowserForTesting(CefRefPtr<CefBrowser> browser) { m_browser = browser; }

    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    CefRefPtr<CefDisplayHandler>  GetDisplayHandler()  override { return this; }
    CefRefPtr<CefLoadHandler>     GetLoadHandler()     override { return this; }

    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

    void OnTitleChange(CefRefPtr<CefBrowser> browser,
                       const CefString& title) override;
    void OnAddressChange(CefRefPtr<CefBrowser> browser,
                          CefRefPtr<CefFrame> frame,
                          const CefString& url) override;
    void OnFaviconURLChange(CefRefPtr<CefBrowser> browser,
                            const std::vector<CefString>& icon_urls) override;

    void OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                              bool isLoading,
                              bool canGoBack,
                              bool canGoForward) override;

    class FaviconRequestClient : public CefURLRequestClient {
    public:
        explicit FaviconRequestClient(CefHandler* handler) : m_handler(handler) {}

        void OnRequestComplete(CefRefPtr<CefURLRequest> request) override;

        void OnDownloadData(CefRefPtr<CefURLRequest> /*request*/,
                            const void* data,
                            size_t data_length) override {
            m_data.append(static_cast<const char*>(data), data_length);
        }

        void OnUploadProgress(CefRefPtr<CefURLRequest>, int64_t, int64_t) override {}
        void OnDownloadProgress(CefRefPtr<CefURLRequest>, int64_t, int64_t) override {}
        bool GetAuthCredentials(bool, const CefString&, int, const CefString&,
                                const CefString&,
                                CefRefPtr<CefAuthCallback>) override { return false; }

    private:
        CefHandler* m_handler;
        std::string m_data;
        IMPLEMENT_REFCOUNTING(FaviconRequestClient);
    };

private:
    CefRefPtr<CefBrowser> m_browser;
    QPointer<QWindow>    m_window;

    IMPLEMENT_REFCOUNTING(CefHandler);
};