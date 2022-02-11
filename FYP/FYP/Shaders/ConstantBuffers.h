#ifndef CONSTANT_BUFFERS_H
#define CONSTANT_BUFFERS_H

#ifdef HLSL
#include "HlslCompat.h"
#else
#include <DirectXMath.h>

using namespace DirectX;
#endif

struct LightCB
{
	XMFLOAT3 Position;
	int Type;

	XMFLOAT4 Color;

	XMFLOAT3 Attenuation;
	float pad;
};

struct ScenePerFrameCB
{
	XMMATRIX InvWorldProjection;

	XMFLOAT3 EyePosW;
	int NumLights;

	int LightIndex;
	int PrimitivePerFrameIndex;
	int PrimitivePerInstanceIndex;
	float pad;
};

struct PrimitivePerFrameCB
{
	XMMATRIX World;
	XMMATRIX InvWorld;
};

struct PrimitiveIndexCB
{
	UINT32 Index;
	XMFLOAT3 pad;
};

struct PrimitiveInstanceCB
{
	UINT32 IndicesIndex;
	UINT32 VerticesIndex;
	UINT32 AlbedoIndex;
	UINT32 NormalIndex;

	UINT32 MetallicRoughnessIndex;
	UINT32 OcclusionIndex;
	XMFLOAT2 pad;
};

#endif // CONSTANT_BUFFERS_H