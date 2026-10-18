// Original Secret Trails glyph distances, baked with bake_font.cpp (WARP).
Texture2D<float> s_watch_sdf;
float st_glyph(float2 uv,int glyph)
{
    float2 tile=(uv+.25)/.75;
    float2 clipped=clamp(tile,.5/512.0,1.0-.5/512.0);
    float2 cell=float2(glyph%8,glyph/8);
    float value=s_watch_sdf.SampleLevel(smp_rtlinear,(cell+clipped)/float2(8,3),0);
    return value+length(tile-clipped)*.75;
}
float dfDigit(float2 origin,float digit,float2 uv){return st_glyph(uv-origin,(int)clamp(floor(digit),0,9));}
float dfNumber(float2 origin,float num,float2 uv)
{
    return min(st_glyph(uv-origin,(int)fmod(floor(num*.1),10)),st_glyph(uv-origin-float2(1.1/6.0,0),(int)fmod(floor(num),10)));
}
float dfNumberHealth(float2 origin,float num,float2 uv)
{
    float count=num>=100?3:(num>=10?2:1);
    float d=st_glyph(uv-origin,(int)fmod(floor(num/(count==3?100:(count==2?10:1))),10));
    d=min(d,st_glyph(uv-origin-float2(1.1/6.0,0),(int)fmod(floor(num/(count==3?10:1)),10))+(count<2)*1000);
    d=min(d,st_glyph(uv-origin-float2(2.2/6.0,0),(int)fmod(floor(num),10))+(count<3)*1000);
    return d;
}
float dfNumber2(float2 origin,float num,float2 uv)
{
    float d=st_glyph(uv-origin,(int)fmod(floor(num*.001),10));
    d=min(d,st_glyph(uv-origin-float2(1.3/6.0,0),(int)fmod(floor(num*.01),10)));
    d=min(d,st_glyph(uv-origin-float2(2.6/6.0,0),(int)fmod(floor(num*.1),10)));
    return min(d,st_glyph(uv-origin-float2(3.9/6.0,0),(int)fmod(floor(num),10)));
}
float dfColon(float2 origin,float2 uv){return st_glyph(uv-origin,10);}
float dfPercent(float2 origin,float2 uv){return st_glyph(uv-origin,11);}
float dfEat(float2 origin,float2 uv){return st_glyph(uv-origin,12);}
float dfRadio(float2 origin,float2 uv){return st_glyph(uv-origin,13);}
float dfKettlebell(float2 origin,float2 uv){return st_glyph(uv-origin,14);}
float dfDrop(float2 origin,float2 uv){return st_glyph(uv-origin,15);}
float dfRadiation(float2 origin,float2 uv){return st_glyph(uv-origin,16);}
// Length of a number in digits
float numberLength(float n) { return floor(max(log(n) / log(10.0), 0.0) + 1.0) + 2.0; }