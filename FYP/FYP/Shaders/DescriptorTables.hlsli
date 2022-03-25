#ifndef DESCRIPTOR_TABLES_HLSL
#define DESCRIPTOR_TABLES_HLSL

Texture2D Tex2DTable[] : register(t0, space0);
Buffer<uint> BufferUintTable[] : register(t0, space1);
Texture2D<float> FloatTex2DTable[] : register(t0, space2);

#endif