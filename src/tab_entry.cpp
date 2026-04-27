#include "tab_entry.h"

void TabEntry::activate()
{
    if (handler && handler->GetBrowser()) {
        handler->GetBrowser()->GetHost()->WasHidden(false);
    }
}

void TabEntry::deactivate()
{
    if (handler && handler->GetBrowser()) {
        handler->GetBrowser()->GetHost()->WasHidden(true);
    }
}