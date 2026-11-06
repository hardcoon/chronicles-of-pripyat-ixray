#include "stdafx.h"
#include "blender_pf_sss.h"

// COP_ENGINE_PATCH: COP-ENG-126 - bind native G-buffer; do not emulate Anomaly s_position packing.
void CBlender_pf_sss::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);
    C.r_Pass("stub_fullscreen_triangle", C.iElement == 0 ? "pf_sss_ao" : "pf_sss_ao_filter", FALSE, FALSE, FALSE);
    C.r_dx10Texture("s_position", r2_RT_P);
    C.r_dx10Texture("s_normal", r2_RT_N);
    C.r_dx10Texture("s_surface", r2_RT_S);
    C.r_dx10Texture("s_diffuse", r2_RT_albedo);
    C.r_dx10Texture("s_velocity", r2_RT_velocity);
    C.r_dx10Texture("pf_sss_data", "$user$pf_sss_data");
    C.r_dx10Texture("pf_sss_previous", "$user$pf_sss_ao_previous");
    C.r_dx10Texture("pf_sss_current", "$user$pf_sss_ao_current");
    C.r_dx10Sampler("smp_nofilter");
    C.r_dx10Sampler("smp_rtlinear");
    C.r_End();
}
