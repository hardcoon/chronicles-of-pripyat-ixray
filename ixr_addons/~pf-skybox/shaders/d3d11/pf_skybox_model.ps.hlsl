#include "pf_skybox_lighting.hlsli"
SBOutput main(SBVarying input, bool front : SV_IsFrontFace)
{
    sbRejectWorld(input);
    float4 material = s_base.Sample(smp_base, input.uv);
    clip(material.a - 0.25);
    return sbFinish(input, material.rgb, front ? input.normal : -input.normal);
}
