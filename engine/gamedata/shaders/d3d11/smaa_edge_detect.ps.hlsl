/*
------------------------------------------------------------------
SMAA edge detection pass
------------------------------------------------------------------
References:
https://github.com/iryoku/smaa
------------------------------------------------------------------
Back to the Roots
*/

#include "common.hlsli"

#if defined(SM_5) || defined(SM_4_1)
    #define SMAA_HLSL_4_1
#else
    #define SMAA_HLSL_4
#endif

uniform float4 scaled_screen_res;
#define SMAA_RT_METRICS scaled_screen_res.zwxy

#define SMAA_PRESET_ULTRA
#define EDGE_DETECT_COLOR

#include "smaa.hlsli"

Texture2D s_hud_mask; // COP_ENGINE_PATCH: COP-ENG-011

// Struct
struct p_smaa
{
    float4 hpos : SV_POSITION;
    float2 tc0 : TEXCOORD0; // Texture coordinates         (for sampling maps)
    float4 offset[3] : TEXCOORD1;
};

float4 main(p_smaa I) : SV_Target
{
#if defined(EDGE_DETECT_COLOR)
    float2 edges = SMAAColorEdgeDetectionPS(I.tc0, I.offset, s_image);

	// COP_ENGINE_PATCH: COP-ENG-011 - in hybrid mode retain only edges that touch the exact HUD mask.
	if (m_hybrid_aa_params.y > 0.5f)
	{
		float maskCenter = s_hud_mask.SampleLevel(smp_nofilter, I.tc0, 0).x;
		float maskLeft = s_hud_mask.SampleLevel(smp_nofilter, I.offset[0].xy, 0).x;
		float maskTop = s_hud_mask.SampleLevel(smp_nofilter, I.offset[0].zw, 0).x;
		edges *= step(float2(0.5f, 0.5f), float2(max(maskCenter, maskLeft), max(maskCenter, maskTop)));
		if (dot(edges, float2(1.0f, 1.0f)) == 0.0f)
			discard;
	}

	return float4(edges, 0.0f, 0.0f);
#else
    return float4(SMAALumaEdgeDetectionPS(I.tc0, I.offset, s_image), 0.0f, 0.0f);
#endif
}
