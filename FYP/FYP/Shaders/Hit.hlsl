#include "Commons.hlsli"

Texture2D<float4> g_LocalAlbedo : register(t4);
Texture2D<float4> g_LocalNormal : register(t5);
Texture2D<float4> g_LocalMetallicRoughness : register(t6);
Texture2D<float4> g_LocalOcclusion : register(t7);
ConstantBuffer<PrimitiveInstanceCB> l_PrimitiveInstanceCB : register(b1);

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
    tangentW = mul(float4(tangentW, 1.0f), g_PrimitivePerFrameCB[l_PrimitiveInstanceCB.InstanceIndex].InvWorld).xyz;
    float3 normalW = mul(float4(normal, 0.0f), g_PrimitivePerFrameCB[l_PrimitiveInstanceCB.InstanceIndex].InvWorld).xyz;
    
    float3x3 tbn = float3x3(tangentW, cross(normalW, tangentW), normalW);
    
    //Remap so between -1 and 1
    float3 normalT = g_LocalNormal.SampleLevel(SamPointWrap, uv, 0).xyz;
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
void ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float3 hitPosW = HitWoldPosition();
    float3 viewW = normalize(g_ScenePerFrameCB.EyePosW.xyz - hitPosW);
    
    //Calculate geometry indices index
    uint indexByteSize = 2;
    uint indicesPerTri = 3;
    uint triIndexStride = indicesPerTri * indexByteSize;
    uint baseIndex = PrimitiveIndex() * triIndexStride;
    
    const uint3 indexes = Load3x16BitIndices(baseIndex);

    //Get relevant mesh information at point on tri
    float3 normals[3] =
    {
        Vertices[indexes[0]].Normal,
        Vertices[indexes[1]].Normal,
        Vertices[indexes[2]].Normal
    };
    
    float3 normal = InterpolateAttribute(normals, attr);
    
    float3 tangents[3] =
    {
        Vertices[indexes[0]].Tangent.xyz,
        Vertices[indexes[1]].Tangent.xyz,
        Vertices[indexes[2]].Tangent.xyz
    };
    
    float3 tangent = InterpolateAttribute(tangents, attr);
    
    //Calculate hit pos UV coords
    float3 bary = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    float2 uv = bary.x * Vertices[indexes[0]].TexCoords + bary.y * Vertices[indexes[1]].TexCoords + bary.z * Vertices[indexes[2]].TexCoords;

    //Get primitive information from textures
    float3 albedo = pow(g_LocalAlbedo.SampleLevel(SamAnisotropicWrap, uv, 0).rgb, 2.2f).xyz;
    float3 perPixelNormal = GetNormal(uv, normal, tangent);
    float fMetallic = g_LocalMetallicRoughness.SampleLevel(SamPointWrap, uv, 0).r;
    float fRoughness = g_LocalMetallicRoughness.SampleLevel(SamAnisotropicWrap, uv, 0).g;
    float fOcclusion = g_LocalOcclusion.SampleLevel(SamPointWrap, uv, 0).r;
    
    float3 outgoingRadiance = float3(0.0f, 0.0f, 0.0f);
    
    float3 refelctance0 = float3(0.04f, 0.04f, 0.04f);
    refelctance0 = lerp(refelctance0, albedo, fMetallic);
    
    int iNumLights = 1;
    
    for (int i = 0; i < iNumLights; ++i)
    {
        float3 light = g_ScenePerFrameCB.LightPosW.xyz - hitPosW;
        
        //Cache distance so not done again when normalizing
        float fdistance = length(light);
        
        light /= fdistance;
        
        float3 halfVec = normalize(viewW + light);

        float fAttenuation = 1.0f;

        float3 radiance = g_ScenePerFrameCB.LightColor.xyz * fAttenuation;

        float3 fresnel = FresnelSchlick(max(dot(halfVec, viewW), 0.0f), refelctance0);
        float fNDF = NormalDistribution(perPixelNormal, halfVec, fRoughness);
        float fGeomDist = GeometryDistribution(perPixelNormal, viewW, light, fRoughness);

        //Add 0.0001f to prevent divide by zero
        float3 specular = (fNDF * fGeomDist * fresnel) / (4.0f * max(dot(perPixelNormal, viewW), 0.0f) * max(dot(perPixelNormal, light), 0.0f) + 0.0001f);
        
        float3 fKD = (float3(1.0f, 1.0f, 1.0f) - fresnel) * (1.0f - fMetallic);
        
        float fNormDotLight = max(dot(perPixelNormal, light), 0.0f);
        outgoingRadiance += ((fKD * albedo / PI) + specular) * radiance * fNormDotLight;
    }

    payload.color = float4((float3(0.03f, 0.03f, 0.03f) * fOcclusion * albedo) + outgoingRadiance, 1.0f);
    
    //Map and gamma correct
    float fMapping = 1.0f / 2.2f;
    
    payload.color.xyz /= (payload.color.xyz + float3(1.0f, 1.0f, 1.0f));
    payload.color.xyz = pow(payload.color.xyz, float3(fMapping, fMapping, fMapping));
}