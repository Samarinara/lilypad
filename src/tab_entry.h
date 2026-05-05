// ============================================================================
// TAB_ENTRY.H
// ============================================================================
// This file defines the TabEntry class — a QObject that holds
// all the information for ONE browser tab in our Lilypad browser.
//
// TabEntry is now a QObject with Q_PROPERTY macros so it can be
// accessed from QML. Each tab tracks:
// - Unique ID (tabId) - constant after creation
// - Page title shown in tab
// - Current URL (web address)
// - Favicon (small icon from website)
// - Loading state (spinning wheel while page loads)
// ============================================================================

#ifndef LILYPAD_TAB_ENTRY_H
#define LILYPAD_TAB_ENTRY_H

// ============================================================================
// QT INCLUDES
// ============================================================================
#include <QObject>
#include <QString>
#include <QPixmap>
#include <QVariant>

// ============================================================================
// CLASS: TabEntry
// ============================================================================
// A QObject that represents one browser tab.
// Inherits QObject for signals/slots and QML compatibility.
// ============================================================================
class TabEntry : public QObject {
    Q_OBJECT
    Q_PROPERTY(int tabId READ tabId CONSTANT)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString url READ url NOTIFY urlChanged)
    Q_PROPERTY(QPixmap favicon READ favicon NOTIFY faviconChanged)
    Q_PROPERTY(QString faviconUrl READ faviconUrl NOTIFY faviconUrlChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)

public:
    explicit TabEntry(QObject* parent = nullptr);

    // ------------------------------------------------------------------------
    // Q_PROPERTY READERS
    // ------------------------------------------------------------------------
    int tabId() const { return m_tabId; }
    QString title() const { return m_title; }
    QString url() const { return m_url; }
    QPixmap favicon() const { return m_favicon; }
    QString faviconUrl() const { return m_faviconUrl; }
    bool isLoading() const { return m_isLoading; }

    // ------------------------------------------------------------------------
    // SETTERS (called by TabManager or QML)
    // ------------------------------------------------------------------------
    Q_INVOKABLE void setTabId(int id) { m_tabId = id; }
    Q_INVOKABLE void setTitle(const QString& title) {
        if (m_title != title) {
            m_title = title;
            emit titleChanged();
        }
    }
    Q_INVOKABLE void setUrl(const QString& url) {
        if (m_url != url) {
            m_url = url;
            emit urlChanged();
        }
    }
    Q_INVOKABLE void setFavicon(const QPixmap& favicon) {
        if (m_favicon.cacheKey() != favicon.cacheKey()) {
            m_favicon = favicon;
            emit faviconChanged();
        }
    }
    Q_INVOKABLE void setFaviconUrl(const QString& faviconUrl) {
        if (m_faviconUrl != faviconUrl) {
            m_faviconUrl = faviconUrl;
            emit faviconUrlChanged();
        }
    }
    Q_INVOKABLE void setIsLoading(bool loading) {
        if (m_isLoading != loading) {
            m_isLoading = loading;
            emit loadingChanged();
        }
    }

    // ------------------------------------------------------------------------
    // METHODS
    // ------------------------------------------------------------------------
    // Note: activate/deactivate are no longer needed as QML handles visibility
    // via the StackLayout or Loader

signals:
    void titleChanged();
    void urlChanged();
    void faviconChanged();
    void faviconUrlChanged();
    void loadingChanged();

private:
    // ------------------------------------------------------------------------
    // MEMBER VARIABLES
    // ------------------------------------------------------------------------
    int m_tabId = -1;
    QString m_title;
    QString m_url;
    QPixmap m_favicon;
    QString m_faviconUrl;
    bool m_isLoading = false;
};

#endif // LILYPAD_TAB_ENTRY_H
