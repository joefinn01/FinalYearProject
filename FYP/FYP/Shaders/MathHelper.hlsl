#ifndef MATH_HELPER_HLSL
#define MATH_HELPER_HLSL

#include "Defines.hlsli"

float3 GetFibonacciSpiralDirection(float index, float numSamples)
{
    const float PHI = sqrt(5.0f) * 0.5f + 0.5f;
    float phi = 2.0f * PI * frac(index * (PHI - 1));
    float cosTheta = 1.0f - ((2.0f * index + 1.0f) / numSamples);
    float sinTheta = sqrt(saturate(1.0f - cosTheta * cosTheta));

    return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

uint FloatToUint(float val, float scale)
{
    return uint(floor(val * scale + 0.5f));
}

uint Float3ToUint(float3 input)
{
    return FloatToUint(input.r, 1023.0f) | (FloatToUint(input.g, 1023.0f) << 10) | (FloatToUint(input.b, 1023.0f) << 20);
}

float3 UintToFloat3(uint input)
{
    float3 output;
    output.x = (float) (input & 0x000003FF) / 1023.0f;
    output.y = (float) ((input >> 10) & 0x000003FF) / 1023.0f;
    output.z = (float) ((input >> 20) & 0x000003FF) / 1023.0f;
    return output;
}

float3 QuaternionRotate(float3 vec, float4 quat)
{
    return vec * (quat.w * quat.w - dot(quat.xyz, quat.xyz)) + quat.xyz * 2.0f * dot(vec, quat.xyz) + cross(quat.xyz, vec) * quat.w * 2.0f;
}

float4 QuaternionConjugate(float4 quat)
{
    return float4(-quat.xyz, quat.w);
}

#endif