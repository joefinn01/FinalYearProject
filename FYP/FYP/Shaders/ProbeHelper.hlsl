#ifndef PROBE_HELPER_HLSL
#define PROBE_HELPER_HLSL

#include "MathHelper.hlsl"
#include "Octahedral.hlsl"
#include "Payloads.hlsli"
#include "Defines.hlsli"

int3 GetProbeCoords(int probeIndex, int3 probeCounts)
{    
    int3 coords;
    
    coords.x = probeIndex % probeCounts.x;
    coords.y = probeIndex / (probeCounts.x * probeCounts.z);
    coords.z = (probeIndex / probeCounts.x) % probeCounts.z;
    
    return coords;
}

int GetProbeIndex(int3 probeCoords, int3 probeCounts)
{
    //Probes per plane * current plane index + probe index in current plane
    return (probeCounts.x * probeCounts.z * probeCoords.y) + probeCoords.x + (probeCounts.x * probeCoords.z);

}

int GetProbeIndex(int2 texCoords, int numTexels, int3 probeCounts)
{
    int planeIndex = int(texCoords.x / (numTexels * probeCounts.x));
    int probeIndex = int(texCoords.x / numTexels) - (planeIndex * probeCounts.x) + (probeCounts.x * int(texCoords.y / numTexels));
    
    //Probes per plane * current plane index + probe index in current plane
    return (probeCounts.x * probeCounts.z * planeIndex) + probeIndex;

}

int GetOffsettedProbeIndex(int3 probeCoords, int3 probeCounts, int3 probeOffsets)
{
    return GetProbeIndex((probeCoords + probeOffsets + probeCounts) % probeCounts, probeCounts);
}

float3 GetProbeCoordsWorld(int3 probeCoords, float3 volumePosition, int3 probeOffsets, float3 probeSpacing, int3 probeCounts)
{
    return volumePosition + (probeOffsets * probeSpacing) - ((probeCounts - 1) * probeSpacing * 0.5f) + (probeCoords * probeSpacing);
}

float3 GetProbeRayDirection(int rayIndex, int raysPerProbe, float4 rayRotation)
{
    return normalize(QuaternionRotate(GetFibonacciSpiralDirection(rayIndex, raysPerProbe), QuaternionConjugate(rayRotation)));
}

void StoreRayMiss(uint2 texCoords, int rayDataFormat, float3 missRadiance, RWTexture2D<float4> rayData)
{
    if (rayDataFormat == FORMAT_PROBE_RAY_DATA_R32G32B32A32_FLOAT)
    {
#if DEBUG_HIT_TYPES
        rayData[texCoords] = float4(float3(1, 0, 0), 1e27f);
#else
        rayData[texCoords] = float4(missRadiance, 1e27f);
#endif
    }
    else
    {
        rayData[texCoords] = float4(Float3ToUint(missRadiance), 1e27f, 0, 0);
    }
}

void StoreRayBackfaceHit(uint2 texCoords, float hitDistance, int rayDataFormat, RWTexture2D<float4> rayData)
{
    if (rayDataFormat == FORMAT_PROBE_RAY_DATA_R32G32B32A32_FLOAT)
    {
#if DEBUG_HIT_TYPES
        rayData[texCoords] = float4(float3(0, 1, 0), 1e27f);
#else
        rayData[texCoords].w = -hitDistance * 0.2f;
#endif
    }
    else
    {
        rayData[texCoords].g = -hitDistance * 0.2f;
    }
}

void StoreRayFrontfaceHit(uint2 texCoords, float3 radiance, float hitDistance, int rayDataFormat, RWTexture2D<float4> rayData)
{
    if (rayDataFormat == FORMAT_PROBE_RAY_DATA_R32G32B32A32_FLOAT)
    {
#if DEBUG_HIT_TYPES
        rayData[texCoords] = float4(float3(0, 0, 1), 1e27f);
#else
        rayData[texCoords] = float4(radiance, hitDistance);
#endif
    }
    else
    {
        static const float c_threshold = 1.0f / 255.0f;
        if (max(radiance.x, max(radiance.y, radiance.z)) <= c_threshold)    //Check if max component will fit into accuracy available
        {
            radiance.rgb = float3(0.f, 0.f, 0.f);
        }

        rayData[texCoords] = float4(Float3ToUint(radiance), hitDistance, 0.f, 0.f);
    }
}

