// Shared live watch palette, two rectangular touch targets and pull tabs.
#ifndef PF_EXO_FEEDBACK_CONTROLS
#define PF_EXO_FEEDBACK_CONTROLS
#include "st_watch_display.hlsli"
float pf_rect_distance(float2 p,float2 half_size)
{
    float2 q=abs(p)-half_size;
    return length(max(q,0.0))+min(max(q.x,q.y),0.0);
}
float pf_rect_line(float distance,float width)
{
    float aa=max(fwidth(distance),.001);
    width=max(width,.55*aa);
    return 1.0-smoothstep(width-.4*aa,width+.6*aa,abs(distance));
}
float pf_touch_glyph(float2 p,bool radio)
{
    if(radio)return st_glyph(p*.26+float2(-.167236,.133789),16);
    return min(pf_rect_distance(p,float2(.075,.24)),pf_rect_distance(p,float2(.24,.075)));
}
float3 pf_touch_row(float2 p,bool radio,float3 background,float pressed)
{
    float box=pf_rect_distance(p,float2(1.34,.36));
    float aa=max(fwidth(box),.001);
    float fill=1.0-smoothstep(0.0,aa,box);
    float edge=pf_rect_line(box,.007);
    float3 row=background*1.85+watch_color_params.rgb*.0015;
    float glyph=pf_touch_glyph(p,radio);
    float footprint=max(fwidth(glyph),.0006);
    float glow=.0018/sqrt(glyph*glyph+footprint*footprint*.25);
    if(radio)glow+=.45*(1.0-smoothstep(.0015,.0015+footprint*.75,glyph));
    if(!radio)glow+=.65*(1.0-smoothstep(-footprint,footprint,glyph));
    float3 ink=PushGamma(min(watch_color_params.rgb*glow,2.0))*.6*clamp(watch_color_params.w,.25,6.0);
    float3 border=PushGamma(watch_color_params.rgb*.42)*.35*clamp(watch_color_params.w,.25,6.0)*edge;
    float3 normal=lerp(background,row+ink+border,fill);
    float glyph_cutout=radio?1.0-smoothstep(.002,.002+footprint,glyph):1.0-smoothstep(-footprint,footprint,glyph);
    float3 pressed_fill=PushGamma(watch_color_params.rgb*.4)*.6*clamp(watch_color_params.w,.25,6.0);
    float3 inverse=lerp(pressed_fill,background,glyph_cutout);
    return lerp(normal,lerp(background,inverse,fill),saturate(pressed));
}
float pf_pull_tab(float2 p)
{
    // Rounded triangular tab: keep a visible pull grip without a sharp spear tip.
    float2 a=float2(1.47,-.10),b=float2(1.65,0),c=float2(1.47,.10);
    float2 ab=b-a,bc=c-b,ca=a-c,ap=p-a,bp=p-b,cp=p-c;
    float2 d0=ap-ab*saturate(dot(ap,ab)/dot(ab,ab));
    float2 d1=bp-bc*saturate(dot(bp,bc)/dot(bc,bc));
    float2 d2=cp-ca*saturate(dot(cp,ca)/dot(ca,ca));
    float d=sqrt(min(dot(d0,d0),min(dot(d1,d1),dot(d2,d2))));
    float side=min(ab.x*ap.y-ab.y*ap.x,min(bc.x*bp.y-bc.y*bp.x,ca.x*cp.y-ca.y*cp.x));
    return (side>=0?-d:d)-.045;
}
float pf_segment_distance(float2 p,float2 a,float2 b)
{
    float2 v=b-a;
    return length(p-a-v*saturate(dot(p-a,v)/dot(v,v)));
}
float4 pf_feedback_controls(float2 tc,float pressed_action)
{
    float2 p=(tc-.5)*float2(3.0,2.0);
    float body=pf_rect_distance(p,float2(1.5,1.0));
    float tab=min(pf_pull_tab(p-float2(0,-.5)),pf_pull_tab(p-float2(0,.5)));
    float silhouette=min(body,tab);
    float aa=max(fwidth(silhouette),.001);
    float coverage=1.0-smoothstep(0.0,aa,silhouette);
    float backlight=clamp(game_time.w,0.0,30.0);
    float3 background=float3(.0030,.0035,.0045)*(.7+backlight*.025);
    background+=watch_color_params.rgb*.0008*backlight*.05;
    float3 color=pf_touch_row(p-float2(0,-.5),false,background,pressed_action==1);
    float2 lower=p-float2(0,.5);
    float lower_box=pf_rect_distance(lower,float2(1.34,.36));
    color=lerp(color,pf_touch_row(lower,true,background,pressed_action==2),1.0-smoothstep(0.0,aa,lower_box));
    color+=PushGamma(watch_color_params.rgb*.2)*.25*clamp(watch_color_params.w,.25,6.0)*pf_rect_line(silhouette,.006);
    float2 marker=float2(p.x,abs(p.y)-.5);
    float marker_distance=min(pf_segment_distance(marker,float2(1.535,-.047),float2(1.582,0)),
                              pf_segment_distance(marker,float2(1.582,0),float2(1.535,.047)));
    float marker_aa=max(fwidth(marker_distance),.001);
    float chevron=1.0-smoothstep(.010,.010+marker_aa,marker_distance);
    color+=PushGamma(watch_color_params.rgb*.45)*.45*clamp(watch_color_params.w,.25,6.0)*chevron;
    // Fully cover HDR digits under the drawer; tiny residual alpha would leave
    // a bright health number ghosting through the touch targets after gamma.
    return float4(color,coverage);
}
float3 pf_feedback_drawer(float2 tc,float3 display,float progress,float pressed_action)
{
    float2 center=float2(.2607214940,.6654896917);
    center.x-=.4289475403*(1.0-saturate(progress));
    float2 local_tc=(tc-center)/float2(.3644520926,.3819560285)+.5;
    float4 drawer=pf_feedback_controls(local_tc,pressed_action);
    return lerp(display,drawer.rgb,drawer.a);
}
#endif
