#ifndef OCTAHEDRAL_HLSL
#define OCTAHEDRAL_HLSL

float3 GetOctahedralDirection(float2 coords)
{
    float3 direction = float3(coords.x, coords.y, 1.0f - abs(coords.x) - abs(coords.y));

    if(direction.z < 0.0f)
    {
        float2 tempDirection = direction.xy;
        direction.xy = (1.0f - abs(direction.yx));

        if (tempDirection.x < 0.0f)
        {
            direction.x *= -1.0f;
        }
        
        if (tempDirection.y < 0.0f)
        {
            direction.y *= -1.0f;
        }
    }
    
    return normalize(direction);
}

float2 GetNormalizedOctahedralCoords(int2 texCoords, int numTexels)
{
    float2 octahedralCoords = texCoords % numTexels;
    octahedralCoords += 0.5f;

    octahedralCoords /= float(numTexels);
    
    octahedralCoords *= 2.0f;
    octahedralCoords -= 1.0f;
    
    return octahedralCoords;
}

float2 GetOctahedralCoords(float3 direction)
{
    float2 uv = direction.xy / (abs(direction.x) + abs(direction.y) + abs(direction.z));
    
    if (direction.z < 0.0f)
    {        
        float2 tempUV = uv;
        uv = 1.0f - abs(uv.yx);
        
        if (tempUV.x < 0.0f)
        {
            uv.x *= -1.0f;
        }
        
        if (tempUV.y < 0.0f)
        {
            uv.y *= -1.0f;
        }
    }
    
    return uv;
}

#endif