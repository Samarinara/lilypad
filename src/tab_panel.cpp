#include "tab_panel.h"
#include "tab_manager.h"
#include "tab_entry_widget.h"
#include "tab_entry.h"

#include <QVBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QWidget>

TabPanel::TabPanel(TabManager* manager, QWidget* parent)
    : QWidget(parent)
    , m_tabManager(manager)
{
    setFixedWidth(kPanelWidth);

    // Outer layout for the panel
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    // Scroll area containing the tab list
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* listContainer = new QWidget(scrollArea);
    m_listLayout = new QVBoxLayout(listContainer);
    m_listLayout->setAlignment(Qt::AlignTop);
    m_listLayout->setSpacing(0);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    listContainer->setLayout(m_listLayout);

    scrollArea->setWidget(listContainer);

    // New Tab button at the bottom
    m_newTabBtn = new QPushButton(QStringLiteral("+ New Tab"), this);
    connect(m_newTabBtn, &QPushButton::clicked, this, [this]() {
        emit newTabRequested();
    });

    outerLayout->addWidget(scrollArea, 1);
    outerLayout->addWidget(m_newTabBtn);
    setLayout(outerLayout);

    // Connect TabManager signals
    connect(m_tabManager, &TabManager::tabCreated,          this, &TabPanel::onTabCreated);
    connect(m_tabManager, &TabManager::tabClosed,           this, &TabPanel::onTabClosed);
    connect(m_tabManager, &TabManager::activeTabChanged,    this, &TabPanel::onActiveTabChanged);
    connect(m_tabManager, &TabManager::titleChanged,        this, &TabPanel::onTitleChanged);
    connect(m_tabManager, &TabManager::faviconChanged,      this, &TabPanel::onFaviconChanged);
    connect(m_tabManager, &TabManager::loadingStateChanged, this, &TabPanel::onLoadingStateChanged);

    updateNewTabButton();
}

// ---------------------------------------------------------------------------
// Public accessor
// ---------------------------------------------------------------------------

QList<int> TabPanel::tabOrder() const
{
    QList<int> order;
    const int count = m_listLayout->count();
    for (int i = 0; i < count; ++i) {
        QLayoutItem* item = m_listLayout->itemAt(i);
        if (!item)
            continue;
        QWidget* w = item->widget();
        if (!w)
            continue;
        auto* tew = qobject_cast<TabEntryWidget*>(w);
        if (tew)
            order.append(tew->tabId());
    }
    return order;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void TabPanel::addTabWidget(int tabId)
{
    TabEntry* entry = m_tabManager->tabById(tabId);
    if (!entry)
        return;

    auto* w = new TabEntryWidget(tabId, this);
    w->setTitle(entry->title);
    w->setUrl(entry->url);
    w->setFavicon(entry->favicon);
    w->setLoading(entry->isLoading);

    connect(w, &TabEntryWidget::clicked,      this, [this, tabId]() {
        emit tabSwitchRequested(tabId);
    });
    connect(w, &TabEntryWidget::closeClicked, this, [this, tabId]() {
        emit tabCloseRequested(tabId);
    });

    m_listLayout->addWidget(w);
    m_widgets.insert(tabId, w);

    updateNewTabButton();
}

void TabPanel::removeTabWidget(int tabId)
{
    auto it = m_widgets.find(tabId);
    if (it == m_widgets.end())
        return;

    TabEntryWidget* w = it.value();
    m_listLayout->removeWidget(w);
    m_widgets.erase(it);
    delete w;

    updateNewTabButton();
}

void TabPanel::setActiveTab(int tabId)
{
    // Deactivate all
    for (auto* w : m_widgets)
        w->setActive(false);

    // Activate the target
    auto it = m_widgets.find(tabId);
    if (it != m_widgets.end())
        it.value()->setActive(true);
}

void TabPanel::updateNewTabButton()
{
    m_newTabBtn->setEnabled(m_tabManager->tabCount() < 50);
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void TabPanel::onTabCreated(int tabId)
{
    addTabWidget(tabId);
}

void TabPanel::onTabClosed(int tabId)
{
    removeTabWidget(tabId);
}

void TabPanel::onActiveTabChanged(int /*oldId*/, int newId)
{
    setActiveTab(newId);
}

void TabPanel::onTitleChanged(int tabId, const QString& title)
{
    auto it = m_widgets.find(tabId);
    if (it != m_widgets.end())
        it.value()->setTitle(title);
}

void TabPanel::onFaviconChanged(int tabId, const QPixmap& pixmap)
{
    auto it = m_widgets.find(tabId);
    if (it != m_widgets.end())
        it.value()->setFavicon(pixmap);
}

void TabPanel::onLoadingStateChanged(int tabId, bool isLoading)
{
    auto it = m_widgets.find(tabId);
    if (it != m_widgets.end())
        it.value()->setLoading(isLoading);
}
