#define HLSL

#include "ConstantBuffers.h"
#include "Vertices.h"
#include "DescriptorTables.hlsli"

struct RayPayload
{
    float4 color;
};

struct Ray
{
    float3 origin;
    float3 direction;
};

RaytracingAccelerationStructure Scene : register(t0, space200);

RWTexture2D<float4> RenderTarget : register(u0);

StructuredBuffer<Vertex> Vertices[] : register(t0, space100);
StructuredBuffer<PrimitivePerFrameCB> g_PrimitivePerFrameCB[] : register(t0, space101);
StructuredBuffer<PrimitiveInstanceCB> g_PrimitivePerInstanceCB[] : register(t0, space102);
StructuredBuffer<LightCB> g_LightCB[] : register(t0, space103);

ConstantBuffer<ScenePerFrameCB> g_ScenePerFrameCB : register(b0);

SamplerState SamPointWrap : register(s0);
SamplerState SamPointClamp : register(s1);
SamplerState SamLinearWrap : register(s2);
SamplerState SamLinearClamp : register(s3);
SamplerState SamAnisotropicWrap : register(s4);
SamplerState SamAnisotropicClamp : register(s5);

static const float PI = 3.14159265f;

float3 HitWoldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
    float2 xy = index + 0.5f;
    float2 PosS = xy / DispatchRaysDimensions().xy * 2.0f - 1.0f;

    PosS.y = -PosS.y;
    
    float4 PosW = mul(float4(PosS, 0, 1), g_ScenePerFrameCB.InvWorldProjection);
    PosW.xyz /= PosW.w;
    
    origin = g_ScenePerFrameCB.EyePosW.xyz;
    direction = normalize(PosW.xyz - origin);

}

float3 InterpolateAttribute(float3 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr)
{
    return vertexAttribute[0] +
        attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
        attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}