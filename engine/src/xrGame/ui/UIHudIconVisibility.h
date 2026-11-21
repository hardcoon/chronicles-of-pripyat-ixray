#pragma once

#include "../../xrUI/Widgets/UIWindow.h"

// COP_ENGINE_PATCH: COP-ENG-233 - stack-only render mask; never changes the underlying conditions.
class CHudIconVisibility
{
    struct SEntry { CUIWindow* window; bool shown; };
    SEntry m_entries[20] = {};
    size_t m_count = 0;
public:
    template<size_t Count>
    CHudIconVisibility(CUIWindow* const (&windows)[Count], bool visible)
    {
        static_assert(Count <= 20);
        if (visible)
            return;
        for (CUIWindow* window : windows)
        {
            if (!window)
                continue;
            m_entries[m_count++] = {window, window->IsShown()};
            window->Show(false);
        }
    }
    ~CHudIconVisibility()
    {
        // Reverse order also restores repeated pointers correctly.
        while (m_count)
        {
            const SEntry& entry = m_entries[--m_count];
            entry.window->Show(entry.shown);
        }
    }
    CHudIconVisibility(const CHudIconVisibility&) = delete;
    CHudIconVisibility& operator=(const CHudIconVisibility&) = delete;
};
