#pragma once
#include <QString>
#include <QPixmap>
#include "cef_handler.h"

struct TabEntry {
    int           tabId     = -1;
    QString       title;
    QString       url;
    QPixmap       favicon;
    bool          isLoading = false;

    CefHandler*   handler   = nullptr;
    QWidget*     container = nullptr;

    void activate();
    void deactivate();
};
