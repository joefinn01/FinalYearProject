#include "RasterCommons.hlsli"

struct VS_INPUT
{
    float3 PosH : POSITION;
    float2 TexCoords : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 PosH : SV_POSITION;
    float2 TexCoords : TEXCOORD;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    output.PosH = float4(input.PosH, 1);
    output.TexCoords = input.TexCoords;

    return output;
}