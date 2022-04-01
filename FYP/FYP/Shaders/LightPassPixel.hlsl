#include "RasterCommons.hlsli"
#include "Irradiance.hlsl"

struct PS_INPUT
{
    float4 PosH : SV_POSITION;
    float2 TexCoords : TEXCOORD;
};

ConstantBuffer<DeferredPerFrameCB> l_DeferredPerFrameCB : register(b1);
ConstantBuffer<RaytracePerFrameCB> l_RaytracePerFrameCB : register(b2);

float4 main(PS_INPUT input) : SV_TARGET
{
    //Get primitive information from textures
    float3 color = Tex2DTable[l_DeferredPerFrameCB.DirectLightIndex].SampleLevel(SamLinearClamp, input.TexCoords, 0).rgb;
    
#if USE_GI
    float3 normalW = normalize((Tex2DTable[l_DeferredPerFrameCB.NormalIndex].Sample(SamLinearClamp, input.TexCoords).rgb * 2.0f) - 1.0f);
    float3 albedo = Tex2DTable[l_DeferredPerFrameCB.AlbedoIndex].SampleLevel(SamAnisotropicWrap, input.TexCoords, 0).rgb;
    float3 posW = Tex2DTable[l_DeferredPerFrameCB.PositionIndex].SampleLevel(SamLinearClamp, input.TexCoords, 0).xyz;
    
    float blendWeight = GetBlendWeight(posW, l_RaytracePerFrameCB.VolumePosition, l_RaytracePerFrameCB.ProbeOffsets, l_RaytracePerFrameCB.ProbeSpacing, l_RaytracePerFrameCB.ProbeCounts);
    float3 surfaceBias = GetSurfaceBias(normalW, normalize(posW - g_ScenePerFrameCB.EyePosW), l_RaytracePerFrameCB.NormalBias, l_RaytracePerFrameCB.ViewBias);
    float4 indirectLight = GetIrradiance(posW, surfaceBias, normalW, l_RaytracePerFrameCB);
    
    if (blendWeight > 0.0f)
    {
#if SHOW_INDIRECT
        color = (albedo * indirectLight.rgb * blendWeight) / PI;
#else
        color += (albedo * indirectLight.rgb * blendWeight) / PI;
#endif
    }
#endif    

    //Map and gamma correct
    float fMapping = 1.0f / 2.2f;
    
    color /= (color + float3(1.0f, 1.0f, 1.0f));
    color = pow(color, float3(fMapping, fMapping, fMapping));
    
    return float4(color, 1.0f);
}