float3 GetSurfaceBias(float3 normal, float3 direction, float normalBias, float viewBias)
{
    return (normal * normalBias) + (-direction * viewBias);
}

int3 GetClosestProbeCoords(float3 posW, float3 volumePosition, int3 probeOffsets, float3 probeSpacing, int3 probeCounts)
{
    float3 relativePos = posW - (volumePosition + (probeOffsets * probeSpacing) - (probeSpacing * (probeCounts - 1)) * 0.5f);

    int3 probeCoords = int3(relativePos / probeSpacing);

    return clamp(probeCoords, int3(0, 0, 0), probeCounts - 1);
}

float GetBlendWeight(float3 posW, float3 volumePosition, int3 probeOffsets, float3 probeSpacing, int3 probeCounts)
{
    float3 deltaPosW = abs(posW - (volumePosition + (probeOffsets * probeSpacing))) - (probeSpacing * (probeCounts - 1)) * 0.5f;

    if (all(deltaPosW < 0.0f))
    {
        return 1;
    }

    float weight = 1.0f;
    weight *= (1.0f - saturate(deltaPosW.x / probeSpacing.x));
    weight *= (1.0f - saturate(deltaPosW.y / probeSpacing.y));
    weight *= (1.0f - saturate(deltaPosW.z / probeSpacing.z));
    
    return weight;
}

float2 GetAtlasCoords(int probeIndex, float2 octCoords, int numTexels, int3 probeCounts)
{
    int gridSpaceX = (probeIndex % probeCounts.x);
    int gridSpaceY = (probeIndex / probeCounts.x);

    int x = gridSpaceX + int(probeIndex / (probeCounts.x * probeCounts.z)) * probeCounts.x;
    int y = gridSpaceY % probeCounts.z;
    
    float2 uv = float2(x * (numTexels + 2), y * (numTexels + 2)) + ((numTexels + 2) * 0.5f);
    uv += octCoords.xy * (numTexels * 0.5f);
    uv /= float2((numTexels + 2) * (probeCounts.x * probeCounts.y), (numTexels + 2) * probeCounts.z);
    
    return uv;
}

float3 GetRayDirection(int rayIndex, int raysPerProbe, float4 rotationQuat)
{
    float3 direction = GetFibonacciSpiralDirection(rayIndex, raysPerProbe);
    
    return normalize(QuaternionRotate(direction, QuaternionConjugate(rotationQuat)));
}

float3 GetRayRadiance(uint2 texCoords, int rayDataFormat, Texture2D<float4> rayData)
{
    if (rayDataFormat == FORMAT_PROBE_RAY_DATA_R32G32B32A32_FLOAT)
    {
        return rayData[texCoords].rgb;
    }
    else
    {
        return UintToFloat3(rayData[texCoords].r);
    }
}

float GetRayDistance(uint2 texCoords, int rayDataFormat, Texture2D<float4> rayData)
{
    if (rayDataFormat == FORMAT_PROBE_RAY_DATA_R32G32B32A32_FLOAT)
    {
        return rayData[texCoords].a;
    }
    else
    {
        return rayData[texCoords].g;
    }
}

bool ClearScrolledPlane(int2 coords, int3 probeCoords, int planeIndex, int3 probeOffsets, int3 probeCounts, int3 clearPlane, RWTexture2D<float4> dataAtlas)
{
    if(clearPlane[planeIndex] == true)
    {
        int plane;
        
        if(probeOffsets[planeIndex] > 0)
        {
            plane = (probeOffsets[planeIndex] % probeCounts[planeIndex]) - 1;

        }
        else
        {
            plane = (probeOffsets[planeIndex] % probeCounts[planeIndex]) + probeCounts[planeIndex];
        }
        
        if (probeCoords[planeIndex] == plane)
        {
            dataAtlas[coords] = float4(0.f, 0.f, 0.f, 0.f);
            
            return true;
        }
    }
    
    return false;
}

#endif