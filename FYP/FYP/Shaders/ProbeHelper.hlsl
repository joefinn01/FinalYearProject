#include "RaytracingCommons.hlsli"

uint FloatToUint(float val, float scale)
{
    return (uint) floor(val * scale + 0.5f);
}

uint Float3ToUint(float3 input)
{
    return (FloatToUint(input.r, 1023.f)) | (FloatToUint(input.g, 1023.f) << 10) | (FloatToUint(input.b, 1023.f) << 20);
}

int3 GetProbeCoords(int probeIndex)
{
    int3 coords;
    
    coords.x = probeIndex % g_RaytracePerFrame.ProbeCounts.x;
    coords.y = probeIndex / (g_RaytracePerFrame.ProbeCounts.x * g_RaytracePerFrame.ProbeCounts.z);
    coords.z = (probeIndex / g_RaytracePerFrame.ProbeCounts.x) % g_RaytracePerFrame.ProbeCounts.z;
    
    return coords;
}

int GetProbeIndex(int3 probeCoords)
{
    //Probes per plane * current plane index + probe index in current plane
    return (g_RaytracePerFrame.ProbeCounts.x * g_RaytracePerFrame.ProbeCounts.z * probeCoords.z) + probeCoords.x + (g_RaytracePerFrame.ProbeCounts.x * probeCoords.z);

}

float3 GetProbeCoordsWorld(int3 probeCoords)
{
    return g_RaytracePerFrame.VolumePosition - ((g_RaytracePerFrame.ProbeCounts - 1) * g_RaytracePerFrame.ProbeSpacing * 0.5f) + (probeCoords * g_RaytracePerFrame.ProbeSpacing);
}

float3 GetFibonacciSpiralDirection(float index, float numSamples)
{
    const float PHI = sqrt(5) * 0.5f + 0.5f;
    float fraction = (index * (PHI - 1)) - floor(index * (PHI - 1));
    float phi = 2.0f * PI * fraction;
    float cosTheta = 1.0f - (2.0f * index + 1.0f) * (1.0f / numSamples);
    float sinTheta = sqrt(saturate(1.0f - cosTheta * cosTheta));

    return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

}

float3 GetProbeRayDirection(int rayIndex)
{
    return normalize(GetFibonacciSpiralDirection(rayIndex, g_RaytracePerFrame.RaysPerProbe));
}

Payload UnpackPayload(PackedPayload packedPayload)
{
    Payload payload;
    payload.HitDistance = packedPayload.HitDistance;
    payload.PosW = packedPayload.PosW;
    
    payload.Albedo.r = f16tof32(packedPayload.Packed0.x);
    payload.Albedo.g = f16tof32(packedPayload.Packed0.x >> 16); //Shift 16 bits so get second value
    payload.Albedo.b = f16tof32(packedPayload.Packed0.y);
    payload.NormalW.x = f16tof32(packedPayload.Packed0.y >> 16);
    payload.NormalW.y = f16tof32(packedPayload.Packed0.z);
    payload.NormalW.z = f16tof32(packedPayload.Packed0.z >> 16);
    payload.Metallic = f16tof32(packedPayload.Packed0.w);
    payload.Roughness = f16tof32(packedPayload.Packed0.w >> 16);

    payload.ShadingNormalW.x = f16tof32(packedPayload.Packed1.x);
    payload.ShadingNormalW.y = f16tof32(packedPayload.Packed1.x >> 16);
    payload.ShadingNormalW.z = f16tof32(packedPayload.Packed1.y);
    payload.Opacity = f16tof32(packedPayload.Packed1.y >> 16);
    payload.HitType = f16tof32(packedPayload.Packed1.z);

    return payload;
}

PackedPayload PackPayload(Payload payload)
{
    PackedPayload packedPayload = (PackedPayload) 0;
    
    packedPayload.HitDistance = payload.HitDistance;
    packedPayload.PosW = payload.PosW;
    
    packedPayload.Packed0.x = f32tof16(payload.Albedo.r);
    packedPayload.Packed0.x |= f32tof16(payload.Albedo.g) << 16;
    packedPayload.Packed0.y = f32tof16(payload.Albedo.b);
    packedPayload.Packed0.y |= f32tof16(payload.NormalW.x) << 16;
    packedPayload.Packed0.z = f32tof16(payload.NormalW.y);
    packedPayload.Packed0.z |= f32tof16(payload.NormalW.z) << 16;
    packedPayload.Packed0.w = f32tof16(payload.Metallic);
    packedPayload.Packed0.w |= f32tof16(payload.Roughness) << 16;
    
    packedPayload.Packed1.x = f32tof16(payload.ShadingNormalW.x);
    packedPayload.Packed1.x |= f32tof16(payload.ShadingNormalW.y) << 16;
    packedPayload.Packed1.y = f32tof16(payload.ShadingNormalW.z);
    packedPayload.Packed1.y |= f32tof16(payload.Opacity) << 16;
    packedPayload.Packed1.z = f32tof16(payload.HitType);
    
    return packedPayload;
}

