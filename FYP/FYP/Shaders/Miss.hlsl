#include "RaytracingCommons.hlsli"
#include "Payloads.hlsli"

[shader("miss")]
void Miss(inout PackedPayload packedPayload)
{
    packedPayload.HitDistance = -1.0f;
}