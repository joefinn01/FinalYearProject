#include "RaytracingCommons.hlsli"
#include "ProbeHelper.hlsl"

ConstantBuffer<PrimitiveIndexCB> l_PrimitiveIndexCB : register(b2);

Ray GenerateCameraRay(uint2 index, float3 eyePosW, float4x4 InvViewProjection)
{
    float2 xy = index + 0.5f;
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    // Invert y axis to correctly recreate world position
    screenPos.y = -screenPos.y;

    float4 PosW = mul(float4(screenPos, 0, 1), InvViewProjection);
    PosW.xyz /= PosW.w;

    Ray ray;
    ray.origin = eyePosW;
    ray.direction = normalize(PosW.xyz - ray.origin);

    return ray;
}

[shader("closesthit")]
#if ALBEDO
void ClosestHitAlbedo(inout PackedPayload packedPayload, in BuiltInTriangleIntersectionAttributes attr)
#else
void ClosestHit(inout PackedPayload packedPayload, in BuiltInTriangleIntersectionAttributes attr)
#endif
{
    Payload payload = (Payload) 0;
    payload.HitDistance = RayTCurrent();
    payload.HitType = HitKind();
    payload.PosW = HitWoldPosition();
    
    //Get copy of this primitives geometry information
    PrimitiveInstanceCB geomInfo = g_PrimitivePerInstanceCB[g_ScenePerFrameCB.PrimitivePerInstanceIndex][l_PrimitiveIndexCB.PrimitiveIndex];
    GameObjectPerFrameCB primInfo = g_PrimitivePerFrameCB[g_ScenePerFrameCB.PrimitivePerFrameIndex][l_PrimitiveIndexCB.InstanceIndex];
    
    //Cache indices
    uint primitiveIndex = PrimitiveIndex();
    
    uint3 indices;
    indices.x = BufferUintTable[geomInfo.IndicesIndex][primitiveIndex * 3];
    indices.y = BufferUintTable[geomInfo.IndicesIndex][primitiveIndex * 3 + 1];
    indices.z = BufferUintTable[geomInfo.IndicesIndex][primitiveIndex * 3 + 2];

    StructuredBuffer<Vertex> vertices = Vertices[geomInfo.VerticesIndex];
    
    //Get relevant mesh information at point on tri
    float3 normals[3] =
    {
        vertices[indices.x].Normal,
        vertices[indices.y].Normal,
        vertices[indices.z].Normal
    };
    
    float3 normalL = InterpolateAttribute(normals, attr);
    payload.NormalW = normalize(mul(float4(normalL, 0.0f), primInfo.InvTransposeWorld).xyz);
    payload.ShadingNormalW = payload.NormalW;
    
    //Calculate hit pos UV coords
    float3 bary = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    float2 uv = bary.x * vertices[indices.x].TexCoords + bary.y * vertices[indices.y].TexCoords + bary.z * vertices[indices.z].TexCoords;
    
#if ALBEDO
    float4 albedo = Tex2DTable[geomInfo.AlbedoIndex].SampleLevel(SamAnisotropicWrap, uv, 0);
#else
    float4 albedo = geomInfo.AlbedoColor;
#endif
    
    payload.Albedo = albedo.rgb;
    payload.Opacity = albedo.w;

    packedPayload = PackPayload(payload);
}