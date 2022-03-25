#ifndef SAMPLERS_HLSL
#define SAMPLERS_HLSL

SamplerState SamPointWrap : register(s0);
SamplerState SamPointClamp : register(s1);
SamplerState SamLinearWrap : register(s2);
SamplerState SamLinearClamp : register(s3);
SamplerState SamAnisotropicWrap : register(s4);
SamplerState SamAnisotropicClamp : register(s5);

#endif