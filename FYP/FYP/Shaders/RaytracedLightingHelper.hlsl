#ifndef RAYTRACED_LIGHTING_HELPER_HLSL
#define RAYTRACED_LIGHTING_HELPER_HLSL

#include "RaytracingCommons.hlsli"
#include "Payloads.hlsli"
#include "Defines.hlsli"
#include "LightingHelper.hlsli"

float CheckLightVisibility(Payload payload, float maxDistance, float3 lightVec)
{
    RayDesc ray;
    ray.Origin = payload.PosW + (g_RaytracePerFrame.NormalBias * payload.NormalW);
    ray.Direction = normalize(lightVec);
    ray.TMin = 0.0f;
    ray.TMax = maxDistance;
    
    PackedPayload packedPayload = (PackedPayload) 0;
    
    TraceRay(Scene, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 0xFF, 0, 1, 0, ray, packedPayload);

    return packedPayload.HitDistance < 0.0f;
}

float3 CalculateDirectLight(Payload payload)
{
    float3 directLight = 0;

    float3 refelctance0 = float3(0.04f, 0.04f, 0.04f);
    refelctance0 = lerp(refelctance0, payload.Albedo, payload.Metallic.r);
    
    float3 viewW = normalize(g_ScenePerFrameCB.EyePosW - payload.PosW);
    
    for (int i = 0; i < g_ScenePerFrameCB.NumLights; ++i)
    {
        float3 light = g_LightCB[g_ScenePerFrameCB.LightIndex][i].Position - payload.PosW;
        
        //Cache distance so not done again when normalizing
        float fDistance = length(light);
        
        float visibility = CheckLightVisibility(payload, fDistance - g_RaytracePerFrame.ViewBias, light);
        
        if (visibility <= 0.0f)
        {
            continue;
        }
        
        light /= fDistance;
        
        float3 halfVec = normalize(viewW + light);

        float fAttenuation = min(1.0f / dot(g_LightCB[g_ScenePerFrameCB.LightIndex][i].Attenuation[0], float3(1.0f, g_LightCB[g_ScenePerFrameCB.LightIndex][i].Attenuation[1] * fDistance, g_LightCB[g_ScenePerFrameCB.LightIndex][i].Attenuation[2] * fDistance * fDistance)), 1.0f);

        float3 radiance = g_LightCB[g_ScenePerFrameCB.LightIndex][i].Color.xyz * fAttenuation;

        float3 fresnel = FresnelSchlick(max(dot(halfVec, viewW), 0.0f), refelctance0);
        float fNDF = NormalDistribution(payload.ShadingNormalW, halfVec, payload.Roughness);
        float fGeomDist = GeometryDistribution(payload.ShadingNormalW, viewW, light, payload.Roughness);

        //Add 0.0001f to prevent divide by zero
        float3 specular = (fNDF * fGeomDist * fresnel) / (4.0f * max(dot(payload.ShadingNormalW, viewW), 0.0f) * max(dot(payload.ShadingNormalW, light), 0.0f) + 0.0001f);
        
        float3 fKD = (float3(1.0f, 1.0f, 1.0f) - fresnel) * (1.0f - payload.Metallic);
        
        float fNormDotLight = max(dot(payload.ShadingNormalW, light), 0.0f);
        directLight += ((fKD * payload.Albedo / PI) + specular) * radiance * fNormDotLight;
    }

    directLight = (float3(0.03f, 0.03f, 0.03f) * payload.Occlusion * payload.Albedo) + directLight;
    
    return directLight;
}

#endif