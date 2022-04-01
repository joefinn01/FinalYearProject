#include "RaytracingCommons.hlsli"
#include "Payloads.hlsli"
#include "RaytracedLightingHelper.hlsl"
#include "LightingHelper.hlsli"

RWTexture2D<float4> NormalW : register(u0);
RWTexture2D<float4> Albedo : register(u1);
RWTexture2D<float4> DirectLight : register(u2);
RWTexture2D<float4> Position : register(u3);

[shader("raygeneration")]
void GBufferRayGen()
{
    float3 rayDirectionW;
    float3 originW;
    
    GenerateCameraRay(DispatchRaysIndex().xy, originW, rayDirectionW);

    RayDesc ray;
    ray.Origin = originW;
    ray.Direction = rayDirectionW;
    ray.TMin = 0;
    ray.TMax = 1e27f;
    
    PackedPayload packedPayload = (PackedPayload) 0;
    TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 1, 0, ray, packedPayload);

    //Missed so exit
    if(packedPayload.HitDistance < 0.0f)
    {
        NormalW[DispatchRaysIndex().xy] = float4(0, 0, 0, 0);
        Albedo[DispatchRaysIndex().xy] = float4(0, 0, 0, 0);
        DirectLight[DispatchRaysIndex().xy] = float4(0, 0, 0, 0);
        Position[DispatchRaysIndex().xy] = float4(0, 0, 0, -1);

        return;
    }
    
    Payload payload = UnpackPayload(packedPayload);

    float3 directLight = CalculateDirectLight(payload);
    
    // Write the raytraced color to the output texture.
    NormalW[DispatchRaysIndex().xy] = float4((payload.ShadingNormalW + 1.0f) * 0.5f, 1);
    Albedo[DispatchRaysIndex().xy] = float4(payload.Albedo, 1);
    DirectLight[DispatchRaysIndex().xy] = float4(directLight, 1);
    Position[DispatchRaysIndex().xy] = float4(payload.PosW, payload.HitType);
}