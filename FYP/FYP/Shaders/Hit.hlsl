#include "Commons.hlsli"

ConstantBuffer<PrimitiveIndexCB> l_PrimitiveIndexCB : register(b1);

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

float3 FresnelSchlick(float fNormDotHalf, float3 reflectance0)
{
    return reflectance0 + (1.0f - reflectance0) * pow(clamp(1.0f - fNormDotHalf, 0.0f, 1.0f), 5.0f);
}

float3 GetNormal(float2 uv, float3 normal, float3 tangent)
{
    float3 tangentW = normalize(tangent - dot(tangent, normal) * normal);
    tangentW = mul(float4(tangentW, 1.0f), g_PrimitivePerFrameCB[g_ScenePerFrameCB.PrimitivePerFrameIndex][l_PrimitiveIndexCB.Index].InvWorld).xyz;
    float3 normalW = mul(float4(normal, 0.0f), g_PrimitivePerFrameCB[g_ScenePerFrameCB.PrimitivePerFrameIndex][l_PrimitiveIndexCB.Index].InvWorld).xyz;
    
    float3x3 tbn = float3x3(tangentW, cross(normalW, tangentW), normalW);
    
    //Remap so between -1 and 1
    float3 normalT = Tex2DTable[g_PrimitivePerInstanceCB[g_ScenePerFrameCB.PrimitivePerInstanceIndex][l_PrimitiveIndexCB.Index].NormalIndex].SampleLevel(SamPointWrap, uv, 0).xyz;
    normalT *= 2.0f;
    normalT -= 1.0f;

    //Transform to model space
    return normalize(mul(normalT, tbn));

}

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

#if NORMAL_MAPPING && OCCLUSION_MAPPING && EMISSION_MAPPING
void ClosestHitNormalOcclusionEmission(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
#elif NORMAL_MAPPING && OCCLUSION_MAPPING
void ClosestHitNormalOcclusion(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
#elif NORMAL_MAPPING
void ClosestHitNormal(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
#elif OCCLUSION_MAPPING
void ClosestHitOcclusion(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
#elif NO_METALLIC_ROUGHNESS
void ClosestHitNoMetallicRoughness(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
#else
void ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
#endif
{
    //Get copy of this primitives geometry information
    PrimitiveInstanceCB geomInfo = g_PrimitivePerInstanceCB[g_ScenePerFrameCB.PrimitivePerInstanceIndex][l_PrimitiveIndexCB.Index];
    
    float3 hitPosW = HitWoldPosition();
    float3 viewW = normalize(g_ScenePerFrameCB.EyePosW.xyz - hitPosW);
    
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
        vertices[indices[0]].Normal,
        vertices[indices[1]].Normal,
        vertices[indices[2]].Normal
    };
    
    float3 normal = InterpolateAttribute(normals, attr);
    
    //Calculate hit pos UV coords
    float3 bary = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    float2 uv = bary.x * vertices[indices[0]].TexCoords + bary.y * vertices[indices[1]].TexCoords + bary.z * vertices[indices[2]].TexCoords;
    
    //Get primitive information from textures
    float3 albedo = pow(Tex2DTable[geomInfo.AlbedoIndex].SampleLevel(SamAnisotropicWrap, uv, 0).rgb, 2.2f).xyz;
    
#if !NO_METALLIC_ROUGHNESS
    
#if NORMAL_MAPPING
    float3 tangents[3] =
    {
        vertices[indices[0]].Tangent.xyz,
        vertices[indices[1]].Tangent.xyz,
        vertices[indices[2]].Tangent.xyz
    };
    
    float3 tangent = InterpolateAttribute(tangents, attr);
    
    float3 perPixelNormal = GetNormal(uv, normal, tangent);
#else
    float3 perPixelNormal = normal;
#endif
    
    float fMetallic = Tex2DTable[geomInfo.MetallicRoughnessIndex].SampleLevel(SamPointWrap, uv, 0).b;
    float fRoughness = Tex2DTable[geomInfo.MetallicRoughnessIndex].SampleLevel(SamAnisotropicWrap, uv, 0).g;
    
#if OCCLUSION_MAPPING
    float fOcclusion = Tex2DTable[geomInfo.MetallicRoughnessIndex].SampleLevel(SamPointWrap, uv, 0).r;
#endif
    
    float3 outgoingRadiance = float3(0.0f, 0.0f, 0.0f);
    
    float3 refelctance0 = float3(0.04f, 0.04f, 0.04f);
    refelctance0 = lerp(refelctance0, albedo, fMetallic);
    
    for (int i = 0; i < g_ScenePerFrameCB.NumLights; ++i)
    {
        float3 light = g_LightCB[g_ScenePerFrameCB.LightIndex][i].Position - hitPosW;
        
        //Cache distance so not done again when normalizing
        float fDistance = length(light);
        
        light /= fDistance;
        
        float3 halfVec = normalize(viewW + light);

        float fAttenuation = min(1.0f / dot(g_LightCB[g_ScenePerFrameCB.LightIndex][i].Attenuation[0], float3(1.0f, g_LightCB[g_ScenePerFrameCB.LightIndex][i].Attenuation[1] * fDistance, g_LightCB[g_ScenePerFrameCB.LightIndex][i].Attenuation[2] * fDistance * fDistance)), 1.0f);

        float3 radiance = g_LightCB[g_ScenePerFrameCB.LightIndex][i].Color.xyz * fAttenuation;

        float3 fresnel = FresnelSchlick(max(dot(halfVec, viewW), 0.0f), refelctance0);
        float fNDF = NormalDistribution(perPixelNormal, halfVec, fRoughness);
        float fGeomDist = GeometryDistribution(perPixelNormal, viewW, light, fRoughness);

        //Add 0.0001f to prevent divide by zero
        float3 specular = (fNDF * fGeomDist * fresnel) / (4.0f * max(dot(perPixelNormal, viewW), 0.0f) * max(dot(perPixelNormal, light), 0.0f) + 0.0001f);
        
        float3 fKD = (float3(1.0f, 1.0f, 1.0f) - fresnel) * (1.0f - fMetallic);
        
        float fNormDotLight = max(dot(perPixelNormal, light), 0.0f);
        outgoingRadiance += ((fKD * albedo / PI) + specular) * radiance * fNormDotLight;
    }
#endif
    
#if OCCLUSION_MAPPING
    payload.color = float4((float3(0.03f, 0.03f, 0.03f) * fOcclusion * albedo) + outgoingRadiance, 1.0f);
#elif NO_METALLIC_ROUGHNESS
    payload.color = float4(albedo, 1.0f);
#else
    payload.color = float4((float3(0.03f, 0.03f, 0.03f) * albedo) + outgoingRadiance, 1.0f);
#endif
    
    //Map and gamma correct
    float fMapping = 1.0f / 2.2f;
    
    payload.color.xyz /= (payload.color.xyz + float3(1.0f, 1.0f, 1.0f));
    payload.color.xyz = pow(payload.color.xyz, float3(fMapping, fMapping, fMapping));
}