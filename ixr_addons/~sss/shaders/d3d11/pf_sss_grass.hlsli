// Screen Space Shaders interactive grass by ascii1457; IX-Ray adapter by Hardcoon.
#ifndef PF_SSS_GRASS_INCLUDED
#define PF_SSS_GRASS_INCLUDED
#include "pf_sss_options.hlsli"
uniform float4 pf_sss_benders[48], pf_sss_benders_previous[48];

float3 pf_sss_grass_interaction(float3 pos, float height, float3 plant_up, bool previous)
{
    float3 result = 0;
    [branch] if (height <= 0) return result;
    // Player + 15 nearest entities + 8 bounded transient events, maintained natively.
    [loop] for (int i = 0; i < 24; ++i)
    {
        float4 bender_pos = previous ? pf_sss_benders_previous[i] : pf_sss_benders[i];
        [branch] if (bender_pos.w <= 0) continue;
        float4 setup = previous ? pf_sss_benders_previous[i + 24] : pf_sss_benders[i + 24];
        [branch] if (setup.w <= 0) continue;
        bool character = i < 16;
        float radius = character ? PF_SSS_GRASS_RADIUS * bender_pos.w : bender_pos.w;
        [branch] if (radius <= 0) continue;
        float3 delta = pos - bender_pos.xyz;
        float distance_squared = dot(delta.xz, delta.xz);
        [branch] if (distance_squared >= radius * radius) continue;
        float influence = 1.0f - saturate(sqrt(distance_squared) / max(radius, 0.001f));
        float vertical_limit = 1.0f - saturate(abs(delta.y) / (character ? 2.0f : max(radius, 0.001f)));
        float3 direction = delta * rsqrt(max(dot(delta, delta), 0.000001f)) * influence;
        float direction_limit = setup.y >= -1.0f ? saturate(dot(direction, setup.xyz) * 5.0f) : 1.0f;
        float horizontal = setup.w * (character ? PF_SSS_GRASS_HORIZONTAL : 1.0f);
        float vertical = setup.w * (character ? PF_SSS_GRASS_VERTICAL : 1.0f);
        result.xz += direction.xz * 2.0f * horizontal * vertical_limit * height * direction_limit;
        result.y -= influence * 0.6f * vertical * vertical_limit * height * direction_limit;
    }
    // Cap overlap without burying grass; keep displacement tangent to sloped terrain.
    float downward = max(result.y, -0.95f * height);
    float3 tangent = float3(result.x, 0, result.z);
    tangent -= plant_up * dot(tangent, plant_up);
    tangent *= min(1.0f, 2.0f * height / max(length(tangent), 0.0001f));
    return tangent + plant_up * downward;
}
#endif
