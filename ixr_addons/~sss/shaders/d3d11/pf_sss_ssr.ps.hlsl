// Adapted SSS 23.5 SSR/gloss/temporal phases, original ascii1457.
#include "pf_sss_surface.hlsli"
Texture2D pf_sss_ssr_previous;
float4 main(PFSSSInput i) : SV_Target {
    float4 puddleData=pf_sss_data.SampleLevel(smp_nofilter,i.uv,0);
    if(puddleData.y<=.001 || puddleData.x*255>1.5 || pf_sss_raw_depth(i.uv)<.02)return 0;
    IXrayGbuffer o = pf_sss_unpack(i.uv);
    float depth = pf_sss_linear_depth(o.Depth);
    if (depth<=0 || pf_sss_control.x<.5 || PF_SSS_SSR_ENABLED<.5) return 0;
    bool hud=o.Depth<.02;
    float category=pf_sss_data.SampleLevel(smp_nofilter,i.uv,0).x*255.0;
    float3 reflected=reflect(o.View,o.Normal);
    float fresnel=pow(saturate(dot(reflected,o.View)),3);
    float gloss=pf_sss_gloss(o)*fresnel;
    if (category>1.5) gloss*=.33;
    gloss=hud ? clamp(gloss*PF_SSS_SSR_WEAPON,0,PF_SSS_SSR_WEAPON_MAX+pf_sss_weather_rain.x*.2)
        : saturate(gloss*PF_SSS_SSR_INTENSITY);
    if (PF_SSS_SSR_RAIN_ONLY>.5) gloss*=pf_sss_weather_rain.x;
    float fog=1.0-saturate(o.ViewDist*fog_params.w+fog_params.x);gloss*=fog*fog;
    float3 sky=pf_sss_sky(reflected)*PF_SSS_SSR_SKY*(hud?1:saturate(o.Hemi*2));
    float noise=PF_SSS_SSR_NOISE>.5 ? pf_sss_hash12(i.uv+timers.xx)*.5 : 0;
    float4 hit=gloss>.02 ? pf_sss_trace(o.PointReal+o.Normal*.01,reflected,i.uv,(int)PF_SSS_SSR_QUALITY,noise,hud) : 0;
    float3 reflectedColor=sky;
    bool terrain=category>.5 && category<1.5;
    if (all(hit.xy>0)) reflectedColor=lerp(sky*terrain,pf_sss_scene.SampleLevel(smp_rtlinear,hit.xy,0).xyz,saturate(hit.y*4));
    else reflectedColor*=saturate(hit.w*4+terrain);
    reflectedColor*=saturate(gloss*10);
    float2 velocity=s_velocity.SampleLevel(smp_nofilter,i.uv,0).xy*float2(.5,-.5);
    float2 previousUV=i.uv-velocity;
    float4 previous=pf_sss_ssr_previous.SampleLevel(smp_rtlinear,previousUV,0);
    float disocclusion=abs(previous.w-depth)/max(depth,.01)*5;
    float weight=saturate(PF_SSS_SSR_TEMPORAL-disocclusion-length(velocity)*5-(hit.z<=0?.2:0));
    weight*=pf_sss_history.x*pf_sss_valid_uv(previousUV)*!hud;
    return float4(lerp(reflectedColor,previous.xyz,weight),depth);
}
