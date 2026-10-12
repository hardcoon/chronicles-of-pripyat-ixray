#include "pf_sss_surface.hlsli"
Texture2D pf_sss_ssr_current;
float4 main(PFSSSInput i) : SV_Target {
    uint w,h;pf_sss_ssr_current.GetDimensions(w,h);
    float4 center=pf_sss_ssr_current.SampleLevel(smp_rtlinear,i.uv,0);
    float3 sum=center.xyz;float weights=1;
    float3 n=NormalDecode(s_normal.SampleLevel(smp_nofilter,i.uv,0).xy);
    [unroll]for(int y=-1;y<=1;++y) [unroll]for(int x=-1;x<=1;++x) {
        if (!x&&!y)continue;
        float2 uv=i.uv+float2(x,y)*PF_SSS_SSR_BLUR*3/float2(w,h);
        float4 tap=pf_sss_ssr_current.SampleLevel(smp_rtlinear,uv,0);
        float3 tn=NormalDecode(s_normal.SampleLevel(smp_nofilter,uv,0).xy);
        float weight=exp(-abs(tap.w-center.w)*2)*pow(saturate(dot(n,tn)),8);
        sum+=tap.xyz*weight;weights+=weight;
    }
    return float4(sum/weights,center.w);
}
