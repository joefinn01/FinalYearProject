#include "RasterCommons.hlsli"
#include "LightingHelper.hlsli"

struct PS_INPUT
{
    float4 PosH : SV_POSITION;
    float2 TexCoords : TEXCOORD;
};

ConstantBuffer<DeferredPerFrameCB> l_DeferredPerFrameCB : register(b1);

float4 main(PS_INPUT input) : SV_TARGET
{
    uint primID = UintTex2DTable[l_DeferredPerFrameCB.PrimitiveIndexIndex].Load(int3(input.TexCoords.x * g_ScenePerFrameCB.ScreenWidth, input.TexCoords.y * g_ScenePerFrameCB.ScreenHeight, 0));
    float2 uv = Float2Tex2DTable[l_DeferredPerFrameCB.TexCoordIndex].Sample(SamLinearClamp, input.TexCoords);
    float3 normalW = (Float3Tex2DTable[l_DeferredPerFrameCB.NormalIndex].Sample(SamLinearClamp, input.TexCoords) * 2.0f) - 1.0f;
    float depth = FloatTex2DTable[l_DeferredPerFrameCB.NormalIndex].Sample(SamLinearClamp, input.TexCoords);

    //Get copy of this primitives geometry information
    PrimitiveInstanceCB geomInfo = g_PrimitivePerInstanceCB[g_ScenePerFrameCB.PrimitivePerInstanceIndex][primID];
    PrimitivePerFrameCB primInfo = g_PrimitivePerFrameCB[g_ScenePerFrameCB.PrimitivePerFrameIndex][primID];
    
    //Calculate world pos by transforming from clip space
    float4 hitPosW = float4(input.TexCoords.x * 2.0f - 1.0f, input.TexCoords.y * 2.0f - 1.0f, depth, 1.0f);
    hitPosW.y *= -1.0f;
	
    hitPosW = mul(hitPosW, g_ScenePerFrameCB.InvViewProjection);
    hitPosW = hitPosW / hitPosW.w;

    float3 viewW = normalize(g_ScenePerFrameCB.EyePosW.xyz - hitPosW.xyz);
    
    //Get primitive information from textures
    float3 albedo = pow(Tex2DTable[geomInfo.AlbedoIndex].SampleLevel(SamAnisotropicWrap, uv, 0).rgb, 2.2f).xyz;

#if !NO_METALLIC_ROUGHNESS

#if NORMAL_MAPPING
    float3 tangentW = (Float3Tex2DTable[l_DeferredPerFrameCB.TangentIndex].Sample(SamLinearClamp, input.TexCoords) * 2.0f) - 1.0f;
    tangentW = normalize(tangentW - dot(tangentW, normalW) * normalW);
    
    normalW = GetNormal(uv, normalW, tangentW, Tex2DTable[g_PrimitivePerInstanceCB[g_ScenePerFrameCB.PrimitivePerInstanceIndex][primID].NormalIndex], SamLinearClamp);
#endif
    
    float fMetallic = Tex2DTable[geomInfo.MetallicRoughnessIndex].SampleLevel(SamPointWrap, uv, 0).b;
    float fRoughness = Tex2DTable[geomInfo.MetallicRoughnessIndex].SampleLevel(SamAnisotropicWrap, uv, 0).g;
    
#if OCCLUSION_MAPPING
    float fOcclusion = Tex2DTable[geomInfo.MetallicRoughnessIndex].SampleLevel(SamPointWrap, uv, 0).r;
#endif
    
    float3 outgoingRadiance = float3(0.0f, 0.0f, 0.0f);
    
    float3 refelctance0 = float3(0.04f, 0.04f, 0.04f);
    refelctance0 = lerp(refelctance0, albedo, fMetallic);
    
    for (int i = 0; i < g_ScenePerFrameCB.NumLights; ++i)
    {
        float3 light = g_LightCB[g_ScenePerFrameCB.LightIndex][i].Position - hitPosW.xyz;
        
        //Cache distance so not done again when normalizing
        float fDistance = length(light);
        
        light /= fDistance;
        
        float3 halfVec = normalize(viewW + light);

        float fAttenuation = min(1.0f / dot(g_LightCB[g_ScenePerFrameCB.LightIndex][i].Attenuation[0], float3(1.0f, g_LightCB[g_ScenePerFrameCB.LightIndex][i].Attenuation[1] * fDistance, g_LightCB[g_ScenePerFrameCB.LightIndex][i].Attenuation[2] * fDistance * fDistance)), 1.0f);

        float3 radiance = g_LightCB[g_ScenePerFrameCB.LightIndex][i].Color.xyz * fAttenuation;

        float3 fresnel = FresnelSchlick(max(dot(halfVec, viewW), 0.0f), refelctance0);
        float fNDF = NormalDistribution(normalW, halfVec, fRoughness);
        float fGeomDist = GeometryDistribution(normalW, viewW, light, fRoughness);

        //Add 0.0001f to prevent divide by zero
        float3 specular = (fNDF * fGeomDist * fresnel) / (4.0f * max(dot(normalW, viewW), 0.0f) * max(dot(normalW, light), 0.0f) + 0.0001f);
        
        float3 fKD = (float3(1.0f, 1.0f, 1.0f) - fresnel) * (1.0f - fMetallic);
        
        float fNormDotLight = max(dot(normalW, light), 0.0f);
        outgoingRadiance += ((fKD * albedo / PI) + specular) * radiance * fNormDotLight;
    }
#endif
    
    float4 color = float4(0, 0, 0, 1);
    
#if OCCLUSION_MAPPING
    color = float4((float3(0.03f, 0.03f, 0.03f) * fOcclusion * albedo) + outgoingRadiance, 1.0f);
#elif NO_METALLIC_ROUGHNESS
    color = float4(albedo, 1.0f);
#else
    color = float4((float3(0.03f, 0.03f, 0.03f) * albedo) + outgoingRadiance, 1.0f);
#endif
    
    //Map and gamma correct
    float fMapping = 1.0f / 2.2f;
    
    color.xyz /= (color.xyz + float3(1.0f, 1.0f, 1.0f));
    color.xyz = pow(color.xyz, float3(fMapping, fMapping, fMapping));
    
    return color;
}