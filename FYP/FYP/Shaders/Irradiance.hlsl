#ifndef IRRADIANCE_HLSL
#define IRRADIANCE_HLSL

#ifndef HLSL
#define HLSL
#endif

#include "ConstantBuffers.h"
#include "DescriptorTables.hlsli"
#include "ProbeHelper.hlsl"
#include "Samplers.hlsli"

//RGB is radiance and W is distance
float4 GetIrradiance(float3 posW, float3 surfaceBias, float3 direction, RaytracePerFrameCB raytracingPerFrameCB)
{
    Texture2D<float4> irradianceAtlas = Tex2DTable[raytracingPerFrameCB.IrradianceIndex];
    Texture2D<float4> distanceAtlas = Tex2DTable[raytracingPerFrameCB.DistanceIndex];
    
    float3 biasedPosW = posW + surfaceBias;
    int3 closestProbeCoords = GetClosestProbeCoords(biasedPosW, raytracingPerFrameCB.VolumePosition, raytracingPerFrameCB.ProbeOffsets, raytracingPerFrameCB.ProbeSpacing, raytracingPerFrameCB.ProbeCounts);
    float3 closestProbeCoordsWorld = GetProbeCoordsWorld(closestProbeCoords, raytracingPerFrameCB.VolumePosition, raytracingPerFrameCB.ProbeOffsets, raytracingPerFrameCB.ProbeSpacing, raytracingPerFrameCB.ProbeCounts);

    float3 distanceRatio = clamp((biasedPosW - closestProbeCoordsWorld) / raytracingPerFrameCB.ProbeSpacing, float3(0, 0, 0), float3(1, 1, 1));
    
    float4 irradiance = float4(0, 0, 0, 0);
    float accumulatedWeights = 0;
    float accumulatedDistanceWeights = 0;
    
    for (int i = 0; i < 8; ++i)
    {
        //Some bitwise magic to get all the different offsets
        int3 probeOffset = int3(i, i >> 1, i >> 2) & int3(1, 1, 1);
        int3 probeCoords = clamp(closestProbeCoords + probeOffset, int3(0, 0, 0), raytracingPerFrameCB.ProbeCounts - 1);
        int probeIndex = GetOffsettedProbeIndex(probeCoords, raytracingPerFrameCB.ProbeCounts, raytracingPerFrameCB.ProbeOffsets);
        
        float3 probeCoordsWorld = GetProbeCoordsWorld(probeCoords, raytracingPerFrameCB.VolumePosition, raytracingPerFrameCB.ProbeOffsets, raytracingPerFrameCB.ProbeSpacing, raytracingPerFrameCB.ProbeCounts);
        
        float3 toProbe = normalize(probeCoordsWorld - posW);
        float3 biasedToProbe = probeCoordsWorld - biasedPosW;
        
        float biasedToProbeDistance = length(biasedToProbe);

        biasedToProbe /= biasedToProbeDistance;

        float3 trilinear = max(0.001f, lerp(1.0f - distanceRatio, distanceRatio, probeOffset));
        
        float wrapShading = (1.0f + dot(toProbe, direction)) * 0.5f;
        
        float weight = wrapShading * wrapShading + 0.2f;

        float2 octCoords = GetOctahedralCoords(-biasedToProbe);
        float2 atlasCoords = GetAtlasCoords(probeIndex, octCoords, raytracingPerFrameCB.NumDistanceTexels, raytracingPerFrameCB.ProbeCounts);

        //R component is distance, G component is distance squared
        float2 distance = 2.0f * distanceAtlas.SampleLevel(SamLinearWrap, atlasCoords, 0).rg;
        
        float variance = abs(distance.x * distance.x - distance.y);

        float chebyshevWeight = 1.0f;
        if (biasedToProbeDistance > distance.x)
        {
            float v = biasedToProbeDistance - distance.x;
            chebyshevWeight = variance / (v * v + variance);

            chebyshevWeight = max(chebyshevWeight * chebyshevWeight * chebyshevWeight, 0.0f);
        }
        
        weight *= max(0.05f, chebyshevWeight);
        weight = max(0.000001f, weight);
        
        const float crushThreshold = 0.2f;
        if (weight < crushThreshold)
        {
            weight *= (weight * weight) / (crushThreshold * crushThreshold);
        }

        float trilinearWeight = trilinear.x * trilinear.y * trilinear.z;
        weight *= trilinearWeight;

        octCoords = GetOctahedralCoords(direction);
        atlasCoords = GetAtlasCoords(probeIndex, octCoords, raytracingPerFrameCB.NumIrradianceTexels, raytracingPerFrameCB.ProbeCounts);

        float3 probeIrradiance = irradianceAtlas.SampleLevel(SamLinearWrap, atlasCoords, 0).rgb;
        probeIrradiance = pow(probeIrradiance, raytracingPerFrameCB.IrradianceGammaEncoding * 0.5f);

        irradiance.rgb += (weight * probeIrradiance);
        irradiance.w += (trilinearWeight * distance.x);
        
        accumulatedWeights += weight;
        accumulatedDistanceWeights += trilinearWeight;

    }
    
    if (accumulatedWeights == 0.0f)
    {
        return float4(0, 0, 0, -1);
    }

    irradiance.rgb /= accumulatedWeights;
    irradiance.rgb *= irradiance.rgb;
    irradiance.rgb *= PI * 2.0f;

    irradiance.w *= 1.0f / accumulatedDistanceWeights;
    
    if (raytracingPerFrameCB.IrradianceFormat == FORMAT_PROBE_IRRADIANCE_R10G10B10A2_FLOAT)
    {
        irradiance.rgb *= 1.0989f;
    }

    return irradiance;
}

#endif