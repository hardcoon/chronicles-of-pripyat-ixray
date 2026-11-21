#include "common.hlsli"

uniform float4x4 sb_current;
uniform float4x4 sb_previous;

struct Input
{
    float4 position : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR0;
};
struct Output
{
    float4 position : SV_POSITION;
    float3 normal : TEXCOORD0;
    float3 miniature : TEXCOORD1;
    float4 current : TEXCOORD2;
    float4 previous : TEXCOORD3;
    float4 color : COLOR0;
};
Output main(Input input)
{
    Output output;
    output.current = mul(sb_current, input.position);
    output.previous = mul(sb_previous, input.position);
    output.position = output.current;
    output.position.xy += m_taa_jitter.xy * output.position.w;
    output.normal = input.normal;
    output.miniature = input.position.xyz;
    output.color = input.color.bgra;
    return output;
}
