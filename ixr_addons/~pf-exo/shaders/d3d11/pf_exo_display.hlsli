// Exoskeleton dashboard: native arm mount and live watch data.
#include "st_watch_display.hlsli"
#include "pf_exo_controls.hlsli"
#include "pf_exo_layout.hlsli"
static const float DA=1.5720421265;
// Shared font line box and baseline: captions keep one glyph scale across words.
static const float DASH_CAPTION_HEIGHT=.030*51.0/43.0;
float dash_box(float2 tc,float2 lo,float2 hi,float radius)
{
    float2 p=(tc-(lo+hi)*.5)*float2(DA,1);
    float2 q=abs(p)-(hi-lo)*float2(DA,1)*.5+radius;
    return length(max(q,0))+min(max(q.x,q.y),0)-radius;
}
float dash_cover(float d)
{
    return 1-smoothstep(0,max(fwidth(d),.0005),d);
}
float dash_stroke(float d,float width)
{
    float aa=max(fwidth(d),.0005);
    width=max(width,.55*aa);
    return 1-smoothstep(width-.4*aa,width+.6*aa,abs(d));
}
float dash_segment(float2 tc,float2 a,float2 b)
{
    tc*=float2(DA,1);a*=float2(DA,1);b*=float2(DA,1);
    float2 v=b-a;
    return length(tc-a-v*saturate(dot(tc-a,v)/max(dot(v,v),1e-8)));
}
float3 dash_primary(float amount)
{
    return watch_color_params.rgb*amount*.6*clamp(watch_color_params.w,.25,6);
}
float3 dash_font(float distance)
{
    float footprint=max(fwidth(distance),.0005);
    float shade=.0028/sqrt(distance*distance+footprint*footprint*.25);
    return PushGamma(min(watch_color_params.rgb*shade,2))*.6*clamp(watch_color_params.w,.25,6);
}
float3 dash_label(float2 tc,float2 origin,float height,int id,float emphasis)
{
    float language=step(5.5,watch_actor_params_1.w);
    float2 uv=(tc-origin)/float2(height*lerp(dash_label_aspect[id],dash_label_aspect_en[id],language)/DA,height);
    float inside=step(0,uv.x)*step(uv.x,1)*step(0,uv.y)*step(uv.y,1);
    float4 rect=lerp(dash_label_rect[id],dash_label_rect_en[id],language);
    float4 text=s_base.Sample(smp_base,lerp(rect.xy,rect.zw,saturate(uv)));
    float alpha=lerp(text.a,text.r,language)*inside;
    return dash_primary(.20*emphasis)*alpha;
}
float3 dash_number(float2 tc,float2 origin,float height,float value,int digits,bool percent)
{
    float scale=height*4;
    float2 uv=(tc-origin)*float2(DA,-1)/scale+float2(0,.25);
    float dist=100;
    [unroll]for(int i=0;i<4;i++)
    {
        float divisor=i==0?1000:(i==1?100:(i==2?10:1));
        float digit=fmod(floor(value/divisor),10);
        float d=st_glyph(uv-float2((i-(4-digits))*.1833333333,0),(int)digit);
        dist=min(dist,d+(i<4-digits)*1000);
    }
    if(percent)dist=min(dist,st_glyph(uv-float2(digits*.1833333333,.045),11));
    return dash_font(dist);
}
float3 dash_time(float2 tc)
{
    float2 uv=(tc-float2(.665,.100))*float2(DA,-1)/.39+float2(0,.25);
    float d=dfNumber(float2(0,0),game_time.x,uv);
    d=min(d,dfColon(float2(.20,0),uv));
    d=min(d,dfNumber(float2(.43,0),game_time.y,uv));
    return dash_font(d);
}
float dash_exoskeleton(float2 tc)
{
    float2 uv=(tc-float2(.066,.293))/float2(.180,.420);
    float inside=step(0,uv.x)*step(uv.x,1)*step(0,uv.y)*step(uv.y,1);
    return s_base.Sample(smp_base,lerp(dash_frame_rect.xy,dash_frame_rect.zw,saturate(uv))).a*inside;
}
float3 dash_exoskeleton_color(float condition)
{
    if(condition>=0 && condition<.25)return float3(.95,.055,.025)*.14*.6*clamp(watch_color_params.w,.25,6);
    if(condition>=0 && condition<.50)return float3(.95,.70,.02)*.14*.6*clamp(watch_color_params.w,.25,6);
    return dash_primary(.14);
}
float3 dash_status(float2 tc,float x,int glyph,float2 glyph_center,int label,float value,bool inverse_bar)
{
    // Keep every part of the status row above the lower physical bezel.
    tc.y+=.025;
    float b=dash_box(tc,float2(x,.79),float2(x+.205,.94),.007);
    float3 color=dash_primary(.004)*dash_cover(b)+dash_primary(.045)*dash_stroke(b,.0018);
    float2 uv=(tc-float2(x+.035,.859))*float2(DA,-1)/.58+glyph_center;
    float d=st_glyph(uv,glyph);
    float aa=max(fwidth(d),.0005);
    float width=max(.0025,.55*aa);
    float mask=1-smoothstep(width-.4*aa,width+.6*aa,d);
    mask+=.00035/max(d,.0006);
    float3 icon=value<.99?st_icon_color(value)*(.16*clamp(watch_color_params.w,.25,6)):dash_primary(.18);
    color+=icon*mask;
    color+=dash_label(tc,float2(x+.071,.810),.028,label,.95);
    float2 lo=float2(x+.075,.874),hi=float2(x+.186,.901);
    float body=dash_cover(dash_box(tc,lo,hi,.001));
    float level=value<0?(inverse_bar?0:1):(inverse_bar?1-saturate(value):saturate(value));
    // At zero, step alone lights the antialiased left edge of an empty bar.
    float filled=level>0?step(tc.x,lerp(lo.x,hi.x,level)):0;
    float3 bar=value<.99?st_icon_color(value)*(.10*clamp(watch_color_params.w,.25,6)):dash_primary(.22);
    color+=lerp(dash_primary(.025),bar,filled)*body;
    return color;
}
float3 exo_dashboard(float2 tc,float progress,float pressed_action,float outfit_condition)
{
    float2 p=(tc-.5)*2;
    // Match the watch's backlight gain and grid contrast at the existing fine square pitch.
    float grid=st_watch_grid((tc-.5)*float2(3.144084253,2)+.5);
    float falloff=.65+.35*exp(-dot(p,p)*1.5);
    float3 color=watch_color_params.rgb*.006*falloff*grid*.6*clamp(game_time.w,0,30);
    color+=dash_label(tc,float2(.058,.096),.062,0,1.05);

    color+=dash_time(tc);
    color+=dash_primary(.045)*dash_stroke(dash_segment(tc,float2(.058,.235),float2(.943,.235)),.0035);


    color+=dash_primary(.045)*dash_stroke(dash_segment(tc,float2(.575,.276),float2(.575,.716)),.0018);
    float fade=1-smoothstep(0,.7,progress);
    float3 vitals=dash_exoskeleton_color(outfit_condition)*dash_exoskeleton(tc);
    float health=round(saturate(watch_actor_params_1.x)*100);
    vitals+=dash_number(tc,float2(.29,.354),.139,health,health>=100?3:(health>=10?2:1),true);

    float health_bar=dash_cover(dash_box(tc,float2(.29,.555),float2(.55,.585),.002));
    // Bound the periodic segment mask so the previous cell cannot leak before the bar.
    float hp_segment=step(.18,frac(saturate((tc.x-.29)/.26)*12));
    float hp_fill=step(tc.x,.29+.26*saturate(watch_actor_params_1.x));
    vitals+=health_bar*hp_segment*lerp(dash_primary(.018),dash_primary(.21),hp_fill);
    vitals+=dash_label(tc,float2(.29,.635),DASH_CAPTION_HEIGHT,1,.95);
    vitals+=dash_label(tc,float2(.29,.675),DASH_CAPTION_HEIGHT,2,.95);
    if(health<30)vitals+=dash_label(tc,float2(.29,.728),.025,5,.9);
    color+=vitals*fade;
    // Treatment replaces the left schematic, avoiding half-clipped figures or
    // numbers behind the drawer. Health remains in a compact row above it.

    color+=(1-fade)*dash_number(tc,float2(.300,.316),.080,health,health>=100?3:(health>=10?2:1),true);

    float rad=round(watch_actor_params_1.z);
    color+=dash_number(tc,float2(.600,.354),.139,rad,4,false);
    // Restore the original hollow watch glyph, with a thicker stroke and safe bezel inset.
    float2 guv=(tc-float2(.906,.310))*float2(DA,-1)/.68+float2(-.1665,.1345);
    float rd=st_glyph(guv,16);
    float raa=max(fwidth(rd),.0008);
    float rwidth=max(.004,.55*raa);
    color+=dash_primary(.23)*(1-smoothstep(rwidth-.4*raa,rwidth+.6*raa,rd));
    float level=saturate(rad/500);
    float rate=dash_cover(dash_box(tc,float2(.60,.555),float2(.86,.585),.002));
    float u=saturate((tc.x-.60)/.26);
    float gap=step(.18,frac(u*12));
    float3 rcolor=u<.55?lerp(float3(.05,.8,.30),float3(.9,.8,.04),u/.55):lerp(float3(.9,.8,.04),float3(.9,.09,.04),(u-.55)/.45);
    color+=rcolor*rate*gap*lerp(.035,.35,step(u,level));




    // Actor radiation is accumulated game burden (0..1)*500, not dose rate.
    // Keep the readout unchanged and name the metric without physical units.
    color+=dash_label(tc,float2(.60,.635),DASH_CAPTION_HEIGHT,3,.95);
    color+=dash_label(tc,float2(.60,.675),DASH_CAPTION_HEIGHT,7,.95);
    if(rad>200)color+=dash_label(tc,float2(.60,.728),.025,5,.9);
    color+=dash_status(tc,.060,13,float2(.0005,.1315),9,watch_actor_params_2.y,false)*fade;
    color+=dash_status(tc,.285,15,float2(-.1665,.1370),11,watch_actor_params_2.w,true)*fade;
    color+=dash_status(tc,.510,12,float2(.0708,.1345),8,watch_actor_params_2.x,false);
    color+=dash_status(tc,.735,14,float2(-.0662,.1418),10,watch_actor_params_2.z,true);
    return pf_feedback_drawer(tc,color,progress,pressed_action);
}

float2 pf_exo_display_uv(float2 atlas)
{
    return float2((atlas.y-.513135314)/.486535549,1.0-(atlas.x-.001267910)/.313194215);
}
float3 pf_exo_display(float2 tc)
{
    // W: drawer progress + 2*pressed_action + 6*English + 12*power_off. Normal watch keeps modes.
    float value=max(watch_actor_params_1.w,0);
    // Keep the opaque mounted glass/depth while switching off all emitted content.
    if(value>=11.5)return float3(0,0,0);
    value-=6*step(5.5,value);
    float action=floor(value*.5);
    return exo_dashboard(tc,saturate(value-action*2),action,m_actor_params.y);
}
