#pragma once
#include "../xrRender/blenders/Blender.h"
// COP_ENGINE_PATCH: COP-ENG-128
class CBlender_pf_sss_postfx : public IBlender {
public:
    CBlender_pf_sss_postfx() { description.CLS=0; }
    LPCSTR getComment() override { return "INTERNAL: SSS postprocessing"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }
    void Compile(CBlender_Compile& C) override;
};
class CBlender_pf_sss_bloom : public IBlender {
    LPCSTR source, base;
public:
    CBlender_pf_sss_bloom(LPCSTR input, LPCSTR parent) : source(input), base(parent) { description.CLS=0; }
    LPCSTR getComment() override { return "INTERNAL: SSS bloom pyramid"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }
    void Compile(CBlender_Compile& C) override;
};

class CBlender_pf_sss_volume:public IBlender {
    LPCSTR input;
public:
    CBlender_pf_sss_volume(LPCSTR source):input(source){description.CLS=0;}
    LPCSTR getComment() override{return "INTERNAL: SSS volumetric filtering";}
    BOOL canBeDetailed() override{return FALSE;}
    BOOL canBeLMAPped() override{return FALSE;}
    void Compile(CBlender_Compile& C) override;
};
