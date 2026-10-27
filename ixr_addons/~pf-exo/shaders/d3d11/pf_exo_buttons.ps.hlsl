#include "common.hlsli"
// The screen must stay legible head-on. No model_distort viewing-angle fade.
float4 main(p_bumped_new I):SV_Target
{
    float4 image=s_base.Sample(smp_base,I.tcdh.xy);
    return float4(detonemap(image.rgb)*1.5,saturate(image.a));
}
