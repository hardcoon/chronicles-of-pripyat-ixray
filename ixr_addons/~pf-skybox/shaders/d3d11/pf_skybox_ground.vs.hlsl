#include "pf_skybox_shared.hlsli"
SBVarying main(SBInput input)
{
    return sbVertex(input, input.position.xyz, input.position.xyz, input.normal);
}
