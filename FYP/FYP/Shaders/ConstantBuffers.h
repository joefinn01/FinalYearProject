#ifndef CONSTANT_BUFFERS_H
#define CONSTANT_BUFFERS_H

#ifdef HLSL
#include "HlslCompat.h"
#else
#include <DirectXMath.h>

using namespace DirectX;
#endif

struct ScenePerFrameCB
{
	XMMATRIX InvWorldProjection;

	XMVECTOR EyePosW;
	XMVECTOR LightPosW;
	XMVECTOR LightAmbientColor;
	XMVECTOR LightDiffuseColor;
};

struct CubeCB
{
	XMFLOAT4 Albedo;
};

#endif // CONSTANT_BUFFERS_H