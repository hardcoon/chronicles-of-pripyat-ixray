#include "common.hlsli"
Texture2D s_pdascreen;
SamplerState smp_pda;
float4 pf_pda_params;
struct Input { float2 uv : TEXCOORD0; float4 color : COLOR0; };
float4 main(Input input) : SV_Target
{
    // COP_ENGINE_PATCH: COP-ENG-216 - keep the full native/Lua UI and its cursor together.
    // Remap the donor mesh atlas, then enlarge content by 1024/996 (about 2.8%).
    float2 uv = (input.uv - float2(0.111332, 0.047251)) / float2(0.777198, 0.915997);
    uv = float2(14.0 / 1024.0, 10.5 / 768.0) + saturate(uv) * (996.0 / 1024.0);
    float3 screen = s_pdascreen.Sample(smp_pda, uv).rgb;
    // COP_ENGINE_PATCH: COP-ENG-219 - power fades the opaque display from black.
    return float4(detonemap(saturate(screen) * saturate(pf_pda_params.x)), 1.f);
}
