// Secret Trails display; native IX-Ray fallback until the watch data bridge is installed.
#ifndef ST_WATCH_DISPLAY
#define ST_WATCH_DISPLAY
uniform float4 m_actor_params;
uniform float4 m_timearrow;
uniform float4 m_digiclock;
#ifndef PI
#define PI 3.14159265359
#endif
#define TPI 6.28318530718
// Watch vector: hours, minutes, seconds, independent display backlight.
uniform float4 game_time;
uniform float4 watch_actor_params_1;
uniform float4 watch_actor_params_2;
uniform float4 watch_color_params;
#include "st_watch_font.hlsli"
// Layout, glyphs, icon colors, glow and grid are adapted from Secret Trails.
// All three modes use the same straight-line pixel program. Avoid driver
// specialization of nested uniform branches and digit-count loops.
float3 st_icon_color(float param)
{
    return param<-.9?lerp(float3(0,.1,.5),0,param):lerp(float3(.9,.1,0),float3(0,.9,.1),param);
}
float3 st_icon(float2 uv,float2 pos,int glyph,float param,float threshold)
{
    return st_icon_color(param)*(.0003/max(st_glyph(uv-pos,glyph),.001))*(param<threshold);
}
float3 st_dosimeter(float2 uv)
{
    uv*=280;
    float radius=length(uv);
    float degrees=atan2(uv.y,uv.x)*180/PI;
    float edge=smoothstep(0,1,1-abs(degrees/90-1));
    float ring=(uv.y>0)*(radius<190)*(radius>170)*(radius>190-15*(.3+.7*edge))*(fmod(degrees,12)>=2);
    float3 color=lerp(float3(1,0,0),float3(0,1,0),degrees/180)*ring;
    // Row-vector UV rotation is inverse to the visible pointer rotation.
    // Positive angle moves the pointer across the upper arc: green to red.
    float angle=saturate(watch_actor_params_1.z/500)*PI;
    uv=mul(uv,float2x2(cos(angle),sin(angle),-sin(angle),cos(angle)));
    float pointer=((uv.x<0)*(abs((uv.x+180)/50)>abs(uv.y))*(uv.x>-160))+(length(uv)<6);
    return lerp(color,float3(.5,.4,.1),saturate(pointer));
}
float st_watch_grid(float2 tc)
{
    float2 footprint_grid=max(fwidth(tc*32),.0001);
    float2 cell=abs(frac(tc*32)-.5);
    float2 coverage=saturate((.45-cell)/footprint_grid+.5);
    return lerp(coverage.x*coverage.y,.81,saturate(max(footprint_grid.x,footprint_grid.y)-.35));
}
// A faint linear phosphor floor survives gamma conversion of the bright glyphs.
// It is emissive only: no sky reflection, gloss or specular term.
float3 st_watch_backlight(float2 tc)
{
    float2 p=(tc-.5)*2;
    float falloff=.65+.35*exp(-dot(p,p)*1.5);
    return watch_color_params.rgb*.006*falloff*st_watch_grid(tc);
}
float3 NixieTime(float2 tc)
{
    float2 uv=(tc-.5)*float2(1.1,-1.1);
    float2 basepos=-float2(1.1,1.6)/6*float2(numberLength(9999),1)*.5;
    float time_dist=min(dfNumber(basepos+float2(.16,0),game_time.x,uv),dfColon(basepos+float2(.39,0),uv));
    time_dist=min(time_dist,dfNumber(basepos+float2(.60,0),game_time.y,uv));

    float4 params=watch_actor_params_2;
    float radiation=watch_actor_params_1.y;
    float indicators=(params.x<.99)||(params.y<.99)||(params.z<1)||(params.w<.99)||(radiation<.99);
    float health=round(watch_actor_params_1.x*100);
    float hx=health<10?.42:(health<100?.33:.22);
    float hs=health<10?.2:(health<100?.35:.55);
    float2 health_pos=basepos+float2(hx,indicators*.1);
    float health_dist=min(dfNumberHealth(health_pos,health,uv),dfPercent(health_pos+float2(hs,0),uv));
    float2 icon_pos=basepos+float2(.15,-.25);
    float3 icon_color=st_icon(uv,icon_pos,12,params.x,.99);
    icon_color+=st_icon(uv,icon_pos+float2(.25,0),13,params.y,.99);
    // Dynamic weight supplies exactly hidden (1), yellow (.5) or red (0).
    float3 weight_color=lerp(float3(.9,.1,0),float3(.9,.75,0),step(.25,params.z));
    float weight_glyph=.0003/max(st_glyph(uv-icon_pos-float2(.50,0),14),.001);
    icon_color+=weight_color*weight_glyph*(params.z<1);
    icon_color+=st_icon(uv,icon_pos+float2(.75,0),15,params.w,.99);
    icon_color+=st_icon(uv,icon_pos+float2(.90,0),16,radiation,.99);

    float rad_dist=dfNumber2(basepos+float2(.15,-.3),round(watch_actor_params_1.z),uv*1.3);
    float3 select=float3(watch_actor_params_1.w==1,watch_actor_params_1.w==2,watch_actor_params_1.w==3);
    float dist=dot(select,float3(time_dist,health_dist,rad_dist));
    float footprint=max(fwidth(dist),.0004);
    float shade=.003/sqrt(dist*dist+footprint*footprint*.25);
    float3 color=watch_color_params.rgb*shade+icon_color*select.y+st_dosimeter(uv*1.3)*select.z;
    color*=st_watch_grid(tc);
    return min(color,2);
}

float3 st_watch_display(float2 tc) {return NixieTime(tc);}
#endif
