#include "common.hlsli"
struct PreviewOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float depth : TEXCOORD1;
    float4 color : COLOR0;
};
PreviewOutput main(v_TL input)
{
    PreviewOutput output;
    // COP-ENG-114: engine UI preview tag. Coordinates never enter world space.
    bool cameraRelative = input.Color.a == 0;
    output.position = cameraRelative ? mul(m_P, input.P) : mul(m_WVP, input.P);
    output.uv = input.Tex0;
    output.depth = cameraRelative ? -input.P.z : mul(m_WV, input.P).z;
    output.color = input.Color.bgra;
    if (cameraRelative) output.color.a = 1;
    return output;
}
