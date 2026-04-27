#include "cef_handler.h"
#include "tab_manager.h"
#include <QImage>
#include <QPixmap>

CefHandler::CefHandler(QWindow* window, QObject* parent)
    : QObject(parent), m_window(window) {}

CefHandler::~CefHandler() {}

void CefHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    m_browser = browser;
    if (m_tabManager)
        m_tabManager->onBrowserCreated(tabId, browser);
}

void CefHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
    if (browser->IsSame(m_browser))
        m_browser = nullptr;
    if (m_tabManager)
        m_tabManager->onBeforeClose(tabId);
}

void CefHandler::OnTitleChange(CefRefPtr<CefBrowser> browser,
                                const CefString& title) {
    if (m_tabManager)
        m_tabManager->onTitleChanged(tabId,
                                     QString::fromStdString(title.ToString()));
}

void CefHandler::OnAddressChange(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  const CefString& url) {
    if (frame->IsMain() && m_tabManager)
        m_tabManager->onUrlChanged(tabId,
                                   QString::fromStdString(url.ToString()));
}

void CefHandler::OnFaviconURLChange(CefRefPtr<CefBrowser> browser,
                                     const std::vector<CefString>& icon_urls) {
    if (!m_tabManager)
        return;

    if (icon_urls.empty()) {
        m_tabManager->onFaviconChanged(tabId, QPixmap());
        return;
    }

    CefRefPtr<CefRequest> request = CefRequest::Create();
    request->SetURL(icon_urls[0]);
    request->SetMethod("GET");

    CefRefPtr<FaviconRequestClient> client = new FaviconRequestClient(this);
    CefURLRequest::Create(request, client, nullptr);
}

void CefHandler::FaviconRequestClient::OnRequestComplete(CefRefPtr<CefURLRequest> request)
{
    if (request->GetRequestStatus() == UR_SUCCESS && m_handler->m_tabManager) {
        QImage img;
        img.loadFromData(reinterpret_cast<const uchar*>(m_data.data()),
                         static_cast<int>(m_data.size()));
        QPixmap pixmap = QPixmap::fromImage(img);
        m_handler->m_tabManager->onFaviconChanged(m_handler->tabId, pixmap);
    }
}

void CefHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                     bool isLoading,
                                     bool canGoBack,
                                     bool canGoForward) {
    if (m_tabManager)
        m_tabManager->onLoadingStateChanged(tabId, isLoading);
}