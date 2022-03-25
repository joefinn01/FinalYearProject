#include "ProbeHelper.hlsl"
#include "RaytracingCommons.hlsli"

#if BLEND_RADIANCE
RWTexture2D<float4> IrradianceData : register(u1);
#else
RWTexture2D<float4> DistanceData : register(u1);
#endif

#if BLEND_RADIANCE
groupshared float3 RayRadiances[BLEND_RAYS_PER_PROBE];
#endif

groupshared float3 RayDirections[BLEND_RAYS_PER_PROBE];
groupshared float RayDistances[BLEND_RAYS_PER_PROBE];

[numthreads(NUM_TEXELS_PER_PROBE, NUM_TEXELS_PER_PROBE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
    float4 result = float4(0, 0, 0, 0);
    
    int numProbes = g_RaytracePerFrame.ProbeCounts.x * g_RaytracePerFrame.ProbeCounts.y * g_RaytracePerFrame.ProbeCounts.z;
    int probeIndex = GetProbeIndex(DTid.xy, NUM_TEXELS_PER_PROBE, g_RaytracePerFrame.ProbeCounts);

    if (probeIndex >= numProbes || probeIndex < 0)
    {
        return;
    }

    uint2 atlasCoords = uint2(1, 1) + DTid.xy + (DTid.xy / NUM_TEXELS_PER_PROBE) * 2;
    
    int3 probeCoords = GetProbeCoords(probeIndex, g_RaytracePerFrame.ProbeCounts);
    
    bool clearedPlane = false;
    
#if BLEND_RADIANCE
    clearedPlane |= ClearScrolledPlane(atlasCoords, probeCoords, 0, g_RaytracePerFrame.ProbeOffsets, g_RaytracePerFrame.ProbeCounts, g_RaytracePerFrame.ClearPlane, IrradianceData);
    clearedPlane |= ClearScrolledPlane(atlasCoords, probeCoords, 1, g_RaytracePerFrame.ProbeOffsets, g_RaytracePerFrame.ProbeCounts, g_RaytracePerFrame.ClearPlane, IrradianceData);
    clearedPlane |= ClearScrolledPlane(atlasCoords, probeCoords, 2, g_RaytracePerFrame.ProbeOffsets, g_RaytracePerFrame.ProbeCounts, g_RaytracePerFrame.ClearPlane, IrradianceData);
#else
    clearedPlane |= ClearScrolledPlane(atlasCoords, probeCoords, 0, g_RaytracePerFrame.ProbeOffsets, g_RaytracePerFrame.ProbeCounts, g_RaytracePerFrame.ClearPlane, DistanceData);
    clearedPlane |= ClearScrolledPlane(atlasCoords, probeCoords, 1, g_RaytracePerFrame.ProbeOffsets, g_RaytracePerFrame.ProbeCounts, g_RaytracePerFrame.ClearPlane, DistanceData);
    clearedPlane |= ClearScrolledPlane(atlasCoords, probeCoords, 2, g_RaytracePerFrame.ProbeOffsets, g_RaytracePerFrame.ProbeCounts, g_RaytracePerFrame.ClearPlane, DistanceData);
#endif

    if(clearedPlane == true)
    {
        return;
    }
    
    float2 octCoords = GetNormalizedOctahedralCoords(DTid.xy, NUM_TEXELS_PER_PROBE);
    float3 direction = GetOctahedralDirection(octCoords);
    
    //Load all necessary data into shared memory
    
    int totalIterations = int(ceil(BLEND_RAYS_PER_PROBE / float(NUM_TEXELS_PER_PROBE * NUM_TEXELS_PER_PROBE)));

    int i;
    for (i = 0; i < totalIterations; ++i)
    {
        int rayIndex = (totalIterations * groupIndex) + i;

        if (rayIndex >= BLEND_RAYS_PER_PROBE)
        {
            break;
        }

#if BLEND_RADIANCE
        RayRadiances[rayIndex] = GetRayRadiance(uint2(rayIndex, probeIndex), g_RaytracePerFrame.RayDataFormat, Tex2DTable[g_RaytracePerFrame.RayDataIndex]);
#endif
        
        RayDistances[rayIndex] = GetRayDistance(uint2(rayIndex, probeIndex), g_RaytracePerFrame.RayDataFormat, Tex2DTable[g_RaytracePerFrame.RayDataIndex]);
        RayDirections[rayIndex] = GetRayDirection(rayIndex, g_RaytracePerFrame.RaysPerProbe, g_RaytracePerFrame.RayRotation);
    }

    GroupMemoryBarrierWithGroupSync();
    
#if BLEND_RADIANCE
    uint numBackfaceHits = 0;
    uint maxBackfaceHits = g_RaytracePerFrame.RaysPerProbe * 0.1f;
#endif
    
    for (i = 0; i < g_RaytracePerFrame.RaysPerProbe; ++i)
    {
        float weight = max(0.0f, dot(direction, RayDirections[i]));

#if BLEND_RADIANCE
        if(RayDistances[i] < 0.0f)
        {
            ++numBackfaceHits;
            
            if(numBackfaceHits >= maxBackfaceHits)
            {
                return;
            }
            
            continue;
        }
        
        float3 radianceTest = RayRadiances[i];
        
        result += float4(radianceTest * weight, weight);
#else
        float maxRayDistance = length(g_RaytracePerFrame.ProbeSpacing) * 1.5f;
        
        weight = pow(weight, g_RaytracePerFrame.DistancePower);
        
        float rayDistance = min(abs(RayDistances[i]), maxRayDistance);
        
        result += float4(rayDistance * weight, rayDistance * rayDistance * weight, 0, weight);
#endif
    }
    
    float epsilon = float(g_RaytracePerFrame.RaysPerProbe) * 1e-9f;
    
    result.rgb *= 1.0f / (2.0f * max(result.a, epsilon));

    float hysteresis = g_RaytracePerFrame.Hysteresis;
    
#if BLEND_RADIANCE
    float3 previous = IrradianceData[atlasCoords].rgb;
    
    //Tonemap
    result.rgb = pow(result.rgb, 1.0f / g_RaytracePerFrame.IrradianceGammaEncoding);
    
    float3 delta = result.rgb - previous.rgb;
    
    if (max(max(previous.r - result.r, previous.g - result.g), previous.b - result.b) > g_RaytracePerFrame.IrradianceThreshold)
    {
        hysteresis = max(0, hysteresis - 0.75f);
    }
    
    if (length(delta) > g_RaytracePerFrame.BrightnessThreshold)
    {
        result.rgb = previous + (delta * 0.25f);
    }
    
    static const float c_threshold = 1.0f / 1024.0f;
    float3 lerpDelta = (1.0f - hysteresis) * delta;
    
    if (max(max(result.r, result.g), result.b) < max(max(previous.r, previous.g), previous.b))
    {
        lerpDelta = min(max(c_threshold, abs(lerpDelta)), abs(delta) * sign(lerpDelta));
    }
    
    IrradianceData[atlasCoords] = float4(previous + lerpDelta, 1.0f);
#else
    float3 previous = DistanceData[atlasCoords].rgb;
    
    DistanceData[atlasCoords] = float4(lerp(result.rg, previous.rg, hysteresis), 0.0f, 1.0f);
#endif
}