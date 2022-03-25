#ifndef RAYTRACING_COMMONS_HLSL
#define RAYTRACING_COMMONS_HLSL

#include "Commons.hlsli"

ConstantBuffer<RaytracePerFrameCB> g_RaytracePerFrame : register(b1);

StructuredBuffer<Vertex> Vertices[] : register(t0, space100);

RaytracingAccelerationStructure Scene : register(t0, space200);

RWTexture2D<float4> RayData : register(u0);

struct Ray
{
    float3 origin;
    float3 direction;
};

float3 HitWoldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
    float2 xy = index + 0.5f;
    float2 PosS = xy / DispatchRaysDimensions().xy * 2.0f - 1.0f;

    PosS.y = -PosS.y;
    
    float4 PosW = mul(float4(PosS, 0, 1), g_ScenePerFrameCB.InvViewProjection);
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

#endif