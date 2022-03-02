#include "ImGuiHelper.h"

#include "Include/ImGui/imgui.h"
#include "Include/ImGui/imgui_internal.h"

bool ImGuiHelper::DragFloat(const std::string& label, float& value, const float& columnWidth, const float& speed, const float& min, const float& max)
{
	bool bUpdated = false;

	ImGui::PushID(label.c_str());

	ImGui::Columns(2);

	ImGui::SetColumnWidth(0, columnWidth);
	ImGui::Text(label.c_str());
	ImGui::NextColumn();

	ImGui::PushItemWidth(ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

	bUpdated = ImGui::DragFloat("##Float", &value, speed, min, max, "%.2f");

	ImGui::PopItemWidth();

	ImGui::PopStyleVar();

	ImGui::Columns(1);

	ImGui::PopID();

	return bUpdated;
}

bool ImGuiHelper::DragFloat3(const std::string& label, DirectX::XMFLOAT3& values, const float& columnWidth, const float& speed, const float& min, const float& max)
{
	ImGuiIO& io = ImGui::GetIO();

	bool bUpdated = false;

	ImGui::PushID(label.c_str());

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	ImGui::Text(label.c_str());
	ImGui::NextColumn();

	ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

	float lineHeight = io.Fonts->Fonts[0]->FontSize + ImGui::GetStyle().FramePadding.y * 2.0f;
	ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	ImGui::Button("X", buttonSize);
	ImGui::SameLine();

	if (ImGui::DragFloat("##X", &values.x, speed, min, max, "%.2f") == true)
	{
		bUpdated = true;
	}

	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::Button("Y", buttonSize);
	ImGui::SameLine();

	if (ImGui::DragFloat("##Y", &values.y, speed, min, max, "%.2f") == true)
	{
		bUpdated = true;
	}

	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::Button("Z", buttonSize);
	ImGui::SameLine();

	if (ImGui::DragFloat("##Z", &values.z, speed, min, max, "%.2f") == true)
	{
		bUpdated = true;
	}

	ImGui::PopItemWidth();

	ImGui::PopStyleVar();

	ImGui::Columns(1);

	ImGui::PopID();

	return bUpdated;
}

void ImGuiHelper::Text(const std::string& label, const char* format, float columnWidth, ...)
{
	ImGuiIO& io = ImGui::GetIO();

	ImGui::PushID(label.c_str());

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	ImGui::TextUnformatted(label.c_str());
	ImGui::NextColumn();

	ImGui::PushItemWidth(ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

	va_list args;
	va_start(args, columnWidth);

	ImGui::TextV(format, args);

	va_end(args);

	ImGui::PopItemWidth();

	ImGui::PopStyleVar();

	ImGui::Columns(1);

	ImGui::PopID();
}
