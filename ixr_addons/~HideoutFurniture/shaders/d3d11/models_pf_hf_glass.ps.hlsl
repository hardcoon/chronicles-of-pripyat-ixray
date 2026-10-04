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
    float4 pane = s_base.Sample(smp_base, I.tc0);
    float3 normal = normalize(I.N);
    float hemi = saturate(normal.y * 0.5 + 0.5);
    float sun = saturate(dot(normal, -L_sun_dir_w.xyz));
    pane.rgb *= max(L_ambient.xyz + L_hemi_color.xyz * hemi + L_sun_color.xyz * sun, 0.05);
    float distanceToEye = length(I.P - eye_position);
    float fog = saturate(distanceToEye * fog_params.w + fog_params.x);
    pane.rgb = lerp(pane.rgb, fog_color.xyz, fog);
    return pane;
}
