#ifndef CONSTANT_BUFFERS_H
#define CONSTANT_BUFFERS_H

#ifdef HLSL
#include "HlslCompat.h"
#else
#include <DirectXMath.h>

#include "Helpers/ImGuiHelper.h"
#include "Include/ImGui/imgui.h"

using namespace DirectX;
#endif

#ifndef HLSL

enum class LightType : int
{
	DIRECTIONAL = 0,
	POINT,

	COUNT
};

#endif

struct LightCB
{
	XMFLOAT3 Position;

#ifdef HLSL
	int Type;
#else
	LightType Type;
#endif

	XMFLOAT3 Direction;	//This is NOT normalized.
	float Range;

	XMFLOAT3 Color;
	float Power;

	XMFLOAT3 Attenuation;
	int Enabled;

#ifndef HLSL

	void ShowUI()
	{
		std::string name;

		if (Type == LightType::DIRECTIONAL)
		{
			name = "Directional";
		}
		else if (Type == LightType::POINT)
		{
			name = "Point";
		}

		if (ImGui::TreeNode(name.c_str()))
		{
			ImGuiHelper::DragFloat3("Colour", Color, 150.0f, 0.01f, 0, 1);

			ImGui::Spacing();

			ImGuiHelper::DragFloat("Power", Power, 150.0f, 0.01f, 0, 100);

			ImGui::Spacing();

			bool bEnabled = (bool)Enabled;
			if (ImGuiHelper::Checkbox("Enabled", bEnabled, 150.0f) == true)
			{
				Enabled = (int)bEnabled;
			}

			ImGui::Spacing();

			if (Type == LightType::DIRECTIONAL)
			{
				ImGuiHelper::DragFloat3("Direction", Direction, 150.0f, 0.01f, -1.0f, 1.0f);

				ImGui::Spacing();
			}
			else if (Type == LightType::POINT)
			{
				ImGuiHelper::DragFloat3("Position", Position, 150.0f);

				ImGui::Spacing();

				ImGuiHelper::DragFloat3("Attenuation", Attenuation, 150.0f, 0.1f, 0.0f, 100.0f);

				ImGui::Spacing();

				ImGuiHelper::DragFloat("Range", Range, 150.0f, 0.1f, 0.0f, 100000.0f);
			}

			ImGui::TreePop();
		}

	}

#endif
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

	XMFLOAT3 EyeDirection;
	UINT32 ScreenHeight;
};

struct GameObjectPerFrameCB
{
	XMMATRIX World;
	XMMATRIX InvTransposeWorld;
};

struct PrimitiveIndexCB
{
	UINT32 InstanceIndex;
	UINT32 PrimitiveIndex;
	XMFLOAT2 pad;
};

struct PrimitiveInstanceCB
{
	UINT32 IndicesIndex;
	UINT32 VerticesIndex;
	UINT32 AlbedoIndex;
	UINT32 NormalIndex;

	XMFLOAT4 AlbedoColor;

	UINT32 MetallicRoughnessIndex;
	UINT32 OcclusionIndex;
	XMFLOAT2 pad;
};

struct DeferredPerFrameCB
{
	UINT32 AlbedoIndex;
	UINT32 DirectLightIndex;
	UINT32 PositionIndex;
	UINT32 NormalIndex;
};

struct RaytracePerFrameCB
{
	XMINT3 ProbeCounts;
	UINT32 RaysPerProbe;

	XMFLOAT3 ProbeSpacing;
	float MaxRayDistance;

	XMFLOAT3 VolumePosition;
	int RayDataFormat;

	XMFLOAT3 MissRadiance;
	float NormalBias;

	XMFLOAT4 RayRotation;

	float ViewBias;
	float DistancePower;
	float Hysteresis;
	float IrradianceGammaEncoding;

	float IrradianceThreshold;
	float BrightnessThreshold;
	int NumDistanceTexels;
	int NumIrradianceTexels;

	int IrradianceIndex;
	int DistanceIndex;
	int IrradianceFormat;
	int RayDataIndex;

	XMINT3 ProbeOffsets;
	float pad;

	XMINT3 ClearPlane;
	float pad2;
};

#endif // CONSTANT_BUFFERS_H