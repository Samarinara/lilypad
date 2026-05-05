// ============================================================================
// TAB_MANAGER.CPP
// ============================================================================
// This file contains the implementation of TabManager.
// Now implements QAbstractListModel for QML ListView compatibility.
// Note: WebEngineView creation is now done in QML, not C++.
// ============================================================================

#include "tab_manager.h"
#include "tab_entry.h"
#include <QDebug>
#include <QUrl>

// ============================================================================
// CONSTRUCTOR
// ============================================================================
TabManager::TabManager(QObject* parent)
    : QAbstractListModel(parent)
{
}

// ============================================================================
// DESTRUCTOR
// ============================================================================
TabManager::~TabManager()
{
    for (TabEntry* entry : m_tabs) {
        delete entry;
    }
    m_tabs.clear();
}

// ============================================================================
// QABSTRACT_LIST_MODEL INTERFACE
// ============================================================================
int TabManager::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_tabs.size();
}

QVariant TabManager::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_tabs.size())
        return QVariant();

    TabEntry* entry = m_tabs.at(index.row());
    if (!entry)
        return QVariant();

    switch (role) {
    case TabIdRole:
        return entry->tabId();
    case TitleRole:
        return entry->title();
    case UrlRole:
        return entry->url();
    case FaviconRole:
        return QVariant::fromValue(entry->favicon());
    case LoadingRole:
        return entry->isLoading();
    case TabEntryRole:
        return QVariant::fromValue(entry);
    case FaviconUrlRole:
        return entry->faviconUrl();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> TabManager::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TabIdRole] = "tabId";
    roles[TitleRole] = "title";
    roles[UrlRole] = "url";
    roles[FaviconRole] = "favicon";
    roles[LoadingRole] = "isLoading";
    roles[TabEntryRole] = "tabEntry";
    roles[FaviconUrlRole] = "faviconUrl";
    return roles;
}

// ============================================================================
// PUBLIC API - Tab Operations
// ============================================================================
int TabManager::createTab(const QString& url)
{
    qDebug() << "Creating new tab with URL:" << url;
    if (m_tabs.size() >= kMaxTabs) {
        qDebug() << "Tab limit reached (" << kMaxTabs << "), cannot create new tab";
        return -1;
    }

    beginInsertRows(QModelIndex(), m_tabs.size(), m_tabs.size());

    TabEntry* entry = new TabEntry(this);
    int tabId = m_nextTabId++;
    entry->setTabId(tabId);

    // Connect TabEntry signals to emit dataChanged for QML model updates
    connect(entry, &TabEntry::urlChanged, this, [this, tabId]() {
        int idx = indexOfTab(tabId);
        if (idx >= 0) {
            QModelIndex modelIndex = createIndex(idx, 0);
            emit dataChanged(modelIndex, modelIndex, {UrlRole});
        }
    });

    connect(entry, &TabEntry::titleChanged, this, [this, tabId]() {
        int idx = indexOfTab(tabId);
        if (idx >= 0) {
            QModelIndex modelIndex = createIndex(idx, 0);
            emit dataChanged(modelIndex, modelIndex, {TitleRole});
        }
    });

    connect(entry, &TabEntry::faviconChanged, this, [this, tabId]() {
        int idx = indexOfTab(tabId);
        if (idx >= 0) {
            QModelIndex modelIndex = createIndex(idx, 0);
            emit dataChanged(modelIndex, modelIndex, {FaviconRole});
        }
    });

    connect(entry, &TabEntry::loadingChanged, this, [this, tabId]() {
        int idx = indexOfTab(tabId);
        if (idx >= 0) {
            QModelIndex modelIndex = createIndex(idx, 0);
            emit dataChanged(modelIndex, modelIndex, {LoadingRole});
        }
    });

    entry->setUrl(url);

    m_tabs.append(entry);
    m_tabIndex[tabId] = entry;
    endInsertRows();

    qDebug() << "Tab created with ID:" << tabId << "Total tabs:" << m_tabs.size();

    // Emit signal so QML can create WebEngineView
    emit tabCreated(tabId);

    switchToTab(tabId);
    return tabId;
}

