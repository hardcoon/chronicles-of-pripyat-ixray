// Original SSS 23.5 terrain puddles / ascii1457, adapted to IX-Ray materials.
#ifndef PF_SSS_PUDDLES_INCLUDED
#define PF_SSS_PUDDLES_INCLUDED
#include "pf_sss_common.hlsli"
#include "pf_sss_options.hlsli"
#include "pf_sss_ripples.hlsli"
Texture2D pf_sss_puddle_mask;
Texture2D pf_sss_perlin;
Texture2D pf_sss_water_normal;
Texture2D pf_sss_rain_splash;
float pf_sss_puddle_wetness() {
    return PF_SSS_PUDDLES_ALWAYS>.5 ? 1.0 : max(PF_SSS_PUDDLES_MIN_WETNESS,saturate(pf_sss_weather_rain.y));
}
float pf_sss_terrain_puddle_film(float terrainHeight) {
    return PF_SSS_TERRAIN_ENABLED>.5 ? saturate((pf_sss_puddle_wetness()*PF_SSS_TERRAIN_WATER_LEVEL*.5-terrainHeight)*4) : 1;
}
float pf_sss_apply_puddles(float2 baseUV,float3x3 basis,inout IXrayMaterial m,float terrainFilm) {
    float wetness=pf_sss_puddle_wetness();
    if(wetness<=0)return 0;
    float mask=1.0-pf_sss_puddle_mask.Sample(smp_base,baseUV).x;
    float size=1.0-PF_SSS_PUDDLES_SIZE*1.1;
    float puddle=saturate((pf_sss_perlin.Sample(smp_base,baseUV*15/PF_SSS_PUDDLES_GLOBAL_SIZE).x-size)*wetness);
    float3 geometricNormal=normalize(mul(basis,float3(0,0,1)));
    float3 worldNormal=mul((float3x3)m_invV,geometricNormal);
    // IX-Ray's up axis is Y. Horizontal faces alone receive the puddle film.
    float slope=smoothstep(.90,.995,worldNormal.y);
    puddle*=slope*mask*terrainFilm;
    puddle=smoothstep(0,max(.0001,.3-PF_SSS_PUDDLES_BORDER*.3),puddle);
    float3 ripple=0;
    if(PF_SSS_PUDDLES_RIPPLES>.5) {
        float rain=saturate(pf_sss_weather_rain.x*1.5);
        float speed=timers.x*(.01+rain*.008)*PF_SSS_PUDDLES_RIPPLE_SPEED;
        float scale=140/PF_SSS_PUDDLES_RIPPLE_SCALE;
        float3 n0=pf_sss_water_normal.Sample(smp_base,baseUV*scale+float2(0,speed)).xyz;
        float3 n1=pf_sss_water_normal.Sample(smp_base,baseUV*scale-float2(.33,speed)).xyz;
        ripple=(n0+n1-1)*saturate(.05+rain*PF_SSS_PUDDLES_RIPPLE_RAIN*PF_SSS_PUDDLES_RIPPLE_INT);
        float2 rainRipple=ssfx_rain_ripples(pf_sss_rain_splash,baseUV*scale*1.3/PF_SSS_PUDDLES_RAIN_SCALE,float3(.85,1,10),m.Depth);
        rainRipple*=pf_sss_weather_rain.x*3*PF_SSS_PUDDLES_RAIN_INT;
        ripple=ripple*.666+float3(rainRipple*.333,0);
    }
    m.Normal=normalize(lerp(m.Normal,mul((float3x3)m_V,float3(ripple.x,1,ripple.y)),puddle));
    // Detail refraction uses the same layer weights as native terrain.
    float2 detailUV=baseUV*dt_params.xy;
    float2 refraction=ripple.xy*.15*smoothstep(.6,.8,wetness*puddle)*PF_SSS_PUDDLES_REFRACTION;
    float3 currentDetail,refractionDetail;
#ifdef USE_4_BUMP
    float4 weights=s_mask.Sample(smp_base,baseUV);weights/=max(dot(weights,1),.0001);
    currentDetail=s_dt_r.Sample(smp_base,detailUV).xyz*weights.x+s_dt_g.Sample(smp_base,detailUV).xyz*weights.y+
        s_dt_b.Sample(smp_base,detailUV).xyz*weights.z+s_dt_a.Sample(smp_base,detailUV).xyz*weights.w;
    refractionDetail=s_dt_r.Sample(smp_base,detailUV+refraction).xyz*weights.x+s_dt_g.Sample(smp_base,detailUV+refraction).xyz*weights.y+
        s_dt_b.Sample(smp_base,detailUV+refraction).xyz*weights.z+s_dt_a.Sample(smp_base,detailUV+refraction).xyz*weights.w;
#else
    currentDetail=s_detail.Sample(smp_base,detailUV).xyz;
    refractionDetail=s_detail.Sample(smp_base,detailUV+refraction).xyz;
#endif
    m.Color.xyz*=refractionDetail/max(currentDetail,.001);
    float3 tint=float3(PF_SSS_PUDDLES_TINT_R,PF_SSS_PUDDLES_TINT_G,PF_SSS_PUDDLES_TINT_B);
    m.Color.xyz=lerp(m.Color.xyz,m.Color.xyz*tint,puddle);
    float gloss=min(puddle,PF_SSS_PUDDLES_REFLECTIVITY)+saturate(pf_sss_weather_rain.y*2)*PF_SSS_PUDDLES_EXTRA_WET*puddle;
#ifdef USE_LEGACY_LIGHT
    m.Roughness=max(m.Roughness,gloss);
#else
    m.Roughness=min(m.Roughness,1-saturate(gloss));
#endif
    return puddle;
}
float pf_sss_apply_puddles(float2 baseUV,float3x3 basis,inout IXrayMaterial m) {
    return pf_sss_apply_puddles(baseUV,basis,m,1);
}
#endif
