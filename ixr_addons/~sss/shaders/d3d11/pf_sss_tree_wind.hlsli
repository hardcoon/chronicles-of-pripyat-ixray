// Screen Space Shaders 23.5 by ascii1457; IX-Ray R4 adapter by Hardcoon.
#ifndef PF_SSS_TREE_WIND_INCLUDED
#define PF_SSS_TREE_WIND_INCLUDED
#include "common.hlsli"
#include "pf_sss_options.hlsli"
uniform float4 pf_sss_wind;
uniform float4 pf_sss_wind_anim,pf_sss_wind_anim_previous;
Texture2D pf_sss_wind_wave : register(t1); // native detail instance buffer owns VS slot 0
struct wind_setup 
{
	float2 direction;
	float speed;
	float sqrt_speed;

	float trees_animspeed;
	float trees_trunk_animspeed;
	float trees_bend;

	float grass_animspeed;
	float grass_turbulence;
	float grass_push;
	float grass_wave;
};

wind_setup ssfx_wind_setup()
{
	wind_setup wsetup;

	// Direction. Radians to Vector
	float r = -pf_sss_wind.x + 1.57079f;
	wsetup.direction = float2(cos(r),sin(r));

	// Wind Speed
	wsetup.speed = max(PF_SSS_WIND_MIN, saturate(pf_sss_wind.y * 0.001));
	wsetup.sqrt_speed = saturate(sqrt(wsetup.speed * 1.66f));

	// Setup
	wsetup.grass_animspeed = PF_SSS_WIND_GRASS_SPEED;
	wsetup.grass_turbulence = PF_SSS_WIND_GRASS_TURBULENCE;
	wsetup.grass_push = PF_SSS_WIND_GRASS_PUSH;
	wsetup.grass_wave = PF_SSS_WIND_GRASS_WAVE;

	wsetup.trees_animspeed = PF_SSS_WIND_TREE_SPEED;
	wsetup.trees_trunk_animspeed = PF_SSS_WIND_TREE_TRUNK;
	wsetup.trees_bend = PF_SSS_WIND_TREE_BEND;

	return wsetup;
}



float3 ssfx_wind_tree_trunk(float3 pos, float Tree_H, wind_setup W, bool prev)
{
	float4 anim = prev ? pf_sss_wind_anim_previous : pf_sss_wind_anim;

	// Phase ( from matrix ) + Offset 
	float Phase = m_xform._24 + anim.z * W.trees_trunk_animspeed;

	// Trunk wave
	float TWave = (cos(Phase) * sin(Phase * 5.0f) + 0.5f) * W.trees_bend;

	// Wind speed
	float WSpeed = saturate(W.sqrt_speed * 1.5f);

	// Base wind speed displacement
	float Base_Bend = WSpeed * 0.006f * saturate(1.0f - Tree_H * 0.005f);

	// Intensity ( Squared height )
	Base_Bend *= Tree_H * Tree_H * TWave * WSpeed;

	// Apply direction
	float2 Final = Base_Bend.xx * W.direction;

	return float3(Final, saturate((TWave + 1.0f) * 0.5));
}

float3 ssfx_wind_tree_branches(float3 pos, float Tree_H, float tc_y, wind_setup W, bool prev)
{
	float4 anim = prev ? pf_sss_wind_anim_previous : pf_sss_wind_anim;

	// UV Offset
	float2 Offset = -anim.xy * W.trees_animspeed;
	
	// Sample flow map
	float3 Flow = pf_sss_wind_wave.SampleLevel( smp_linear, (pos.xz + Offset) * 0.02f, 0);
	
	// Sample 2, slower and detailed
	float3 Flow2 = pf_sss_wind_wave.SampleLevel( smp_linear, (pos.xz + Offset * 0.2f) * 0.1f, 0);

	// Branch motion [ -1.0 ~ 1.0 ]
	float3 branchMotion = float3(Flow.x, Flow2.y, Flow.y) * 2.0f - 1.0f;

	// Trunk position
	float3 Trunk = ssfx_wind_tree_trunk(pos, Tree_H, W, prev);

	// Gust from trunk data.
	branchMotion.xz *= Trunk.z * clamp(Tree_H * 0.1f, 1.0f, 2.5f);

	// Add wind direction
	branchMotion.xz += Flow2.z * W.direction;

	// Add wind gust
	branchMotion.y *= saturate(Tree_H * 0.1f);

	// Everything is limited by the UV and wind speed
	branchMotion *= (1.0f - tc_y) * W.speed;

	// Add trunk animation
	branchMotion.xz += Trunk.xy;
	
	return branchMotion;
}


// IX-Ray uses one tree VS for both materials. Alpha-tested foliage receives
// branch motion; opaque bark receives trunk motion only. Both passes use this.
float3 pf_sss_tree_motion(float3 pos, float height, float uv_y, bool previous)
{
    wind_setup setup = ssfx_wind_setup();
#ifdef USE_AREF
    return ssfx_wind_tree_branches(pos, height, uv_y, setup, previous);
#else
    float2 trunk = ssfx_wind_tree_trunk(pos, height, setup, previous).xy;
    return float3(trunk.x, 0, trunk.y);
#endif
}
#endif
