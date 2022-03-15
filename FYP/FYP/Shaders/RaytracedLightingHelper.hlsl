#include "RaytracingCommons.hlsli"

float CheckLightVisibility(Payload payload, float maxDistance, float3 lightVec)
{
    RayDesc ray;
    ray.Origin = payload.PosW + (g_RaytracePerFrame.NormalBias * payload.NormalW);
    ray.Direction = normalize(lightVec);
    ray.TMin = 0.0f;
    ray.TMax = maxDistance;
    
    PackedPayload packedPayload = (PackedPayload) 0;
    
    TraceRay(Scene, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, ~0, 0, 1, 0, ray, packedPayload);

    return packedPayload.HitDistance < 0.0f;
}

float3 CalculateDirectDiffuseLight(Payload payload)
{
    float3 brdf = payload.Albedo / PI;
    float3 directLight = 0;

    for (int i = 0; i < g_ScenePerFrameCB.NumLights; ++i)
    {
        float3 light = g_LightCB[g_ScenePerFrameCB.LightIndex][i].Position - payload.PosW;
        
        //Cache distance so not done again when normalizing
        float distance = length(light);
        
        float visibility = CheckLightVisibility(payload, distance - g_RaytracePerFrame.ViewBias, light);
        
        if(visibility <= 0.0f)
        {
            return float3(0, 0, 0);

        }
        
        light /= distance;
        
        float nDotL = max(dot(payload.NormalW, light), 0.0f);
        float attenuation = min(1.0f / dot(g_LightCB[g_ScenePerFrameCB.LightIndex][i].Attenuation[0], float3(1.0f, g_LightCB[g_ScenePerFrameCB.LightIndex][i].Attenuation[1] * distance, g_LightCB[g_ScenePerFrameCB.LightIndex][i].Attenuation[2] * distance * distance)), 1.0f);

        directLight += g_LightCB[g_ScenePerFrameCB.LightIndex][i].Color.xyz * attenuation * nDotL * visibility;
    }

    return directLight * brdf;
}