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
	XMMATRIX ViewProjection;
	XMMATRIX InvViewProjection;

	XMFLOAT3 EyePosW;
	UINT32 NumLights;

	UINT32 LightIndex;
	UINT32 PrimitivePerFrameIndex;
	UINT32 PrimitivePerInstanceIndex;
	UINT32 ScreenWidth;

	UINT32 ScreenHeight;
	XMFLOAT3 pad;
};

struct PrimitivePerFrameCB
{
	XMMATRIX World;
	XMMATRIX InvTransposeWorld;
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

struct DeferredPerFrameCB
{
	UINT32 NormalIndex;
	UINT32 AlbedoIndex;
	UINT32 MetallicRoughnessOcclusion;
	UINT32 DepthIndex;
};

#endif // CONSTANT_BUFFERS_H