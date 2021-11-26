#include "Commons.hlsli"



[shader("raygeneration")]
void RayGen()
{
    float3 rayDirectionW;
    float3 originW;
    
    GenerateCameraRay(DispatchRaysIndex().xy, originW, rayDirectionW);

    // Trace the ray.
    // Set the ray's extents.
    RayDesc ray;
    ray.Origin = originW;
    ray.Direction = rayDirectionW;
    // Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
    // TMin should be kept small to prevent missing geometry at close contact areas.
    ray.TMin = 0.001;
    ray.TMax = 10000.0;
    RayPayload payload = { float4(0, 0, 0, 0) };
    TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);

    // Write the raytraced color to the output texture.
    RenderTarget[DispatchRaysIndex().xy] = payload.color;
}