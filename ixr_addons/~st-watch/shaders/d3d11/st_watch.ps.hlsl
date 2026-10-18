// Watch v0.2 56157bfd60d1: orthogonal dial UV, filtered donor grid and finite phosphor glow.
#include "common.hlsli"
void main(p_bumped_new I,out IXrayGbufferPack O)
{
    IXrayMaterial M;
    M.Depth=I.position.z;
    M.Sun=0;
    M.Hemi=0;
    M.Point=I.position.xyz;
    M.Color=float4(.004,.005,.004,1);
    M.Normal=normalize(float3(I.M1.z,I.M2.z,I.M3.z));
    M.Metalness=0;
    // Legacy lighting stores gloss here; PBR stores roughness.
#ifdef USE_LEGACY_LIGHT
    M.Roughness=0;
#else
    M.Roughness=1;
#endif
    M.AO=1;
    M.SSS=0;
    M.SnowMask=0;
    O.Velocity=I.hpos_curr.xy/I.hpos_curr.w-I.hpos_old.xy/I.hpos_old.w;
    GbufferPack(O,M);
}
