#include "ProbeHelper.hlsl"
#include "LightingHelper.hlsli"
#include "RaytracingCommons.hlsli"
#include "Irradiance.hlsl"

RWTexture2D<float4> RayData : register(u0);

[shader("raygeneration")]
void RayGen()
{
    uint2 dispatchIndex = DispatchRaysIndex().xy;
    
    int rayIndex = dispatchIndex.x;
    int probeIndex = dispatchIndex.y;
    
    int3 probeCoords = GetProbeCoords(probeIndex, g_RaytracePerFrame.ProbeCounts);
    
    float3 probeCoordsW = GetProbeCoordsWorld(probeCoords, g_RaytracePerFrame.VolumePosition, g_RaytracePerFrame.ProbeOffsets, g_RaytracePerFrame.ProbeSpacing, g_RaytracePerFrame.ProbeCounts);
    
    probeIndex = GetOffsettedProbeIndex(probeCoords, g_RaytracePerFrame.ProbeCounts, g_RaytracePerFrame.ProbeOffsets);
    
    float3 directionW = GetProbeRayDirection(rayIndex, g_RaytracePerFrame.RaysPerProbe, g_RaytracePerFrame.RayRotation);
    
    uint2 texCoords = uint2(rayIndex, probeIndex);
    
    // Trace the ray.
    // Set the ray's extents.
    RayDesc ray;
    ray.Origin = probeCoordsW;
    ray.Direction = directionW;
    ray.TMin = 0.0f;
    ray.TMax = g_RaytracePerFrame.MaxRayDistance;
    
    PackedPayload packedPayload = (PackedPayload)0;
    
    TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, packedPayload);

    if (packedPayload.HitDistance < 0.0f)   //If missed
    {
        StoreRayMiss(texCoords, g_RaytracePerFrame.RayDataFormat, g_RaytracePerFrame.MissRadiance, RayData);
        
        return;
    }
    
    Payload payload = UnpackPayload(packedPayload);
    
    if (payload.HitType == HIT_KIND_TRIANGLE_BACK_FACE) //If hit backface
    {
        StoreRayBackfaceHit(texCoords, packedPayload.HitDistance, g_RaytracePerFrame.RayDataFormat, RayData);
        return;
    }
    
    float3 diffuseLight = CalculateDirectLight(payload);
    float3 surfaceBias = GetSurfaceBias(payload.NormalW, directionW, g_RaytracePerFrame.NormalBias, g_RaytracePerFrame.ViewBias);
    float blendWeight = GetBlendWeight(payload.PosW, g_RaytracePerFrame.VolumePosition, g_RaytracePerFrame.ProbeOffsets, g_RaytracePerFrame.ProbeSpacing, g_RaytracePerFrame.ProbeCounts);
    
    float4 irradiance = float4(0, 0, 0, -1);
    
    if(blendWeight > 0.0f)
    {
        irradiance = GetIrradiance(payload.PosW, surfaceBias, payload.NormalW, g_RaytracePerFrame);
        
        irradiance.xyz *= blendWeight;
    }
    
    float3 radiance = diffuseLight + ((min(payload.Albedo, float3(0.9f, 0.9f, 0.9f)) * irradiance.xyz) / PI);
    
    StoreRayFrontfaceHit(texCoords, radiance, payload.HitDistance, g_RaytracePerFrame.RayDataFormat, RayData);
}