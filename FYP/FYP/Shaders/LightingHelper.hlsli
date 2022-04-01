#ifndef LIGHTING_HELPER_HLSL
#define LIGHTING_HELPER_HLSL

#include "Defines.hlsli"

float NormalDistribution(float3 normal, float3 halfVec, float fRoughness)
{
    float fRoughSq = fRoughness * fRoughness * fRoughness * fRoughness;
    
    float fNormDotHalf = max(dot(normal, halfVec), 0.0f);

    float fDenominator = (fNormDotHalf * fNormDotHalf * (fRoughSq - 1.0f) + 1.0f);
    fDenominator = PI * fDenominator * fDenominator;
    
    return fRoughSq / fDenominator;
}

float SchlickGGX(float fNormDotView, float fRoughness)
{
    float fR = fRoughness + 1;
    float fK = (fR * fR) / 8.0f;

    return fNormDotView / (fNormDotView * (1.0f - fK) + fK);
}

float GeometryDistribution(float3 normal, float3 view, float3 light, float fRoughness)
{
    float fNormDotView = max(dot(normal, view), 0.0f);
    float fNormDotLight = max(dot(normal, light), 0.0f);

    return SchlickGGX(fNormDotView, fRoughness) * SchlickGGX(fNormDotLight, fRoughness);

}

float3 FresnelSchlick(float fHalfDotView, float3 reflectance0)
{
    return reflectance0 + (1.0f - reflectance0) * pow(clamp(1.0f - fHalfDotView, 0.0f, 1.0f), 5.0f);
}
#endif