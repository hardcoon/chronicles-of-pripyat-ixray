#include "common.hlsli"
uniform float4 screen_res;
struct PreviewInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float depth : TEXCOORD1;
    float4 color : COLOR0;
};
float4 main(PreviewInput input) : SV_TARGET
{
    // Same scene-depth reconstruction as IX-Ray's native debug_draw shader.
    float depth = s_position.SampleLevel(smp_nofilter, input.position.xy * screen_res.zw, 0).x;
    depth = depth_unpack.x / (depth - depth_unpack.y);
    clip(depth - input.depth + .001);
    float4 color = s_base.Sample(smp_base, input.uv) * input.color;
    color.rgb = PushGamma(color.rgb);
    return color;
}
