#ifndef RAYTRACING_COMMONS_HLSL
#define RAYTRACING_COMMONS_HLSL

#include "Commons.hlsli"

#define HIT_TYPE_TRIANGLE_BACK_FACE 0
#define HIT_TYPE_TRIANGLE_FRONT_FACE 1

#define FORMAT_PROBE_RAY_DATA_R32G32_FLOAT 0
#define FORMAT_PROBE_RAY_DATA_R32G32B32A32_FLOAT 1

#define FORMAT_PROBE_IRRADIANCE_R10G10B10A2_FLOAT 0
#define FORMAT_PROBE_IRRADIANCE_R32G32B32A32_FLOAT 1

ConstantBuffer<RaytracePerFrameCB> g_RaytracePerFrame : register(b1);

StructuredBuffer<Vertex> Vertices[] : register(t0, space100);

RaytracingAccelerationStructure Scene : register(t0, space200);

RWTexture2D<float4> RayData : register(u0);

struct PackedPayload
{
    float HitDistance;
    float3 PosW;
    uint4 Packed0;  //X = Albedo R && Albedo G, Y = Albedo B and Normal X, Z = Normal  and Normal Z, W = Metallic and Roughness
    uint4 Packed1;  //X = ShadingNormal X and ShadingNormal Y, Y = ShadingNormal Z and Opacity Z = Hit Type
};

struct Payload
{
    float3 Albedo;
    float Opacity;
    float3 PosW;
    float Metallic;
    float3 NormalW;
    float Roughness;
    float3 ShadingNormalW;
    float HitDistance;
    uint HitType;
};

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