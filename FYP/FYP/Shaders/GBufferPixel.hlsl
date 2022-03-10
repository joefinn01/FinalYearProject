#include "RasterCommons.hlsli"
#include "LightingHelper.hlsli"

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
    float4 Albedo : SV_TARGET1;
    float4 MetallicRoughnessOcclusion : SV_TARGET2;
};

ConstantBuffer<PrimitiveIndexCB> l_PrimitiveIndexCB : register(b1);

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    
    PrimitiveInstanceCB geomInfo = g_PrimitivePerInstanceCB[g_ScenePerFrameCB.PrimitivePerInstanceIndex][l_PrimitiveIndexCB.PrimitiveIndex];
    
    float3 tangentW = normalize(input.TangentW);
    float3 normalW = normalize(input.NormalW);
    
#if NORMAL_MAPPING    
    normalW = GetNormal(input.TexCoords, normalW, tangentW, Tex2DTable[geomInfo.NormalIndex], SamPointWrap);
#endif
    
    output.NormalW.xyz = (normalW + 1) * 0.5f;
    output.Albedo = Tex2DTable[geomInfo.AlbedoIndex].SampleLevel(SamPointWrap, input.TexCoords, 0);
    
#if !NO_METALLIC_ROUGHNESS
    output.MetallicRoughnessOcclusion.xy = Tex2DTable[geomInfo.MetallicRoughnessIndex].SampleLevel(SamPointWrap, input.TexCoords, 0).bg;
#else
    output.MetallicRoughnessOcclusion.x = 0.5f;
    output.MetallicRoughnessOcclusion.y = 0.5f;
#endif
    
#if OCCLUSION_MAPPING
    output.MetallicRoughnessOcclusion.z = Tex2DTable[geomInfo.MetallicRoughnessIndex].SampleLevel(SamPointWrap, input.TexCoords, 0).r;
#else
    output.MetallicRoughnessOcclusion.z = 1.0f;
#endif
    
    return output;
}