#pragma once
// COP_ENGINE_PATCH: COP-ENG-128
class CBlender_pf_sss_reflections : public IBlender {
public:
    CBlender_pf_sss_reflections() { description.CLS = 0; }
    LPCSTR getComment() override { return "INTERNAL: SSS reflections and indirect light"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }
    void Compile(CBlender_Compile& C) override;
};

// COP_ENGINE_PATCH: COP-ENG-128 - water colors retain the original immutable HDR scene.
class CBlender_pf_sss_water : public IBlender {
    LPCSTR input;
public:
    explicit CBlender_pf_sss_water(LPCSTR source):input(source){description.CLS=0;}
    LPCSTR getComment() override{return "INTERNAL: SSS water reflection color filter";}
    BOOL canBeDetailed() override{return FALSE;}
    BOOL canBeLMAPped() override{return FALSE;}
    void Compile(CBlender_Compile& C) override;
};
