#ifndef PAYLOADS_HLSL
#define PAYLOADS_HLSL

struct PackedPayload
{
    float HitDistance;
    float3 PosW;
    uint4 Packed0; //X = Albedo R && Albedo G, Y = Albedo B and Normal X, Z = Normal  and Normal Z, W = Metallic and Roughness
    uint4 Packed1; //X = ShadingNormal X and ShadingNormal Y, Y = ShadingNormal Z and Opacity Z = Hit Type
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

Payload UnpackPayload(PackedPayload packedPayload)
{
    Payload payload = (Payload) 0;
    payload.HitDistance = packedPayload.HitDistance;
    payload.PosW = packedPayload.PosW;
    
    payload.Albedo.r = f16tof32(packedPayload.Packed0.x);
    payload.Albedo.g = f16tof32(packedPayload.Packed0.x >> 16);
    payload.Albedo.b = f16tof32(packedPayload.Packed0.y);
    payload.NormalW.x = f16tof32(packedPayload.Packed0.y >> 16);
    payload.NormalW.y = f16tof32(packedPayload.Packed0.z);
    payload.NormalW.z = f16tof32(packedPayload.Packed0.z >> 16);
    payload.Metallic = f16tof32(packedPayload.Packed0.w);
    payload.Roughness = f16tof32(packedPayload.Packed0.w >> 16);

    payload.ShadingNormalW.x = f16tof32(packedPayload.Packed1.x);
    payload.ShadingNormalW.y = f16tof32(packedPayload.Packed1.x >> 16);
    payload.ShadingNormalW.z = f16tof32(packedPayload.Packed1.y);
    payload.Opacity = f16tof32(packedPayload.Packed1.y >> 16);
    payload.HitType = f16tof32(packedPayload.Packed1.z);

    return payload;
}

PackedPayload PackPayload(Payload payload)
{
    PackedPayload packedPayload = (PackedPayload) 0;
    
    packedPayload.HitDistance = payload.HitDistance;
    packedPayload.PosW = payload.PosW;
    
    packedPayload.Packed0.x = f32tof16(payload.Albedo.r);
    packedPayload.Packed0.x |= f32tof16(payload.Albedo.g) << 16;
    packedPayload.Packed0.y = f32tof16(payload.Albedo.b);
    packedPayload.Packed0.y |= f32tof16(payload.NormalW.x) << 16;
    packedPayload.Packed0.z = f32tof16(payload.NormalW.y);
    packedPayload.Packed0.z |= f32tof16(payload.NormalW.z) << 16;
    packedPayload.Packed0.w = f32tof16(payload.Metallic);
    packedPayload.Packed0.w |= f32tof16(payload.Roughness) << 16;
    
    packedPayload.Packed1.x = f32tof16(payload.ShadingNormalW.x);
    packedPayload.Packed1.x |= f32tof16(payload.ShadingNormalW.y) << 16;
    packedPayload.Packed1.y = f32tof16(payload.ShadingNormalW.z);
    packedPayload.Packed1.y |= f32tof16(payload.Opacity) << 16;
    packedPayload.Packed1.z = f32tof16(payload.HitType);
    
    return packedPayload;
}

#endif