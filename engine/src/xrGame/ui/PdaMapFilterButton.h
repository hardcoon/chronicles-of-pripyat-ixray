#pragma once
#include "../../xrUI/Widgets/UICheckButton.h"
#include "../../xrUI/ui_base.h"
#include "PdaFilterLayout.h"

// COP_ENGINE_PATCH: COP-ENG-261 - native three-piece skin; its indicator never follows button width.
class CPdaMapFilterButton final : public CUICheckButton
{
    using inherited = CUICheckButton;
public:
    PdaFilterLayout::CaptionPlacement PlaceCaption(float left, float right, float kx)
    {
        CUIStatic* skin = m_background ? m_background->Get(S_Enabled) : nullptr;
        const float sourceHeight = skin ? skin->GetStaticItem()->GetTextureRect().height() : 0.f;
        const float scale = sourceHeight > 0.f ? GetHeight() / sourceHeight : 1.f;
        const auto alignment = TextItemControl()->GetTextAlignment();
        float anchor = 0.f;
        if (alignment == CGameFont::alCenter) anchor = 0.5f;
        if (alignment == CGameFont::alRight) anchor = 1.f;
        return PdaFilterLayout::PlaceCaption(GetWidth(), left, right, scale * kx, anchor);
    }

    void DrawTexture() override
    {
        // CUIStatic::Draw traverses children after DrawTexture; avoid a second unsliced background.
        if (m_background) m_background->SetCustomDraw(true);
        CUIStatic* skin = m_background ? m_background->Get(S_Current) : nullptr;
        if (!m_bTextureEnable || !skin) { inherited::DrawTexture(); return; }
        const Frect source = skin->GetStaticItem()->GetTextureRect();
        if (source.width() <= PdaFilterLayout::LeftSkinCap + PdaFilterLayout::RightSkinCap || source.height() <= 0.f)
            { inherited::DrawTexture(); return; }
        const auto parts = PdaFilterLayout::Split(GetWidth(), GetHeight(), source.height(), UI().get_current_kx());
        if (parts.middle <= 0.f) { inherited::DrawTexture(); return; }
        const Fvector2 originalPosition = skin->GetWndPos();
        const Fvector2 originalSize = skin->GetWndSize();
        const Fvector2 originalItemSize = skin->GetStaticItem()->GetSize();
        const float widths[] = {parts.left, parts.middle, parts.right};
        const float boundaries[] = {source.x1, source.x1 + parts.sourceLeft,
            source.x2 - parts.sourceRight, source.x2};
        float x = 0.f;
        for (unsigned part = 0; part < 3; ++part)
        {
            skin->SetWndPos(Fvector2().set(x, 0.f));
            skin->SetWndSize(Fvector2().set(widths[part], GetHeight()));
            skin->SetTextureRect(Frect().set(boundaries[part], source.y1, boundaries[part + 1], source.y2));
            inherited::DrawTexture();
            x += widths[part];
        }
        // Leave the borrowed native background exactly as the standard control expects it.
        skin->SetTextureRect(source);
        skin->SetWndPos(originalPosition);
        skin->SetWndSize(originalSize);
        skin->GetStaticItem()->SetSize(originalItemSize);
    }
};
