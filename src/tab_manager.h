// ============================================================================
// TAB_MANAGER.H
// ============================================================================
// This file defines the TabManager class — the "brain" of Lilypad's tab system.
// TabManager orchestrates all browser tabs: creating them, closing them, switching
// between them, and handling communication between QWebEngine and our UI.
//
// Now inherits QAbstractListModel to work as a model for QML ListView.
// ============================================================================

#ifndef LILYPAD_TAB_MANAGER_H
#define LILYPAD_TAB_MANAGER_H

// ============================================================================
// QT INCLUDES
// ============================================================================
#include <QAbstractListModel>
#include <QObject>
#include <QHash>
#include <QList>
#include <QMap>
#include <QSet>
#include <QPixmap>
#include <QTimer>

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
class TabEntry;

// ============================================================================
// CLASS: TabManager
// ============================================================================
class TabManager : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int activeTabId READ activeTabId NOTIFY activeTabChanged)

public:
    // ============================================================================
    // ROLES FOR QML
    // ============================================================================
    enum Role {
        TabIdRole = Qt::UserRole + 1,
        TitleRole,
        UrlRole,
        FaviconRole,
        LoadingRole,
        TabEntryRole,
        FaviconUrlRole
    };

    // ============================================================================
    // CONSTRUCTOR
    // ============================================================================
    explicit TabManager(QObject* parent = nullptr);
    ~TabManager();

    // ============================================================================
    // QAbstractListModel INTERFACE
    // ============================================================================
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // ============================================================================
    // Q_INVOKABLE METHODS (callable from QML)
    // ============================================================================
    Q_INVOKABLE int createTab(const QString& url);
    Q_INVOKABLE void closeTab(int tabId);
    Q_INVOKABLE void switchToTab(int tabId);
    Q_INVOKABLE void closeAllTabs();
    Q_INVOKABLE TabEntry* activeTab() const;
    Q_INVOKABLE TabEntry* tabById(int tabId) const;
    Q_INVOKABLE int tabCount() const;
    Q_INVOKABLE int activeTabId() const { return m_activeTabId; }
    Q_INVOKABLE void loadUrlForActiveTab(const QString& url);

    // ============================================================================
    // CALLBACK METHODS (called by TabEntry/WebView signals)
    // ============================================================================
    Q_INVOKABLE void onTitleChanged(int tabId, const QString& title);
    Q_INVOKABLE void onFaviconChanged(int tabId, const QUrl& faviconUrl);
    Q_INVOKABLE void onLoadingStateChanged(int tabId, bool isLoading);
    Q_INVOKABLE void onUrlChanged(int tabId, const QString& url);
    Q_INVOKABLE void onBrowserCreated(int tabId);
    Q_INVOKABLE void onBeforeClose(int tabId);

signals:
    // ============================================================================
    // SIGNALS (NOTIFICATIONS TO UI)
    // ============================================================================
    void tabCreated(int tabId);
    void tabClosed(int tabId);
    void activeTabChanged(int oldTabId, int newTabId);
    void titleChanged(int tabId, const QString& title);
    void faviconChanged(int tabId, const QPixmap& pixmap);
    void loadingStateChanged(int tabId, bool isLoading);
    void urlChanged(int tabId, const QString& url);

private:
    // ============================================================================
    // PRIVATE DATA MEMBERS
    // ============================================================================
    QList<TabEntry*> m_tabs;
    QHash<int, TabEntry*> m_tabIndex;
    int m_activeTabId = -1;
    int m_nextTabId = 1;
    QSet<int> m_closingTabs;

    // ============================================================================
    // CONSTANTS
    // ============================================================================
    static constexpr int kMaxTabs = 50;

    // ============================================================================
    // PRIVATE HELPER METHODS
    // ============================================================================
    int indexOfTab(int tabId) const;
    void removeTabEntry(int tabId);
};

#endif // LILYPAD_TAB_MANAGER_H
