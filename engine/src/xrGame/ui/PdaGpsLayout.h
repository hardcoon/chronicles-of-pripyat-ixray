#pragma once
#include "../../xrUI/Widgets/UIWindow.h"

// COP_ENGINE_PATCH: COP-ENG-107 - preserve physical proportions across UI aspect ratios.
inline Frect PdaGpsIconRect(const Frect& slot, float kx)
{
    const float height = _min(slot.height(), slot.width() / kx);
    const float width = height * kx;
    const float x = slot.x1 + (slot.width() - width) * 0.5f;
    const float y = slot.y1 + (slot.height() - height) * 0.5f;
    return Frect().set(x, y, x + width, y + height);
}

inline void ScalePdaGpsControl(CUIWindow* window, float sx, float sy)
{
    // Children first: CUI3tButton size setters also resize its background.
    for (CUIWindow* child : window->GetChildWndList())
        ScalePdaGpsControl(child, sx, sy);
    const Frect rect = window->GetWndRect();
    window->SetWndRect(Frect().set(rect.x1 * sx, rect.y1 * sy, rect.x2 * sx, rect.y2 * sy));
}

inline void ScalePdaGpsDialog(CUIWindow* dialog, CUIWindow* background, float scale, float kx)
{
    const float sx = scale * kx;
    const Frect panel = background->GetWndRect();
    const float dx = panel.x1 + panel.width() * (1.f - sx) * 0.5f - panel.x1 * sx;
    const float dy = panel.y1 + panel.height() * (1.f - scale) * 0.5f - panel.y1 * scale;
    for (CUIWindow* child : dialog->GetChildWndList())
    {
        ScalePdaGpsControl(child, sx, scale);
        const Fvector2 pos = child->GetWndPos();
        child->SetWndPos(Fvector2().set(pos.x + dx, pos.y + dy));
    }
}
