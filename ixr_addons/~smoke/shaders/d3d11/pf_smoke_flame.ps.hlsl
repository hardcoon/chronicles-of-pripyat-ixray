#include "common.hlsli"

float4 main(p_bumped_new I) : SV_Target
{
    float2 uv = I.tcdh.xy;
    float height = 1.0f - saturate(uv.y);
    float width = 0.43f * (1.0f - height);
    float edge = saturate((width - abs(uv.x - 0.5f)) * 35.0f);
    float alpha = edge * saturate(height * 18.0f);
    clip(alpha - 0.015f);
    float core = saturate((width * 0.65f - abs(uv.x - 0.5f)) * 24.0f);
    float3 color = lerp(float3(1.0f, 0.19f, 0.015f), float3(1.0f, 0.82f, 0.25f), core);
    color = lerp(float3(0.15f, 0.3f, 1.0f), color, saturate(height * 8.0f));
    float flicker = 0.9f + 0.1f * sin(timers.x * 43.0f) * sin(timers.x * 71.0f);
    return float4(color * (2.5f * flicker), alpha);
}
