#include "common.hlsli"
#include "pf_exo_display.hlsli"
float4 main(p_bumped_new I):SV_Target
{
    return float4(pf_exo_display(pf_exo_display_uv(I.tcdh.xy)),0.0);
}
