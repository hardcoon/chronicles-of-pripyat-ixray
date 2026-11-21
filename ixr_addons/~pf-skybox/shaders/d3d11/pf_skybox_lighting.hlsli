#ifndef PF_SKYBOX_V2_LIGHTING
#define PF_SKYBOX_V2_LIGHTING
#include "pf_skybox_shared.hlsli"
#include "metalic_roughness_light.hlsli"
#include "metalic_roughness_ambient.hlsli"
struct SBOutput { float4 color : SV_Target0; float4 velocity : SV_Target1; };
void sbRejectWorld(SBVarying input)
{
    // COP_ENGINE_PATCH: COP-ENG-245 COP-ENG-246. Main snapshot never aliases our DSV.
    if (s_position.Load(int3(input.position.xy, 0)).x < 1.0) discard;
}
SBOutput sbFinish(SBVarying input, float3 base, float3 worldNormal)
{
    float3 relative = input.miniature - sb_camera_scale.xyz;
    float worldDistance = length(relative) * sb_camera_scale.w;
    float fog = PushGamma(saturate(worldDistance * fog_params.w + fog_params.x));
    float3 normal = normalize(mul((float3x3)sb_view, normalize(worldNormal)));
    float3 view = normalize(mul((float3x3)sb_view, relative));
    float3 albedo = PushGamma(max(base, 0.0));
    // Legacy uses this slot as gloss intensity; PBR uses perceptual roughness.
#ifdef USE_LEGACY_LIGHT
    float surfaceRoughness = 0.0;
#else
    float surfaceRoughness = 1.0;
#endif
    float3 color = AmbientLighting(view, normal, albedo, 0.0, surfaceRoughness, sb_params.z, 0.04);
    color += DirectLight(Ldynamic_color, Ldynamic_dir.xyz, normal, view, albedo, 0.0, surfaceRoughness, 0.04);
    color *= sb_params.w;
    color = lerp(color, PushGamma(fog_color.rgb), fog);
#ifdef USE_LEGACY_LIGHT
    fog *= fog;
#endif
    if (fog >= 1.0) discard;
    SBOutput output;
    output.color = float4(color, fog);
    float2 velocity = input.current.xy / max(input.current.w, 0.0001) - input.previous.xy / max(input.previous.w, 0.0001);
    output.velocity = float4(velocity, 0.0, fog);
    return output;
}
#endif
