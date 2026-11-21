#include "pf_skybox_lighting.hlsli"
Texture2D sb_grass;
Texture2D sb_earth;
Texture2D sb_asphalt;
Texture2D sb_normal;
SBOutput main(SBVarying input)
{
    sbRejectWorld(input);
    if (dot(input.normal, sb_camera_scale.xyz - input.miniature) <= 0.0) discard;
    float2 world = input.miniature.xz * sb_camera_scale.w;
    float2 uv = (world - sb_bounds.xy) * sb_bounds.zw;
    uv.y = 1.0 - uv.y; // DDS row zero is north, unlike the WebGL preview's flipped upload.
    float3 base = s_base.Sample(smp_rtlinear, uv).rgb;
    float kind = floor(input.color.a * 255.0 + 0.5);
    float2 detailUV = world / 16.0;
    float3 detail = sb_grass.Sample(smp_base, detailUV).rgb;
    if (kind > 1.5 && kind < 3.5)
    {
        detail = sb_earth.Sample(smp_base, detailUV).rgb;
    }
    if (kind > 4.5)
    {
        detail = sb_asphalt.Sample(smp_base, detailUV).rgb;
    }
    float3 normal = input.normal;
    if (kind < 3.5)
    {
        float4 bump = sb_normal.Sample(smp_base, detailUV);
        normal = normalize(normal + float3(bump.a * 2.0 - 1.0, 0.0, bump.g * 2.0 - 1.0) * 0.22);
    }
    // Match native deffer_impl: atlas albedo multiplied by the authored detail * 2.
    // Keep the detail's tint; normalizing by its RGB mean turns olive soil grey.
    if (kind < 3.5 || kind > 4.5) base *= detail * 2.0;
    return sbFinish(input, base, normal);
}
