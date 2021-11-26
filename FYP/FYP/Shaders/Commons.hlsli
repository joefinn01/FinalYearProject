#define HLSL

#include "ConstantBuffers.h"
#include "Vertices.h"

struct RayPayload
{
    float4 color;
};

RaytracingAccelerationStructure Scene : register(t0, space0);

RWTexture2D<float4> RenderTarget : register(u0);

ByteAddressBuffer Indices : register(t1, space0);
StructuredBuffer<Vertex> Vertices : register(t2, space0);

ConstantBuffer<ScenePerFrameCB> g_ScenePerFrameCB : register(b0);
ConstantBuffer<CubeCB> g_CubeCB : register(b1);

// Load three 16 bit indices from a byte addressed buffer.
uint3 Load3x16BitIndices(uint offsetBytes)
{
    uint3 indices;

    // ByteAdressBuffer loads must be aligned at a 4 byte boundary.
    // Since we need to read three 16 bit indices: { 0, 1, 2 } 
    // aligned at a 4 byte boundary as: { 0 1 } { 2 0 } { 1 2 } { 0 1 } ...
    // we will load 8 bytes (~ 4 indices { a b | c d }) to handle two possible index triplet layouts,
    // based on first index's offsetBytes being aligned at the 4 byte boundary or not:
    //  Aligned:     { 0 1 | 2 - }
    //  Not aligned: { - 0 | 1 2 }
    const uint dwordAlignedOffset = offsetBytes & ~3;
    const uint2 four16BitIndices = Indices.Load2(dwordAlignedOffset);
 
    // Aligned: { 0 1 | 2 - } => retrieve first three 16bit indices
    if (dwordAlignedOffset == offsetBytes)
    {
        indices.x = four16BitIndices.x & 0xffff;
        indices.y = (four16BitIndices.x >> 16) & 0xffff;
        indices.z = four16BitIndices.y & 0xffff;
    }
    else // Not aligned: { - 0 | 1 2 } => retrieve last three 16bit indices
    {
        indices.x = (four16BitIndices.x >> 16) & 0xffff;
        indices.y = four16BitIndices.y & 0xffff;
        indices.z = (four16BitIndices.y >> 16) & 0xffff;
    }

    return indices;
}

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
    
    origin = g_ScenePerFrameCB.EyePosW;
    direction = normalize(PosW.xyz - origin);

}

float3 InterpolateAttribute(float3 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr)
{
    return vertexAttribute[0] +
        attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
        attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

float4 CalculateDiffuseLighting(float3 posW, float3 normalW)
{
    float3 toLight = normalize(g_ScenePerFrameCB.LightPosW - posW);
    
    return g_CubeCB.Albedo * g_ScenePerFrameCB.LightDiffuseColor * max(0.0f, dot(toLight, normalW));
}