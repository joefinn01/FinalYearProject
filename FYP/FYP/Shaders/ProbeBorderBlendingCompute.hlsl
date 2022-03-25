#include "ProbeHelper.hlsl"

#if BLEND_RADIANCE
RWTexture2D<float4> IrradianceData : register(u1);
#else
RWTexture2D<float4> DistanceData : register(u1);
#endif

[numthreads(8, 8, 1)]
void RowBlend( uint3 DTid : SV_DispatchThreadID )
{
    //+2 for one row of padding on each side
    uint rowLength = NUM_TEXELS_PER_PROBE + 2;
    
    uint2 threadCoords = DTid.xy;
    threadCoords.y *= rowLength;
    
    //Check if corner texel and return as do corners in column update
    int mod = (DTid.x % rowLength);
    if (mod == 0 || mod == rowLength - 1)
    {
        return;
    }
    
    uint probeStart = uint(threadCoords.x / rowLength) * rowLength;
    uint offset = rowLength - (threadCoords.x % rowLength) - 1;
    
    uint2 copyCoords = uint2(probeStart + offset, threadCoords.y + 1);

#if BLEND_RADIANCE
    IrradianceData[threadCoords] = IrradianceData[copyCoords];
    
    threadCoords.y += rowLength - 1;

    copyCoords = uint2(probeStart + offset, threadCoords.y - 1);
    
    IrradianceData[threadCoords] = IrradianceData[copyCoords];
#else
    DistanceData[threadCoords] = DistanceData[copyCoords];
    
    threadCoords.y += rowLength - 1;

    copyCoords = uint2(probeStart + offset, threadCoords.y - 1);
    
    DistanceData[threadCoords] = DistanceData[copyCoords];
#endif
}   

[numthreads(8, 8, 1)]
void ColumnBlend(uint3 DTid : SV_DispatchThreadID)
{
    //+2 for one column of padding on each side
    uint columnLength = NUM_TEXELS_PER_PROBE + 2;
    
    uint2 threadCoords = DTid.xy;
    threadCoords.x *= columnLength;
    
    uint2 copyCoords = uint2(0, 0);
    
    //Check if corner texel
    int mod = (threadCoords.y % columnLength);
    if (mod == 0 || mod == columnLength - 1)
    {
        copyCoords.x = threadCoords.x + NUM_TEXELS_PER_PROBE;
        copyCoords.y = threadCoords.y - sign(mod - 1) * NUM_TEXELS_PER_PROBE;
        
#if BLEND_RADIANCE
        IrradianceData[threadCoords] = IrradianceData[copyCoords];
        
        threadCoords.x += columnLength - 1;
        copyCoords.x = threadCoords.x - NUM_TEXELS_PER_PROBE;

        IrradianceData[threadCoords] = IrradianceData[copyCoords];
#else
        DistanceData[threadCoords] = DistanceData[copyCoords];
        
        threadCoords.x += columnLength - 1;
        copyCoords.x = threadCoords.x - NUM_TEXELS_PER_PROBE;

        DistanceData[threadCoords] = DistanceData[copyCoords];
#endif
        
        return;
    }
    
    uint probeStart = uint(threadCoords.y / columnLength) * columnLength;
    uint offset = columnLength - (threadCoords.y % columnLength) - 1;

    copyCoords = uint2(threadCoords.x + 1, probeStart + offset);

#if BLEND_RADIANCE
    IrradianceData[threadCoords] = IrradianceData[copyCoords];
    
    threadCoords.x += columnLength - 1;
    copyCoords = uint2(threadCoords.x - 1, probeStart + offset);

    IrradianceData[threadCoords] = IrradianceData[copyCoords];
#else
    DistanceData[threadCoords] = DistanceData[copyCoords];
    
    threadCoords.x += columnLength - 1;
    copyCoords = uint2(threadCoords.x - 1, probeStart + offset);

    DistanceData[threadCoords] = DistanceData[copyCoords];
#endif
}