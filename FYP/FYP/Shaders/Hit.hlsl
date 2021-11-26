#include "Commons.hlsli"

[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float3 hitPosW = HitWoldPosition();
    
    //Calculate geometry indices index
    uint indexByteSize = 2;
    uint indicesPerTri = 3;
    uint triIndexStride = indicesPerTri * indexByteSize;
    uint baseIndex = PrimitiveIndex() * triIndexStride;
    
    const uint3 indexes = Load3x16BitIndices(baseIndex);
    
    //Calculate hit position normal
    float3 normalsW[3] =
    {
        Vertices[indexes[0]].Normal,
        Vertices[indexes[1]].Normal,
        Vertices[indexes[2]].Normal

    };
    
    float3 normalW = InterpolateAttribute(normalsW, attr);
    
    float4 diffuse = CalculateDiffuseLighting(hitPosW, normalW);

    payload.color = g_ScenePerFrameCB.LightAmbientColor + diffuse;
}