void TabManager::closeTab(int tabId)
{
    int idx = indexOfTab(tabId);
    if (idx < 0)
        return;

    if (m_tabs.size() == 1) {
        createTab("about:blank");
    }

    if (tabId == m_activeTabId) {
        int currentIdx = indexOfTab(tabId);
        int successorIdx = (currentIdx > 0) ? currentIdx - 1 : 0;
        TabEntry* successor = m_tabs[successorIdx];
        if (successor->tabId() == tabId) {
            successorIdx = (successorIdx + 1 < m_tabs.size()) ? successorIdx + 1 : successorIdx - 1;
            successor = m_tabs[successorIdx];
        }
        switchToTab(successor->tabId());
    }

    m_closingTabs.insert(tabId);
    onBeforeClose(tabId);
}

void TabManager::switchToTab(int tabId)
{
    TabEntry* entry = tabById(tabId);
    if (!entry || tabId == m_activeTabId)
        return;

    int oldTabId = m_activeTabId;
    m_activeTabId = tabId;

    emit activeTabChanged(oldTabId, tabId);
}

void TabManager::closeAllTabs()
{
    QList<int> ids;
    for (TabEntry* e : m_tabs)
        ids.append(e->tabId());

    for (int id : ids) {
        removeTabEntry(id);
    }
}

// ============================================================================
// ACCESSORS
// ============================================================================
TabEntry* TabManager::activeTab() const
{
    return tabById(m_activeTabId);
}

TabEntry* TabManager::tabById(int tabId) const
{
    return m_tabIndex.value(tabId, nullptr);
}

int TabManager::tabCount() const
{
    return m_tabs.size();
}

void TabManager::loadUrlForActiveTab(const QString& url)
{
    TabEntry* entry = activeTab();
    if (entry) {
        entry->setUrl(url);
    } else {
        qDebug() << "No active tab found!";
    }
}

// ============================================================================
// CALLBACK METHODS (called from QML or WebEngineView signals)
// ============================================================================
void TabManager::onTitleChanged(int tabId, const QString& title)
{
    if (m_closingTabs.contains(tabId))
        return;
    TabEntry* entry = tabById(tabId);
    if (!entry)
        return;
    entry->setTitle(title);
}

void TabManager::onFaviconChanged(int tabId, const QUrl& faviconUrl)
{
    if (m_closingTabs.contains(tabId))
        return;
    TabEntry* entry = tabById(tabId);
    if (!entry)
        return;
    // Store the favicon URL - QML will handle loading the icon
    entry->setFaviconUrl(faviconUrl.toString());
}

void TabManager::onLoadingStateChanged(int tabId, bool isLoading)
{
    if (m_closingTabs.contains(tabId))
        return;
    TabEntry* entry = tabById(tabId);
    if (!entry)
        return;
    entry->setIsLoading(isLoading);
}

void TabManager::onUrlChanged(int tabId, const QString& url)
{
    if (m_closingTabs.contains(tabId))
        return;
    TabEntry* entry = tabById(tabId);
    if (!entry)
        return;

    // Only update if URL actually changed (TabEntry::setUrl checks this)
    // The dataChanged signal will be emitted via the connection in createTab()
    entry->setUrl(url);
}

void TabManager::onBeforeClose(int tabId)
{
    m_closingTabs.remove(tabId);
    removeTabEntry(tabId);
}

void TabManager::onBrowserCreated(int tabId)
{
    // No-op in QML version
}

// ============================================================================
// PRIVATE HELPERS
// ============================================================================
int TabManager::indexOfTab(int tabId) const
{
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs[i]->tabId() == tabId)
            return i;
    }
    return -1;
}

void TabManager::removeTabEntry(int tabId)
{
    int idx = indexOfTab(tabId);
    if (idx < 0)
        return;

    beginRemoveRows(QModelIndex(), idx, idx);
    TabEntry* entry = m_tabs.takeAt(idx);
    m_tabIndex.remove(tabId);

    if (entry) {
        delete entry;
    }
    endRemoveRows();

    emit tabClosed(tabId);
}
