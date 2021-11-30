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
	XMFLOAT2 TexCoords0;
	XMFLOAT2 TexCoords1;
};

#endif // VERTICES_H