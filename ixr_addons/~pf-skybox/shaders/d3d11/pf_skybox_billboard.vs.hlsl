#include "pf_skybox_shared.hlsli"
struct SBInstance { float4 positionHeight; float4 rotationWidthType; };
StructuredBuffer<SBInstance> sb_instances : register(t0);
float2 sbDirection(float2 v) { return v * rsqrt(max(dot(v,v), 0.00000001)); }
SBVarying main(SBInput input, uint instanceID : SV_InstanceID)
{
    SBInstance item = sb_instances[(uint)sb_params.y + instanceID];
    float2 direction = sbDirection(sb_camera_scale.xz - item.positionHeight.xz);
    float2 oldDirection = sbDirection(sb_old_camera.xz - item.positionHeight.xz);
    float horizontal = input.position.x * item.positionHeight.w * item.rotationWidthType.z;
    float vertical = input.position.y * item.positionHeight.w;
    float3 p = item.positionHeight.xyz + float3(direction.y * horizontal, vertical, -direction.x * horizontal);
    float3 oldP = item.positionHeight.xyz + float3(oldDirection.y * horizontal, vertical, -oldDirection.x * horizontal);
    SBVarying output = sbVertex(input, p, oldP, float3(0.0,1.0,0.0));
    float angle = atan2(direction.y, direction.x) - atan2(item.rotationWidthType.x, item.rotationWidthType.y);
    float frame = frac(angle / 6.28318530718) * 8.0;
    float first = floor(frame), second = fmod(first + 1.0, 8.0);
    float2 within = (input.uv * 63.0 + 0.5) / 512.0;
    output.uv = within + float2(first, item.rotationWidthType.w) / 8.0;
    output.extra = float3(within + float2(second, item.rotationWidthType.w) / 8.0, frac(frame));
    return output;
}
