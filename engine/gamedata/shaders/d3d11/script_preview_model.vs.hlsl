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
    output.position = mul(m_WVP, input.P);
    output.uv = input.Tex0;
    output.depth = mul(m_WV, input.P).z;
    output.color = input.Color.bgra;
    return output;
}
