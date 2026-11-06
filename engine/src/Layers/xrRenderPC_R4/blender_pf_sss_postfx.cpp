#include "stdafx.h"
#include "blender_pf_sss_postfx.h"
// COP_ENGINE_PATCH: COP-ENG-128
void CBlender_pf_sss_postfx::Compile(CBlender_Compile& C) {
    IBlender::Compile(C);
    static LPCSTR shaders[]={"pf_sss_taa","pf_sss_taa_sharp","pf_sss_motion_blur","pf_sss_post_blur","pf_sss_emissive_add","pf_sss_fog_scattering"};
    // COP_ENGINE_PATCH: COP-ENG-133 - native Shader::E has six elements; slot 4 is free because DOF is excluded.
    if(C.iElement>=SHADER_ELEMENTS_MAX) return;
    C.r_Pass("stub_fullscreen_triangle",shaders[C.iElement],FALSE,FALSE,FALSE,C.iElement==4,D3DBLEND_ONE,D3DBLEND_ONE);
    C.r_dx10Texture("pf_sss_emissive","$user$pf_sss_emissive");
    C.r_dx10Texture("pf_sss_scene",r2_RT_generic0);
    C.r_dx10Texture("pf_sss_data","$user$pf_sss_data");
    C.r_dx10Texture("pf_sss_velocity","$user$pf_sss_velocity");
    C.r_dx10Texture("s_velocity",r2_RT_velocity);
    C.r_dx10Texture("pf_sss_post_previous","$user$pf_sss_post_previous");
    C.r_dx10Texture("pf_sss_post_resolve","$user$pf_sss_post_resolve");
    C.r_dx10Texture("pf_sss_post_blur","$user$pf_sss_post_blur");
    C.r_dx10Texture("pf_sss_hud_mask",r2_RT_hud_aa_mask);
    C.r_dx10Texture("s_position",r2_RT_P);
    C.r_dx10Texture("s_normal",r2_RT_N);
    C.r_dx10Texture("s_surface",r2_RT_S);
    C.r_dx10Texture("s_diffuse",r2_RT_albedo);
    C.r_dx10Sampler("smp_nofilter");C.r_dx10Sampler("smp_rtlinear");C.r_End();
}
void CBlender_pf_sss_bloom::Compile(CBlender_Compile& C) {
    IBlender::Compile(C);
    static LPCSTR shaders[]={"pf_sss_bloom_build","pf_sss_bloom_down","pf_sss_bloom_up","pf_sss_bloom_lens","pf_sss_bloom_finish"};
    if(C.iElement>=5) return;
    C.r_Pass("stub_fullscreen_triangle",shaders[C.iElement],FALSE,FALSE,FALSE);
    C.r_dx10Texture("s_image",r2_RT_generic);
    C.r_dx10Texture("pf_sss_emissive","$user$pf_sss_emissive");
    C.r_dx10Texture("pf_sss_bloom_input",source); C.r_dx10Texture("pf_sss_bloom_base",base);
    C.r_dx10Texture("pf_sss_bloom_lens","$user$pf_sss_bloom_lens");
    C.r_dx10Texture("pf_sss_starburst","pf_sss\\fx\\starburst");
    C.r_dx10Texture("pf_sss_lens_dirt","pf_sss\\fx\\lens_dirt");
    C.r_dx10Texture("pf_sss_lens_colors","pf_sss\\fx\\lens_colors");
    C.r_dx10Texture("s_position",r2_RT_P);
    C.r_dx10Sampler("smp_nofilter");C.r_dx10Sampler("smp_rtlinear");C.r_dx10Sampler("smp_linear");C.r_End();
}

void CBlender_pf_sss_volume::Compile(CBlender_Compile& C) {
    IBlender::Compile(C);if(C.iElement>1)return;
    C.r_Pass("stub_fullscreen_triangle",C.iElement==0?"pf_sss_volumetric_blur":"pf_sss_volumetric_combine",FALSE,FALSE,FALSE);
    C.r_dx10Texture("pf_sss_volume_input",input);C.r_dx10Texture("pf_sss_volume_sun",r2_RT_generic2);
    C.r_dx10Texture("pf_sss_scene",r2_RT_generic0);
    C.r_dx10Sampler("smp_rtlinear");C.r_End();
}
