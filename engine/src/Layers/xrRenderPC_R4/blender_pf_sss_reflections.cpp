#include "stdafx.h"
#include "blender_pf_sss_reflections.h"
// COP_ENGINE_PATCH: COP-ENG-128 - never sample a bound output target.
void CBlender_pf_sss_reflections::Compile(CBlender_Compile& C) {
    IBlender::Compile(C);
    static LPCSTR shaders[] = {"pf_sss_ssr", "pf_sss_ssr_filter", "pf_sss_ssr_combine",
        "pf_sss_il", "pf_sss_il_filter", "pf_sss_il_combine"};
    // COP_ENGINE_PATCH: COP-ENG-138 - puddle SSR only; no IL shader pipeline.
    if(C.iElement>2)return;
    C.r_Pass("stub_fullscreen_triangle", shaders[C.iElement], FALSE, FALSE, FALSE);
    C.r_dx10Texture("s_position", r2_RT_P);
    C.r_dx10Texture("s_normal", r2_RT_N);
    C.r_dx10Texture("s_surface", r2_RT_S);
    C.r_dx10Texture("s_diffuse", r2_RT_albedo);
    C.r_dx10Texture("s_velocity", r2_RT_velocity);
    C.r_dx10Texture("s_accumulator", r2_RT_accum);
    C.r_dx10Texture("pf_sss_scene", r2_RT_generic0);
    C.r_dx10Texture("pf_sss_data", "$user$pf_sss_data");
    C.r_dx10Texture("pf_sss_ssr_current", "$user$pf_sss_ssr");
    C.r_dx10Texture("pf_sss_ssr_previous", "$user$pf_sss_ssr_previous");
    C.r_dx10Texture("pf_sss_ssr_filtered", "$user$pf_sss_ssr_filter");
    C.r_dx10Texture("pf_sss_il_current", "$user$pf_sss_il");
    C.r_dx10Texture("pf_sss_il_previous", "$user$pf_sss_il_previous");
    C.r_dx10Texture("pf_sss_il_filtered", "$user$pf_sss_il_filter");
    C.r_dx10Texture("sky_s0", r2_T_sky0);
    C.r_dx10Texture("sky_s1", r2_T_sky1);
    C.r_dx10Texture("env_s0", r2_T_envs0);
    C.r_dx10Texture("env_s1", r2_T_envs1);
    C.r_dx10Sampler("smp_nofilter");
    C.r_dx10Sampler("smp_rtlinear");
    C.r_dx10Sampler("smp_linear");
    C.r_End();
}

void CBlender_pf_sss_water::Compile(CBlender_Compile& C) {
    IBlender::Compile(C);if(C.iElement>1)return;
    C.r_Pass("stub_fullscreen_triangle",C.iElement==0?"pf_sss_water_down":"pf_sss_water_blur",FALSE,FALSE,FALSE);
    C.r_dx10Texture("pf_sss_water_input",input);C.r_dx10Texture("s_position",r2_RT_P);
    C.r_dx10Sampler("smp_nofilter");C.r_dx10Sampler("smp_rtlinear");C.r_End();
}