void StoreRayMiss(uint2 texCoords)
{
    if (g_RaytracePerFrame.RayDataFormat == FORMAT_PROBE_RAY_DATA_R32G32B32A32_FLOAT)
    {
        RayData[texCoords] = float4(g_RaytracePerFrame.MissRadiance, 1e27f);
    }
    else
    {
        RayData[texCoords] = float4(asfloat(Float3ToUint(g_RaytracePerFrame.MissRadiance)), 1e27f, 0, 0);
    }
}

void StoreRayBackfaceHit(uint2 texCoords, float hitDistance)
{
    if (g_RaytracePerFrame.RayDataFormat == FORMAT_PROBE_RAY_DATA_R32G32B32A32_FLOAT)   //-ve means back face hit. Make magnitude shorter so has less influence
    {
        RayData[texCoords].w = hitDistance * -0.2f;
    }
    else
    {
        RayData[texCoords].g = hitDistance * -0.2f;
    }
}

void StoreRayFrontfaceHit(uint2 texCoords, float3 radiance, float hitDistance)
{
    if (g_RaytracePerFrame.RayDataFormat == FORMAT_PROBE_RAY_DATA_R32G32B32A32_FLOAT)
    {
        RayData[texCoords] = float4(radiance, hitDistance);
    }
    else
    {
        static const float c_threshold = 1.f / 255.f;
        if (max(radiance.x, max(radiance.y, radiance.z)) <= c_threshold)    //Check if max component will fit into accuracy available
        {
            radiance.rgb = float3(0.f, 0.f, 0.f);
        }

        RayData[texCoords] = float4(asfloat(Float3ToUint(radiance)), hitDistance, 0.f, 0.f);
    }
}

float3 GetSurfaceBias(float3 normal, float3 direction)
{
    return (normal * g_RaytracePerFrame.NormalBias) - (direction * g_RaytracePerFrame.ViewBias);
}

int3 GetClosestProbeCoords(float3 posW)
{
    float3 relativePos = posW - g_RaytracePerFrame.VolumePosition;
    relativePos += (g_RaytracePerFrame.ProbeSpacing * (g_RaytracePerFrame.ProbeCounts - 1)) * 0.5f;

    int3 probeCoords = int3(relativePos / g_RaytracePerFrame.ProbeSpacing);

    return clamp(probeCoords, int3(0, 0, 0), g_RaytracePerFrame.ProbeCounts - 1);

}

float GetBlendWeight(float3 posW)
{
    float3 deltaPosW = abs(posW - g_RaytracePerFrame.VolumePosition);

    if (all(deltaPosW < 0.0f))
    {
        return 1;
    }

    return (1.0f - saturate(deltaPosW.x / g_RaytracePerFrame.ProbeSpacing.x)) * (1.0f - saturate(deltaPosW.x / g_RaytracePerFrame.ProbeSpacing.y)) * (1.0f - saturate(deltaPosW.x / g_RaytracePerFrame.ProbeSpacing.z));
}

float3 GetIrradiance(float3 posW, float3 surfaceBias, float3 normalW)
{
    float3 biasedPosW = posW + surfaceBias;
    int3 closestProbeCoords = GetClosestProbeCoords(biasedPosW);
    float3 closestProbeCoordsWorld = GetProbeCoordsWorld(closestProbeCoords);

    float3 distanceRatio = clamp((biasedPosW - closestProbeCoordsWorld) / g_RaytracePerFrame.ProbeSpacing, float3(0, 0, 0), float3(1, 1, 1));
    
    for (int i = 0; i < 8; ++i)
    {
        //Some bitwise magic to get all the different offsets
        int3 probeOffset = int3(i, i >> 1, i >> 2) & int3(1, 1, 1);
        int3 probeCoords = clamp(closestProbeCoords + probeOffset, int3(0, 0, 0), g_RaytracePerFrame.ProbeCounts - 1);
        int probeIndex = GetProbeIndex(probeCoords);
    }

    return float3(0, 0, 0);

}