#include "RasterCommons.hlsli"
#include "LightingHelper.hlsli"
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
    float3 normalW = normalize((Tex2DTable[l_DeferredPerFrameCB.NormalIndex].Sample(SamLinearClamp, input.TexCoords).xyz * 2.0f) - 1.0f);
    float depth = Tex2DTable[l_DeferredPerFrameCB.DepthIndex].Sample(SamLinearClamp, input.TexCoords).r;
    float3 albedo = pow(Tex2DTable[l_DeferredPerFrameCB.AlbedoIndex].SampleLevel(SamAnisotropicWrap, input.TexCoords, 0).rgb, 2.2f).xyz;
    float3 fMetallicRoughnessOcclusion = Tex2DTable[l_DeferredPerFrameCB.MetallicRoughnessOcclusion].SampleLevel(SamPointWrap, input.TexCoords, 0).xyz;
    
    //Calculate world pos by transforming from clip space
    float4 hitPosW = float4(input.TexCoords.x * 2.0f - 1.0f, input.TexCoords.y * 2.0f - 1.0f, depth, 1.0f);
    hitPosW.y *= -1.0f;
	
    hitPosW = mul(hitPosW, g_ScenePerFrameCB.InvViewProjection);
    hitPosW = hitPosW / hitPosW.w;

    float3 viewW = normalize(g_ScenePerFrameCB.EyePosW.xyz - hitPosW.xyz);
    
    float3 outgoingRadiance = float3(0.0f, 0.0f, 0.0f);
    
    float3 refelctance0 = float3(0.04f, 0.04f, 0.04f);
    refelctance0 = lerp(refelctance0, albedo, fMetallicRoughnessOcclusion.r);
    
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
        float fNDF = NormalDistribution(normalW, halfVec, fMetallicRoughnessOcclusion.g);
        float fGeomDist = GeometryDistribution(normalW, viewW, light, fMetallicRoughnessOcclusion.g);

        //Add 0.0001f to prevent divide by zero
        float3 specular = (fNDF * fGeomDist * fresnel) / (4.0f * max(dot(normalW, viewW), 0.0f) * max(dot(normalW, light), 0.0f) + 0.0001f);
        
        float3 fKD = (float3(1.0f, 1.0f, 1.0f) - fresnel) * (1.0f - fMetallicRoughnessOcclusion.r);
        
        float fNormDotLight = max(dot(normalW, light), 0.0f);
        outgoingRadiance += ((fKD * albedo / PI) + specular) * radiance * fNormDotLight;
    }
    
    float4 color = float4((float3(0.03f, 0.03f, 0.03f) * fMetallicRoughnessOcclusion.b * albedo) + outgoingRadiance, 1.0f);
    
    float blendWeight = GetBlendWeight(hitPosW.xyz, l_RaytracePerFrameCB.VolumePosition, l_RaytracePerFrameCB.ProbeSpacing, l_RaytracePerFrameCB.ProbeCounts);
    float3 surfaceBias = GetSurfaceBias(normalW, -viewW, l_RaytracePerFrameCB.NormalBias, l_RaytracePerFrameCB.ViewBias);
    float4 indirectLight = GetIrradiance(hitPosW.xyz, surfaceBias, normalW, l_RaytracePerFrameCB);
    
    if (blendWeight > 0.0f)
    {
#if !SHOW_INDIRECT
        color.rgb = (albedo.rgb * indirectLight.rgb * blendWeight) / PI;
#else
        color.rgb += (albedo.rgb * indirectLight.rgb * blendWeight) / PI;
#endif
    }
    
    //Map and gamma correct
    float fMapping = 1.0f / 2.2f;
    
    color.xyz /= (color.xyz + float3(1.0f, 1.0f, 1.0f));
    color.xyz = pow(color.xyz, float3(fMapping, fMapping, fMapping));
    
    return color;
}