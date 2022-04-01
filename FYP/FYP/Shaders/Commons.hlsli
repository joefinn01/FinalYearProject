#ifndef COMMONS_HLSL
#define COMMONS_HLSL

#ifndef HLSL
#define HLSL
#endif

#include "ConstantBuffers.h"
#include "Vertices.h"
#include "DescriptorTables.hlsli"
#include "Samplers.hlsli"

StructuredBuffer<GameObjectPerFrameCB> g_PrimitivePerFrameCB[] : register(t0, space101);
StructuredBuffer<PrimitiveInstanceCB> g_PrimitivePerInstanceCB[] : register(t0, space102);
StructuredBuffer<LightCB> g_LightCB[] : register(t0, space103);

ConstantBuffer<ScenePerFrameCB> g_ScenePerFrameCB : register(b0);

float3 GetNormal(float2 uv, float3 normal, float3 tangent, Texture2D tex, SamplerState samplerState)
{
    float3x3 tbn = float3x3(tangent, cross(normal, tangent), normal);
    
    //Remap so between -1 and 1
    float3 normalT = tex.SampleLevel(samplerState, uv, 0).xyz;
    normalT *= 2.0f;
    normalT -= 1.0f;

    //Transform to world space
    return normalize(mul(normalT, tbn));
}

#endif