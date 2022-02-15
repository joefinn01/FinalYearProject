#include "RasterCommons.hlsli"

struct PS_INPUT
{
    float4 PosH : SV_POSITION;
    float3 NormalW : NORMAL;
    float2 TexCoords : TEXCOORD;
    float3 TangentW : TANGENT;
};

struct PS_OUTPUT
{
    float4 NormalW : SV_TARGET0;
    float4 TangentW : SV_TARGET1;
    float2 TexCoords : SV_TARGET2;
    uint PrimitiveID : SV_Target3;
};

ConstantBuffer<PrimitiveIndexCB> g_PrimitiveIndexCB : register(b1);

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    
    output.NormalW = float4((normalize(input.NormalW) + 1) * 0.5f, 0.0f);
    output.TangentW = float4((normalize(input.TangentW) + 1) * 0.5f, 0.0f);
    output.TexCoords = input.TexCoords;
    output.PrimitiveID = g_PrimitiveIndexCB.Index;
    
    return output;
}