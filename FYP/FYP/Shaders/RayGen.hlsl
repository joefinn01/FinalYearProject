#include "ProbeHelper.hlsl"
#include "RaytracedLightingHelper.hlsl"

[shader("raygeneration")]
void RayGen()
{
    uint2 dispatchIndex = DispatchRaysIndex().xy;
    
    int rayIndex = dispatchIndex.x;
    int probeIndex = dispatchIndex.y;
    
    int3 probeCoords = GetProbeCoords(probeIndex);
    
    float3 probeCoordsW = GetProbeCoordsWorld(probeCoords);
    
    float3 directionW = GetProbeRayDirection(rayIndex);
    
    uint2 texCoords = uint2(rayIndex, probeIndex);
    
    // Trace the ray.
    // Set the ray's extents.
    RayDesc ray;
    ray.Origin = probeCoordsW;
    ray.Direction = directionW;
    ray.TMin = 0.001;
    ray.TMax = g_RaytracePerFrame.MaxRayDistance;
    
    PackedPayload packedPayload = (PackedPayload)0;
    
    TraceRay(Scene, RAY_FLAG_NONE, ~0, 0, 1, 0, ray, packedPayload);

    if (packedPayload.HitDistance < 0.0f)   //If missed
    {
        StoreRayMiss(texCoords);
        
        return;
    }
    
    Payload payload = UnpackPayload(packedPayload);
    
    if (payload.HitType == HIT_TYPE_TRIANGLE_BACK_FACE) //If hit backface
    {
        StoreRayBackfaceHit(texCoords, packedPayload.HitDistance);
        return;
    }
    
    float3 diffuseLight = CalculateDirectDiffuseLight(payload);
    float3 surfaceBias = GetSurfaceBias(payload.NormalW, directionW);
    float blendWeight = GetBlendWeight(payload.PosW);
    
    float3 irradiance = float3(0, 0, 0);
    
    if(blendWeight > 0.0f)
    {
        irradiance = GetIrradiance(payload.PosW, surfaceBias, payload.NormalW);
        
        irradiance *= blendWeight;
    }
    
    float3 radiance = diffuseLight + ((min(payload.Albedo, float3(0.9f, 0.9f, 0.9f)) * irradiance) / PI);
    
    StoreRayFrontfaceHit(texCoords, radiance, payload.HitDistance);
}