// Watch v0.2 56157bfd60d1: orthogonal dial UV, filtered donor grid and finite phosphor glow.
#include "common.hlsli"
#include "st_watch_display.hlsli"
// Weight indicator palette follows the dynamic HUD severity (shader CRC refresh).
float4 main(p_bumped_new I):SV_Target
{
    float3 glyphs=PushGamma(st_watch_display(I.tcdh.xy))*.6*clamp(watch_color_params.w,.25,6);
    float3 backlight=st_watch_backlight(I.tcdh.xy)*.6*clamp(game_time.w,0,30);
    return float4(glyphs+backlight,0);
}
