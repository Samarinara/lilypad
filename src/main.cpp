// ============================================================================
// MAIN.CPP
// ============================================================================
// Entry point for the Lilypad browser application.
//
// This file:
// 1. Initialize Qt
// 2. Create the main window
// 3. Run the event loop
// 4. Clean up on exit
// ============================================================================

#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication qapp(argc, argv);

    MainWindow w;
    w.show();
    w.createInitialTab();

    return qapp.exec();
}
