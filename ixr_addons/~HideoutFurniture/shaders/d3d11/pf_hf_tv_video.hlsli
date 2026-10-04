#include "common.hlsli"
struct vf
{
    float4 hpos : SV_Position;
    float2 tc0 : TEXCOORD0;
    float3 T : TEXCOORD1;
    float3 B : TEXCOORD2;
    float3 N : TEXCOORD3;
    float3 P : TEXCOORD4;
};
float4 main(vf I) : SV_Target
{
    // Fit the complete 449:253 broadcast into each physical screen, preserving its aspect.
    float2 uv = (I.tc0 - PF_HF_TV_UV_MIN) / (PF_HF_TV_UV_MAX - PF_HF_TV_UV_MIN);
    uv.y = (uv.y - 0.5) * ((449.0 / 253.0) / PF_HF_TV_ASPECT) + 0.5;
    float3 rgb = 0;
    if (uv.y >= 0 && uv.y <= 1)
    {
        // Same limited-range BT.601 conversion as IX-Ray's hud_movie/yuv2rgb.
        float3 yuv = s_base.Sample(smp_base, uv).xyz;
        rgb = saturate(1.16406 * yuv.z +
            float3(0, -0.390625, 2.01562) * yuv.y +
            float3(1.59765, -0.8125, 0) * yuv.x +
            float3(-0.86961, 0.53076, -1.0786));
    }
    float fog = saturate(length(I.P - eye_position) * fog_params.w + fog_params.x);
    return float4(lerp(rgb, fog_color.xyz, fog), 1);
}
