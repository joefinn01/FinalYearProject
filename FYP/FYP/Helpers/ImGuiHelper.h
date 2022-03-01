#pragma once

#include <string>
#include <DirectXMath.h>

class ImGuiHelper
{
public:
	static bool DragFloat(const std::string& label, float& value, const float& columnWidth = 100.0f, const float& speed = 0.1f, const float& min = 0, const float& max = 0);
	static bool DragFloat3(const std::string& label, DirectX::XMFLOAT3& values, const float& columnWidth = 100.0f, const float& speed = 0.1f, const float& min = 0, const float& max = 0);
};

