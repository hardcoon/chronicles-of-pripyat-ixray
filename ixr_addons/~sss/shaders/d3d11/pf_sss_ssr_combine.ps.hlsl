#include "pf_sss_surface.hlsli"
Texture2D pf_sss_ssr_filtered;
float4 main(PFSSSInput i) : SV_Target {
    float4 scene=pf_sss_scene.SampleLevel(smp_rtlinear,i.uv,0);
    if(pf_sss_control.x<.5||PF_SSS_SSR_ENABLED<.5)return scene;
    float4 puddleData=pf_sss_data.SampleLevel(smp_nofilter,i.uv,0);
    if(puddleData.y<=.001 || puddleData.x*255>1.5 || pf_sss_raw_depth(i.uv)<.02)return scene;
    IXrayGbuffer o=pf_sss_unpack(i.uv);
    if (pf_sss_linear_depth(o.Depth)<=0)return scene;
    float3 reflected=reflect(o.View,o.Normal);
    float gloss=pf_sss_gloss(o)*pow(saturate(dot(reflected,o.View)),3);
    float category=pf_sss_data.SampleLevel(smp_nofilter,i.uv,0).x*255;
    if(category>1.5)gloss*=.33;
    gloss=o.Depth<.02 ? clamp(gloss*PF_SSS_SSR_WEAPON,0,PF_SSS_SSR_WEAPON_MAX+pf_sss_weather_rain.x*.2)
        :saturate(gloss*PF_SSS_SSR_INTENSITY);
    if(PF_SSS_SSR_RAIN_ONLY>.5)gloss*=pf_sss_weather_rain.x;
    float fog=1.0-saturate(o.ViewDist*fog_params.w+fog_params.x);gloss*=fog*fog;
    gloss*=puddleData.y;
    // Original SSS blends its reflected radiance over the lit scene.
    return float4(lerp(scene.xyz,pf_sss_ssr_filtered.SampleLevel(smp_rtlinear,i.uv,0).xyz,gloss),scene.w);
}
