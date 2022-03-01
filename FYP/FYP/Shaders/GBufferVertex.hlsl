#include "RasterCommons.hlsli"

struct VS_INPUT
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float3 TangentL : TANGENT;
    float2 TexCoords : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 PosH : SV_POSITION;
    float3 NormalW : NORMAL;
    float2 TexCoords : TEXCOORD;
    float3 TangentW : TANGENT;
};

ConstantBuffer<PrimitiveIndexCB> l_PrimitiveIndexCB : register(b1);

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT result;

    GameObjectPerFrameCB primPerFrameCB = g_PrimitivePerFrameCB[g_ScenePerFrameCB.PrimitivePerFrameIndex][l_PrimitiveIndexCB.InstanceIndex];
    
    result.PosH = mul(float4(input.PosL, 1.0f), primPerFrameCB.World);
    result.PosH = mul(result.PosH, g_ScenePerFrameCB.ViewProjection);

    result.NormalW = normalize(mul(float4(input.NormalL, 0), primPerFrameCB.InvTransposeWorld).xyz);
    result.TangentW = normalize(mul(float4(input.TangentL, 0), primPerFrameCB.InvTransposeWorld).xyz);
    
    result.TexCoords = input.TexCoords;
    
    return result;
}