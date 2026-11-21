#include "common.hlsli"

uniform float4 sb_camera_scale;
uniform float4 sb_sun_direction;
uniform float4 sb_sun_color;
uniform float4 sb_ambient;

struct Input
{
    float4 position : SV_POSITION;
    float3 normal : TEXCOORD0;
    float3 miniature : TEXCOORD1;
    float4 current : TEXCOORD2;
    float4 previous : TEXCOORD3;
    float4 color : COLOR0;
};
struct Output
{
    float4 color : SV_Target0;
    float4 velocity : SV_Target1;
};
Output main(Input input)
{
    // COP_ENGINE_PATCH: COP-ENG-245 - main depth is a separate native snapshot.
    // No distant-scene contribution may enter an opaque level/HUD pixel.
    float levelDepth = s_position.Load(int3(input.position.xy, 0)).x;
    if (levelDepth < 1.0) discard;
    // PFSB v1 alpha=0 opts terrain into top-side rendering. Legacy alpha=255
    // meshes keep their original two-sided behavior. Normals and camera share
    // miniature world space; this also handles views below the native port.
    if (input.color.a < 0.5 && dot(input.normal, sb_camera_scale.xyz - input.miniature) <= 0.0)
        discard;
    Output output;
    float sunlight = saturate(dot(normalize(input.normal), -sb_sun_direction.xyz));
    // PFSB palette colors are already linear (exported from THREE.Color).
    float3 light = max(PushGamma(sb_ambient.rgb) + PushGamma(sb_sun_color.rgb) * sunlight, 0.0001);
    float3 color = input.color.rgb * light;
    float worldDistance = length(input.miniature - sb_camera_scale.xyz) * sb_camera_scale.w;
    // Match combine_1 exactly, including gamma and legacy-light blending.
    float fog = PushGamma(saturate(worldDistance * fog_params.w + fog_params.x));
    color = lerp(color, PushGamma(fog_color.rgb), fog);
#ifdef USE_LEGACY_LIGHT
    fog *= fog;
#endif
    if (fog >= 1.0) discard;
    output.color = float4(color, fog);
    float2 velocity = input.current.xy / max(input.current.w, 0.0001) - input.previous.xy / max(input.previous.w, 0.0001);
    output.velocity = float4(velocity, 0.0, fog);
    return output;
}
