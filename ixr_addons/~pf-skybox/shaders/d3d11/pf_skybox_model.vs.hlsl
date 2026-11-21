#include "pf_skybox_shared.hlsli"
struct SBInstance { float4 positionHeight; float4 rotationWidthType; };
StructuredBuffer<SBInstance> sb_instances : register(t0);
SBVarying main(SBInput input, uint instanceID : SV_InstanceID)
{
    SBInstance item = sb_instances[(uint)sb_params.y + instanceID];
    float sine = item.rotationWidthType.x, cosine = item.rotationWidthType.y;
    float3 p = float3(cosine * input.position.x + sine * input.position.z,
        input.position.y, -sine * input.position.x + cosine * input.position.z);
    float3 normal = float3(cosine * input.normal.x + sine * input.normal.z,
        input.normal.y, -sine * input.normal.x + cosine * input.normal.z);
    p = item.positionHeight.xyz + p * item.positionHeight.w;
    return sbVertex(input, p, p, normal);
}
