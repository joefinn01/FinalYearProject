#define HLSL

#include "ConstantBuffers.h"
#include "Vertices.h"
#include "DescriptorTables.hlsli"

StructuredBuffer<PrimitivePerFrameCB> g_PrimitivePerFrameCB[] : register(t0, space101);
StructuredBuffer<PrimitiveInstanceCB> g_PrimitivePerInstanceCB[] : register(t0, space102);
StructuredBuffer<LightCB> g_LightCB[] : register(t0, space103);

ConstantBuffer<ScenePerFrameCB> g_ScenePerFrameCB : register(b0);

SamplerState SamPointWrap : register(s0);
SamplerState SamPointClamp : register(s1);
SamplerState SamLinearWrap : register(s2);
SamplerState SamLinearClamp : register(s3);
SamplerState SamAnisotropicWrap : register(s4);
SamplerState SamAnisotropicClamp : register(s5);

static const float PI = 3.14159265f;