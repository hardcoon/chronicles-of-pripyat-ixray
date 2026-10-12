// Screen Space Shaders by ascii1457: https://www.moddb.com/mods/stalker-anomaly/addons/screen-space-shaders
// IX-Ray R4 adapter by Hardcoon. COP_ENGINE_PATCH: COP-ENG-126
#ifndef PF_SSS_COMMON_INCLUDED
#define PF_SSS_COMMON_INCLUDED
#include "common.hlsli"
#ifndef PF_SSS_CONTROL_UNIFORM
#define PF_SSS_CONTROL_UNIFORM
uniform float4 pf_sss_control;
#endif
uniform float4 pf_sss_shadow;
uniform float4 pf_sss_quality;
uniform float4 pf_sss_ao;
uniform float4 pf_sss_ao2;
uniform float4 pf_sss_weather_rain;
uniform float4 rain_params;

float pf_sss_hash12(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}
float3 pf_sss_hash33(float3 p)
{
    p = frac(p * float3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return frac((p.xxy + p.yxx) * p.zyx);
}
bool pf_sss_valid_uv(float2 uv) { return all(uv >= 0.0) && all(uv < 1.0); }
float2 pf_sss_project(float3 viewPoint, bool hud)
{
    float4 projected = hud ? mul(m_P_hud, float4(viewPoint, 1)) : mul(m_P, float4(viewPoint, 1));
    return projected.xy / max(projected.w, 0.00001) * float2(0.5, -0.5) + 0.5;
}
// Native hardware depth and HUD partition. No Anomaly packed-position reads.
float pf_sss_linear_depth(float raw)
{
    if (raw >= 0.999999 || raw <= 0.0) return 0.0;
    return raw < 0.02 ? depth_unpack.z / (raw * 50.0 - depth_unpack.w)
                      : depth_unpack.x / (raw - depth_unpack.y);
}
float pf_sss_raw_depth(float2 uv) { return s_position.SampleLevel(smp_nofilter, uv, 0).x; }
#endif
