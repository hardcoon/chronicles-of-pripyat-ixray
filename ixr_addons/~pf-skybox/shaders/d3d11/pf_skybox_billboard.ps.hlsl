#include "pf_skybox_lighting.hlsli"
SBOutput main(SBVarying input)
{
    sbRejectWorld(input);
    float4 first = s_base.Sample(smp_rtlinear, input.uv);
    float4 second = s_base.Sample(smp_rtlinear, input.extra.xy);
    float alpha = lerp(first.a, second.a, input.extra.z);
    clip(alpha - 0.25);
    float3 color = lerp(first.rgb * first.a, second.rgb * second.a, input.extra.z) / max(alpha, 0.0001);
    return sbFinish(input, color, input.normal);
}
