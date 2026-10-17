// name5go: bodycam like effect, 2024-07-17. IX-Ray COP-ENG-168 binding.
#define float3_color_black float3 (0.0,0.0,0.0)
/*
pf_bodycam_state.x = xx.xyyzz 
                   x.xx = bodycam_zoom (1.00)
                   yy.  = bodycam_circle_ratio_x (10)
                   zz.  = bodycam_circle_ratio_y (10)

pf_bodycam_state.y = xx.xyyy
                   x.xx = bodycam_distortion (1.00)
                   y.yy = bodycam_distortion_cubic (1.00)

pf_bodycam_state.z = xx.xxyyyy
                   x.xxx = bodycam_circle_radius (1.000)
                   y.yyy = bodycam_circle_blur (1.000)

pf_bodycam_state.w = xx.yyyyyy
                   0.00xx = bodycam_ca_param (0.0001)
                   yy.yyyy = bodycam_elapsed_time(10.0000) 
*/
uniform float4 pf_bodycam_state;

float3 bodycam(float3 image, float2 texcoord)
{
  //BodyCamParam
  float4 lua_param = pf_bodycam_state;
  
  float bodycam_zoom             = 1.0 / (floor(lua_param.x * 10.0) * 0.01);
  float bodycam_circle_ratio_x   = floor(frac(lua_param.x * 10.0) * 100.0);
  float bodycam_circle_ratio_y   = frac(frac(lua_param.x * 10.0) * 100.0) * 100.0;
  float bodycam_distortion       = floor(lua_param.y * 10.0) * 0.01;
  float bodycam_distortion_cubic = frac(lua_param.y * 10.0) * 100.0;
  float bodycam_circle_radius    = floor(lua_param.z * 100.0) * 0.0001;
  float bodycam_circle_blur      = frac(lua_param.z * 100.0) * 10.0;
  float bodycam_ca_param         = floor(lua_param.w) * 0.0001;
  float bodycam_elapsed_time     = frac(lua_param.w) * 10.0;

  //Center
  float2 uv = texcoord.xy;
  float2 shake = 0.0;
  shake.x = sin(bodycam_elapsed_time * 100.0 ) * 0.01;
  shake.y = cos(bodycam_elapsed_time * 100.0 ) * 0.02;
  // User-requested stabilization: 20% motion for both image and lens border.
  shake *= 0.2;


  uv -= shake * 0.175;

  float2 center = 0.5;


  float2 bodycam_uv = uv;
  float2 uv_center = bodycam_uv - center;
  
  //BodyCamDistortion
  float r = (bodycam_distortion_cubic == 0.0) ? (bodycam_uv.y - center.y) : (length(bodycam_uv - center));
  float f = 1.0 + r * (bodycam_distortion + bodycam_distortion_cubic);

  bodycam_uv = (uv_center * 0.5) * f * bodycam_zoom + center;

  //IsOutDisplay?
  bool is_out_display = (bodycam_uv.x > 1.0 || bodycam_uv.x < 0.0) || (bodycam_uv.y > 1.0 || bodycam_uv.y < 0.0);
  
  //IsOutCircle?
  float2 circle_center = (uv + shake - center);
  circle_center.x /= (bodycam_circle_ratio_y / bodycam_circle_ratio_x);

  float circle_radius = length(circle_center);
  bool is_out_circle = circle_radius > bodycam_circle_radius;

  //Blur
  bool is_out_of_render = (is_out_display || is_out_circle);
  float blur = (circle_radius - bodycam_circle_radius + bodycam_circle_blur) / bodycam_circle_blur;
  blur = clamp(blur , 0.0 , 1.0);
  blur = blur * blur * blur * blur;
  float blur_render_param = is_out_of_render ? 0.0 : 1.0 - blur;

  //Result
  float red = s_image.SampleLevel(smp_rtlinear, bodycam_uv + bodycam_ca_param, 0.0).x;
  float blue = s_image.SampleLevel(smp_rtlinear, bodycam_uv - bodycam_ca_param, 0.0).z;
  float3 color =  s_image.SampleLevel(smp_rtlinear, bodycam_uv, 0.0).xyz;
  color.x = (color.x + red) * 0.5;
  color.z = (color.z + blue) * 0.5;
  float3 black = float3_color_black;
  black.z = blue * (blur_render_param);

  float3 result = lerp(black , color , blur_render_param);



  return result;
}