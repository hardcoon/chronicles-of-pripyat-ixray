#include "common.hlsli"

cbuffer DetailConstants
{
    float4 consts;

    float4 wave;
    float4 wave_old;

    float4 dir2D;
    float4 dir2D_old;
};

//LVutner: Has to match the CPU struct
struct InstanceData
{
	float3 hpb;
	float scale;
	float3 pos;
	float hemi;
    float4 grass; // COP_ENGINE_PATCH: COP-ENG-170: world bend XZ, vertical scale, reserved
    float4 grass_old;
};

//LVutner: Always bound to slot0 (see CPP code)
StructuredBuffer<InstanceData> detail_buffer : register(t0);

float3x3 setMatrix (float3 hpb)
{
    float _ch, _cp, _cb, _sh, _sp, _sb, _cc, _cs, _sc, _ss;

    sincos(hpb.x, _sh, _ch);
    sincos(hpb.y, _sp, _cp);
    sincos(hpb.z, _sb, _cb);

    _cc = _ch*_cb; _cs = _ch*_sb; _sc = _sh*_cb; _ss = _sh*_sb;

    return float3x3(_cc-_sp*_ss, _sp*_sc+_cs, -_cp*_sh,
					-_cp*_sb, 	 _cp*_cb,	  _sp,
					_sp*_cs+_sc, _ss-_sp*_cc, _cp*_ch);
};

float3 vehicle_grass_offset(float4 shape, float3 up, float height)
{
    // Keep deformation tangent to the terrain-aligned plant, including sloped ground.
    [branch] if (shape.z == 1.0f) return 0.0f;
    float3 bend = float3(shape.x, 0.0f, shape.y);
    float strength = length(bend);
    bend -= up * dot(bend, up);
    float planar_length = length(bend);
    if (planar_length > 0.0001f) bend *= strength / planar_length;
    else bend = normalize(cross(up, abs(up.y) < 0.99f ? float3(0,1,0) : float3(1,0,0))) * strength;
    return height * (up * (shape.z - 1.0f) + bend);
}

void main(in v_detail I, out p_bumped_new O, uint instance_id : SV_InstanceID)
{
	//LVutner: Read our structured buffer
	InstanceData det = detail_buffer[instance_id];

	float3x3 mmhpb = setMatrix(det.hpb);

	float hemi = abs(det.hemi);
	float sun = sign(det.hemi)*0.25f+0.25f;

    float4 m0 = float4(mmhpb[0]*det.scale, det.pos.x);
    float4 m1 = float4(mmhpb[1]*det.scale, det.pos.y);
    float4 m2 = float4(mmhpb[2]*det.scale, det.pos.z);

    float4 pos, pos_old;
    pos.x = dot(m0, float4(I.pos.xyz, 1.0));
    pos.y = dot(m1, float4(I.pos.xyz, 1.0));
    pos.z = dot(m2, float4(I.pos.xyz, 1.0));

    pos.w = 1.0f;
    pos_old = pos;
    // COP_ENGINE_PATCH: COP-ENG-170 - rooted deformation in all colour/shadow passes.
    const float rooted_height = max(I.pos.y, 0.0f) * det.scale;
    const float3 plant_up = float3(mmhpb[0].y, mmhpb[1].y, mmhpb[2].y);
    pos.xyz += vehicle_grass_offset(det.grass, plant_up, rooted_height);
    pos_old.xyz += vehicle_grass_offset(det.grass_old, plant_up, rooted_height);

#ifdef USE_TREEWAVE
    float H = I.pos.y * length(m1.xyz);

    float dp = calc_cyclic(dot(pos, wave));
    float inten = H * det.grass.z * dp;

    pos.xz += calc_xz_wave(dir2D.xz * inten, I.pos.w);

    #ifndef DETAIL_SHADOW_PASS
		float dp_old = calc_cyclic(dot(pos_old, wave_old));
		float inten_old = H * det.grass_old.z * dp_old;

		pos_old.xz += calc_xz_wave(dir2D_old.xz * inten_old, I.pos.w);
    #endif
#endif

    float3 Pe = mul(m_WV, pos);

    float3 N;
    N.x = pos.x - m0.w;
    N.y = pos.y - m1.w + 0.75f;
    N.z = pos.z - m2.w;

    O.tcdh = float4(I.tc.xy, hemi, sun);
    O.position = float4(Pe, 1.0f);

    N.xyz = mul((float3x3)m_WV, N.xyz);

    O.M1 = N.xxx;
    O.M2 = N.yyy;
    O.M3 = N.zzz;

    O.hpos = mul(m_WVP, pos);

#ifndef DETAIL_SHADOW_PASS
    O.hpos_curr = O.hpos;
    O.hpos_old = mul(m_VP_old, pos_old);

    O.hpos.xy += m_taa_jitter.xy * O.hpos.w;
#else
    O.hpos_curr = O.hpos_old = O.hpos;
#endif
	O.snow_mask = 0.0;
}
