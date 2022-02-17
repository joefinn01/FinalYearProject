#ifndef VERTICES_H
#define VERTICES_H

#ifdef HLSL
#include "HlslCompat.h"
#else
#include <DirectXMath.h>

using namespace DirectX;
#endif

struct Vertex
{
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT4 Tangent;
	XMFLOAT2 TexCoords;
};

struct ScreenQuadVertex
{
#ifndef HLSL
	ScreenQuadVertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 texCoords)
	{
		Position = pos;
		TexCoords = texCoords;
	}
#endif

	XMFLOAT3 Position;
	XMFLOAT2 TexCoords;
};

#endif // VERTICES_H