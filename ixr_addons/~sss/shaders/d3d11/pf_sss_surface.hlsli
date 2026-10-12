// SSS by ascii1457; native IX-Ray material/depth adapter by Hardcoon.
#ifndef PF_SSS_SURFACE_INCLUDED
#define PF_SSS_SURFACE_INCLUDED
#include "pf_sss_common.hlsli"
#include "pf_sss_options.hlsli"
Texture2D pf_sss_data;
Texture2D pf_sss_scene;
uniform float4 pf_sss_history;
struct PFSSSInput { float4 hpos : SV_POSITION; float2 uv : TEXCOORD0; };
IXrayGbuffer pf_sss_unpack(float2 uv) {
    IXrayGbuffer o;
    float4 normal = s_normal.SampleLevel(smp_nofilter, uv, 0);
    float4 material = s_surface.SampleLevel(smp_nofilter, uv, 0);
    float4 color = s_diffuse.SampleLevel(smp_nofilter, uv, 0);
    o.Depth = pf_sss_raw_depth(uv);
    float2 unjitterUV = uv - m_taa_jitter.xy * float2(.5, -.5);
    o.PointReal = GbufferGetPointRealUnjitter(unjitterUV, o.Depth);
    o.Point = o.PointReal; o.PointHud = o.PointReal;
    o.ViewDist = max(length(o.PointReal), .00001);
    o.View = o.PointReal / o.ViewDist;
    o.Normal = NormalDecode(normal.xy);
    o.Roughness = normal.z; o.SnowMask = normal.w;
    o.Hemi = material.y; o.Metalness = material.x;
    o.Color = PushGamma(color.xyz); o.SSS = color.w;
    o.AO = PushGamma(material.z); o.F0 = .002 + .028 * material.w;
    return o;
}
float pf_sss_gloss(IXrayGbuffer o) {
#ifdef USE_LEGACY_LIGHT
    return saturate(o.Roughness);
#else
    return saturate(1.0 - o.Roughness);
#endif
}
float3 pf_sss_sky(float3 viewRay) {
    float3 direction = mul((float3x3)m_invV, viewRay);
    float2 rotation; sincos(L_sky_color.w, rotation.x, rotation.y);
    direction.xz = float2(direction.x * rotation.y - direction.z * rotation.x,
        direction.x * rotation.x + direction.z * rotation.y);
#ifndef USE_FULL_SKY_SPHERE
    direction.y = direction.y * 2.0 - 1.0;
#endif
    float3 color = lerp(sky_s0.SampleLevel(smp_rtlinear, direction, 0).xyz,
        sky_s1.SampleLevel(smp_rtlinear, direction, 0).xyz, L_ambient.w);
#ifdef USE_BGRA_SKYCOLOR
    color *= L_sky_color.zyx;
#else
    color *= L_sky_color.xyz;
#endif
    return PushGamma(color);
}
// Original SSS 23.5 quality table and perspective-correct UV ray interpolation.
static const int2 pf_sss_ssr_steps[6] = {int2(8,72),int2(16,36),int2(24,18),int2(32,5),int2(48,1),int2(64,1)};
float4 pf_sss_trace(float3 origin, float3 direction, float2 uv, int quality, float noise, bool hud) {
    int2 q = pf_sss_ssr_steps[clamp(quality,0,5)];
    float maxLength = 150.0;
    // Clip rays before the eye plane to keep projection finite.
    if (direction.z < 0) maxLength = min(maxLength, (origin.z - .05) / -direction.z);
    if (maxLength <= 0) return 0;
    float3 endpoint = origin + direction * maxLength;
    float2 start = pf_sss_project(origin, hud), end = pf_sss_project(endpoint, hud);
    float2 stepUV = (end - start) / q.x;
    float total = max(length(end-start), .00001);
    float2 pos = start + stepUV * (1.0 - noise), behind = 0;
    float skyY = 0, hitDepth = 0;
    float originDepth = pf_sss_linear_depth(pf_sss_raw_depth(uv));
    [loop] for (int i=0; i<q.x; ++i) {
        if (pos.y < 0 || pos.y >= 1) return 0;
        if (pos.x < 0 || pos.x >= 1) { pos-=stepUV; stepUV.x=-stepUV.x; pos+=stepUV; }
        float fraction = saturate(length(pos-start) / total);
        float rayDepth = origin.z*endpoint.z / max(lerp(endpoint.z,origin.z,fraction),.00001);
        float raw = pf_sss_raw_depth(pos), depth = pf_sss_linear_depth(raw);
        float difference = rayDepth-depth;
        bool world = raw >= .02 && depth > 1.3;
        if (world && difference > 0) {
            if (difference <= q.y) return float4(pos,depth,0);
            if (quality > 2) {
                float2 halfPos = pos-stepUV*.5;
                float halfDepth = pf_sss_linear_depth(pf_sss_raw_depth(halfPos));
                float halfFraction = saturate(length(halfPos-start)/total);
                float halfRay = origin.z*endpoint.z/max(lerp(endpoint.z,origin.z,halfFraction),.00001);
                if (halfDepth>1.3 && abs(halfRay-halfDepth)<=1.25) return float4(halfPos,halfDepth,0);
            }
        } else {
            if (depth<=0) skyY=pos.y;
            behind = (world && originDepth-2.0<depth) ? pos : 0;
        }
        hitDepth=depth; pos+=stepUV;
    }
    return float4(behind,hitDepth,skyY);
}
#endif
