// ============================================================================
// MAINWINDOW.CPP
// ============================================================================
#include "mainwindow.h"
#include "tab_manager.h"
#include "tab_panel.h"
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCloseEvent>
#include <QStackedWidget>
#include <QMap>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Lilypad");
    resize(1200, 800);

    auto* central = new QWidget(this);
    setCentralWidget(central);

    m_urlBar = new QLineEdit(central);
    m_urlBar->setPlaceholderText("Enter URL...");

    // Content stack for tab widgets
    m_contentStack = new QStackedWidget(central);

    m_tabManager = new TabManager(this);

    m_tabPanel = new TabPanel(m_tabManager, central);

    // Layout
    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_urlBar);

    auto* contentLayout = new QHBoxLayout();
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    contentLayout->addWidget(m_tabPanel);
    contentLayout->addWidget(m_contentStack, 1);
    mainLayout->addLayout(contentLayout, 1);

    // Connections
    connect(m_tabPanel, &TabPanel::newTabRequested, this, [this]() {
        m_tabManager->createTab("about:blank");
    });
    connect(m_tabPanel, &TabPanel::tabCloseRequested, m_tabManager, &TabManager::closeTab);
    connect(m_tabPanel, &TabPanel::tabSwitchRequested, m_tabManager, &TabManager::switchToTab);

    connect(m_tabManager, &TabManager::activeTabChanged, this, &MainWindow::onActiveTabChanged);
    connect(m_tabManager, &TabManager::urlChanged, this, &MainWindow::onUrlChanged);
    connect(m_tabManager, &TabManager::tabCreated, this, [this](int tabId) {
        // Tab created, but view might not be ready yet
    });
    // Connect to new signal for when container is ready
    connect(m_tabManager, &TabManager::tabViewCreated, this, &MainWindow::onTabViewCreated);

    connect(m_urlBar, &QLineEdit::returnPressed, this, &MainWindow::onUrlBarSubmit);
}

void MainWindow::createInitialTab()
{
    m_tabManager->createTab("https://polli.page");
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    m_tabManager->closeAllTabs();
    event->accept();
}

void MainWindow::onUrlBarSubmit()
{
    TabEntry* active = m_tabManager->activeTab();
    if (active && active->view) {
        active->view->load(QUrl(m_urlBar->text()));
    }
}

void MainWindow::onActiveTabChanged(int /*oldId*/, int newId)
{
    TabEntry* entry = m_tabManager->tabById(newId);
    if (!entry)
        return;

    m_urlBar->setText(entry->url);

    // Switch the content stack to this tab's container
    if (m_tabIdToStackIndex.contains(newId)) {
        m_contentStack->setCurrentIndex(m_tabIdToStackIndex[newId]);
    }
}

void MainWindow::onUrlChanged(int tabId, const QString& url)
{
    TabEntry* active = m_tabManager->activeTab();
    if (active && active->tabId == tabId) {
        m_urlBar->setText(url);
    }
}

void MainWindow::onTabViewCreated(int tabId, QWidget* container)
{
    // Add container to stack and record index
    int index = m_contentStack->addWidget(container);
    m_tabIdToStackIndex[tabId] = index;

    // If this is the active tab, switch to it
    if (m_tabManager->activeTab() && m_tabManager->activeTab()->tabId == tabId) {
        m_contentStack->setCurrentIndex(index);
    }
}
