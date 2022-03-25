#ifndef COMMONS_HLSL
#define COMMONS_HLSL

#ifndef HLSL
#define HLSL
#endif

#include "ConstantBuffers.h"
#include "Vertices.h"
#include "DescriptorTables.hlsli"
#include "Samplers.hlsli"

StructuredBuffer<GameObjectPerFrameCB> g_PrimitivePerFrameCB[] : register(t0, space101);
StructuredBuffer<PrimitiveInstanceCB> g_PrimitivePerInstanceCB[] : register(t0, space102);
StructuredBuffer<LightCB> g_LightCB[] : register(t0, space103);

ConstantBuffer<ScenePerFrameCB> g_ScenePerFrameCB : register(b0);

#endif