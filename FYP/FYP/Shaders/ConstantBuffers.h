#ifndef CONSTANT_BUFFERS_H
#define CONSTANT_BUFFERS_H

#ifdef HLSL
#include "HlslCompat.h"
#else
#include <DirectXMath.h>

using namespace DirectX;
#endif

struct Viewport
{
	float Left;
	float Top;
	float Right;
	float Bottom;
};

struct RayGenerationCB
{
	Viewport View;
	Viewport Stencil;
};

#endif // CONSTANT_BUFFERS_H