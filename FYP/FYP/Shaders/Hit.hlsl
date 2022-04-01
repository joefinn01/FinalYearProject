#include "RaytracingCommons.hlsli"
#include "ProbeHelper.hlsl"

ConstantBuffer<PrimitiveIndexCB> l_PrimitiveIndexCB : register(b2);

[shader("closesthit")]

#if NORMAL_MAPPING && OCCLUSION_MAPPING && EMISSION_MAPPING && ALBEDO && METALLIC_ROUGHNESS
void ClosestHitNormalOcclusionEmissionAlbedoMetallicRoughness(inout PackedPayload packedPayload, in BuiltInTriangleIntersectionAttributes attr)
#elif NORMAL_MAPPING && OCCLUSION_MAPPING && ALBEDO && METALLIC_ROUGHNESS
void ClosestHitNormalOcclusionAlbedoMetallicRoughness(inout PackedPayload packedPayload, in BuiltInTriangleIntersectionAttributes attr)
#elif NORMAL_MAPPING && ALBEDO && METALLIC_ROUGHNESS
void ClosestHitNormalAlbedoMetallicRoughness(inout PackedPayload packedPayload, in BuiltInTriangleIntersectionAttributes attr)
#elif OCCLUSION_MAPPING && ALBEDO && METALLIC_ROUGHNESS
void ClosestHitOcclusionAlbedoMetallicRoughness(inout PackedPayload packedPayload, in BuiltInTriangleIntersectionAttributes attr)
#elif ALBEDO && METALLIC_ROUGHNESS
void ClosestHitAlbedoMetallicRoughness(inout PackedPayload packedPayload, in BuiltInTriangleIntersectionAttributes attr)
#elif ALBEDO
void ClosestHitAlbedo(inout PackedPayload packedPayload, in BuiltInTriangleIntersectionAttributes attr)
#else
void ClosestHit(inout PackedPayload packedPayload, in BuiltInTriangleIntersectionAttributes attr)
#endif
{
    Payload payload = (Payload) 0;
    payload.HitDistance = RayTCurrent();
    payload.HitType = HitKind();
    payload.PosW = HitWorldPosition();
    
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
    
    //Calculate hit pos UV coords
    float3 bary = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    float2 uv = bary.x * vertices[indices.x].TexCoords + bary.y * vertices[indices.y].TexCoords + bary.z * vertices[indices.z].TexCoords;
    
#if NORMAL_MAPPING
    float3 tangents[3] =
    {
        vertices[indices.x].Tangent.xyz,
        vertices[indices.y].Tangent.xyz,
        vertices[indices.z].Tangent.xyz
    };
    
    float3 tangent = InterpolateAttribute(tangents, attr);
   
    tangent = normalize(mul(float4(tangent, 0.0f), primInfo.InvTransposeWorld).xyz);
    
    payload.ShadingNormalW = GetNormal(uv, payload.NormalW, tangent, Tex2DTable[geomInfo.NormalIndex], SamPointWrap);
#else
    payload.ShadingNormalW = payload.NormalW;
#endif
    
#if METALLIC_ROUGHNESS
    float4 metallicRoughnessOcclusion = Tex2DTable[geomInfo.MetallicRoughnessIndex].SampleLevel(SamPointWrap, uv, 0);
    
#if OCCLUSION_MAPPING
    payload.Occlusion = metallicRoughnessOcclusion.r;
#else
    payload.Occlusion = 1.0f;
#endif
    
    payload.Roughness = metallicRoughnessOcclusion.g;
    payload.Metallic = metallicRoughnessOcclusion.b;
#else
    payload.Roughness = 1;
    payload.Metallic = 0;
    payload.Occlusion = 1;
#endif
    
#if ALBEDO
    float4 albedo = pow(Tex2DTable[geomInfo.AlbedoIndex].SampleLevel(SamAnisotropicWrap, uv, 0), 2.2f);
#else
    float4 albedo = geomInfo.AlbedoColor;
#endif
    
    payload.Albedo = albedo.rgb;
    payload.Opacity = albedo.w;

    packedPayload = PackPayload(payload);
}