// ============================================================================
// MAINWINDOW.H
// ============================================================================
// This file defines the MainWindow class — the main application window
// that brings together all of Lilypad's components.
//
// Think of MainWindow as the "frame" around the browser. It contains:
// - The URL bar (top) for navigation
// - The sidebar (left) showing all tabs
// - The main content area (right) showing the active web page
// ============================================================================

#ifndef LILYPAD_MAINWINDOW_H
#define LILYPAD_MAINWINDOW_H

// ============================================================================
// QT INCLUDES
// ============================================================================
#include <QMainWindow>  // Qt's main window base class
#include <QStackedWidget> // For stacking tab contents

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
class QLineEdit;     // Text input for URL
class TabManager;   // Tab manager (brain of tab system)
class TabPanel;    // Sidebar panel

// ============================================================================
// CLASS: MainWindow
// ============================================================================
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    void createInitialTab();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onUrlBarSubmit();
    void onActiveTabChanged(int oldId, int newId);
    void onUrlChanged(int tabId, const QString& url);
    void onTabViewCreated(int tabId, QWidget* container);

private:
    QLineEdit* m_urlBar;
    TabManager* m_tabManager;
    TabPanel* m_tabPanel;
    QStackedWidget* m_contentStack;
    QMap<int, int> m_tabIdToStackIndex; // Map tabId to stack index
};

#endif // LILYPAD_MAINWINDOW_H
