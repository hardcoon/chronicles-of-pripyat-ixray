#ifndef PF_SKYBOX_V2_SHARED
#define PF_SKYBOX_V2_SHARED
#include "common.hlsli"
// COP_ENGINE_PATCH: COP-ENG-246. Miniature coordinates and explicit instance ranges.
uniform float4x4 sb_current;
uniform float4x4 sb_previous;
uniform float4x4 sb_view;
uniform float4 sb_camera_scale;
uniform float4 sb_old_camera;
uniform float4 sb_params; // mode, first visible instance, hemi, exposure
uniform float4 sb_bounds; // world x/z minimum and inverse extent
struct SBInput
{
    float4 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};
struct SBVarying
{
    float4 position : SV_POSITION;
    float3 normal : TEXCOORD0;
    float3 miniature : TEXCOORD1;
    float4 current : TEXCOORD2;
    float4 previous : TEXCOORD3;
    float2 uv : TEXCOORD4;
    float3 extra : TEXCOORD5;
    float4 color : COLOR0;
};
SBVarying sbVertex(SBInput input, float3 miniature, float3 oldMiniature, float3 normal)
{
    SBVarying output;
    output.current = mul(sb_current, float4(miniature, 1.0));
    output.previous = mul(sb_previous, float4(oldMiniature, 1.0));
    output.position = output.current;
    output.position.xy += m_taa_jitter.xy * output.position.w;
    output.normal = normal;
    output.miniature = miniature;
    output.uv = input.uv;
    output.extra = 0.0;
    output.color = input.color.bgra;
    return output;
}
#endif
