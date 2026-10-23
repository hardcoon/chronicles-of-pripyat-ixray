// Screen Space Shaders grass wind by ascii1457; IX-Ray adapter by Hardcoon.
#ifndef PF_SSS_GRASS_WIND_INCLUDED
#define PF_SSS_GRASS_WIND_INCLUDED
#include "pf_sss_options.hlsli"
uniform float4 pf_sss_wind;
uniform float4 pf_sss_wind_anim, pf_sss_wind_anim_previous;
Texture2D pf_sss_wind_wave : register(t1); // native detail_buffer owns VS slot 0

float3 pf_sss_grass_wind(float3 pos, float height, bool previous)
{
    float4 anim = previous ? pf_sss_wind_anim_previous : pf_sss_wind_anim;
    float angle = -pf_sss_wind.x + 1.57079f;
    float2 direction = float2(cos(angle), sin(angle));
    float speed = max(PF_SSS_WIND_MIN, saturate(pf_sss_wind.y * 0.001f));
    float height_limit = saturate(height * height - 0.01f) * saturate(1.0f - height * 0.1f);
    float2 offset = -anim.xy * PF_SSS_WIND_GRASS_SPEED;
    float3 flow = pf_sss_wind_wave.SampleLevel(smp_linear, (pos.xz + offset) * 0.018f, 0).xyz;
    float2 turbulence = (flow.xy * 2.0f - 1.0f) * PF_SSS_WIND_GRASS_TURBULENCE;
    float2 push = flow.z * direction * PF_SSS_WIND_GRASS_PUSH;
    return float3(turbulence.x + push.x, flow.z * PF_SSS_WIND_GRASS_WAVE, turbulence.y + push.y) * speed * height_limit;
}
#endif
