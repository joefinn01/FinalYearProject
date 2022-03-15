#include "RaytracingCommons.hlsli"

[shader("miss")]
void Miss(inout PackedPayload packedPayload)
{
    packedPayload.HitDistance = -1.0f;
}