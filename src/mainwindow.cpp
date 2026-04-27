#include "mainwindow.h"
#include "tab_manager.h"
#include "tab_panel.h"
#include "tab_entry.h"
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCloseEvent>
#include <QResizeEvent>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Lilypad");
    resize(1200, 800);

    auto* central = new QWidget(this);
    setCentralWidget(central);

    m_urlBar = new QLineEdit(central);
    m_urlBar->setPlaceholderText("Enter URL...");

    m_cefWindow = new QWindow();
    m_cefWindow->setFlags(Qt::Window | Qt::FramelessWindowHint);
    m_cefWindow->resize(800, 600);

    QWidget* cefContainer = QWidget::createWindowContainer(m_cefWindow, central);
    cefContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_tabManager = new TabManager(m_cefWindow, this);

    m_tabPanel = new TabPanel(m_tabManager, central);

    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_urlBar);

    auto* contentLayout = new QHBoxLayout();
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    contentLayout->addWidget(m_tabPanel);
    contentLayout->addWidget(cefContainer, 1);
    mainLayout->addLayout(contentLayout, 1);

    connect(m_tabPanel, &TabPanel::newTabRequested, this, [this]() {
        m_tabManager->createTab("about:blank");
    });
    connect(m_tabPanel, &TabPanel::tabCloseRequested, m_tabManager, &TabManager::closeTab);
    connect(m_tabPanel, &TabPanel::tabSwitchRequested, m_tabManager, &TabManager::switchToTab);

    connect(m_tabManager, &TabManager::activeTabChanged, this, &MainWindow::onActiveTabChanged);
    connect(m_tabManager, &TabManager::urlChanged, this, &MainWindow::onUrlChanged);

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

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    TabEntry* active = m_tabManager->activeTab();
    if (active && active->handler && active->handler->GetBrowser()) {
        active->handler->GetBrowser()->GetHost()->WasResized();
    }
}

void MainWindow::onUrlBarSubmit()
{
    TabEntry* active = m_tabManager->activeTab();
    if (active && active->handler && active->handler->GetBrowser()) {
        active->handler->GetBrowser()->GetMainFrame()->LoadURL(
            m_urlBar->text().toStdString());
    }
}

void MainWindow::onActiveTabChanged(int /*oldId*/, int newId)
{
    TabEntry* entry = m_tabManager->tabById(newId);
    if (!entry)
        return;

    m_urlBar->setText(entry->url);

    if (entry->handler && entry->handler->GetBrowser()) {
        entry->handler->GetBrowser()->GetHost()->WasResized();
    }
}

void MainWindow::onUrlChanged(int tabId, const QString& url)
{
    TabEntry* active = m_tabManager->activeTab();
    if (active && active->tabId == tabId) {
        m_urlBar->setText(url);
    }
}