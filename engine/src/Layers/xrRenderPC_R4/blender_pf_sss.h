#pragma once
// COP_ENGINE_PATCH: COP-ENG-126
class CBlender_pf_sss : public IBlender
{
public:
    CBlender_pf_sss() { description.CLS = 0; }
    LPCSTR getComment() override { return "INTERNAL: Screen Space Shaders IX-Ray adapter"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }
    void Compile(CBlender_Compile& C) override;
};
