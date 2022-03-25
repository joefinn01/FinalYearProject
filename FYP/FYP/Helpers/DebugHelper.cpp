#include "DebugHelper.h"
#include "Commons/ScopedTimer.h"
#include "Include/ImGui/imgui.h"
#include "Helpers/ImGuiHelper.h"

#include <stdio.h>
#include <iostream>
#include <cstdarg>
#include <Windows.h>
#include <sstream>

std::unordered_map<std::string, double> DebugHelper::s_TimerTimes = std::unordered_map<std::string, double>();
Microsoft::WRL::ComPtr<ID3D12QueryHeap> DebugHelper::s_pQueryHeap = nullptr;
Microsoft::WRL::ComPtr<ID3D12Resource> DebugHelper::s_pTimestampResource = nullptr;
UINT64 DebugHelper::s_uiTimestampFrequency = 0;
std::vector<double> DebugHelper::s_GpuStatTimes = std::vector<double>();

std::vector<std::string> DebugHelper::s_sGpuStatNames =
{
	"Full Frame",
	"Draw GI Volume",
	"Trace Rays",
	"Blend Probes",
	"Blend Probe Atlases",
	"Blend Probe Borders",
	"Deferred Pass",
	"G Buffer Pass",
	"Light Pass"
};

#define BUFFER_SIZE 256

const std::wstring ksVerboseTag = L"VERBOSE: ";
const std::wstring ksWarningTag = L"WARNING: ";
const std::wstring ksErrorTag = L"ERROR: ";

Tag tag = L"DebugHelper";

void DebugHelper::Log(LogLevel logLevel, std::wstring sTag, std::wstring sText, ...)
{
	wchar_t buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);

	std::wstringstream ss;

	ss << sTag << "::";

	switch (logLevel)
	{
	case LogLevel::VERBOSE_LOG:
		ss << ksVerboseTag;
		break;

	case LogLevel::WARNING_LOG:
		ss << ksWarningTag;
		break;

	case LogLevel::ERROR_LOG:
		ss << ksErrorTag;
		break;

	default:
		break;
	}

	ss << sText << std::endl;

	

	//pass in varidic arguments and output to the console.
	va_list args;
	va_start(args, sText);
	vswprintf_s(buffer, BUFFER_SIZE, ss.str().c_str(), args);
	OutputDebugString(buffer);
	va_end(args);
}

void DebugHelper::AddTime(ScopedTimer* pTimer)
{
	if (s_TimerTimes.count(pTimer->GetName()) == 0)
	{
		s_TimerTimes[pTimer->GetName()] = 0;
	}

	s_TimerTimes[pTimer->GetName()] += pTimer->DeltaTime();
}

void DebugHelper::ResetFrameTimes()
{
	for (std::unordered_map<std::string, double>::iterator it = s_TimerTimes.begin(); it != s_TimerTimes.end(); ++it)
	{
		it->second = 0;
	}
}

std::unordered_map<std::string, double>* DebugHelper::GetFrameTimes()
{
	return &s_TimerTimes;
}

void DebugHelper::ShowUI()
{
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::TextUnformatted("CPU Times (ms)");

	ImGui::Spacing();

	for (std::unordered_map<std::string, double>::iterator it = s_TimerTimes.begin(); it != s_TimerTimes.end(); ++it)
	{
		ImGuiHelper::Text(it->first, "%f", 150.0f, it->second * 1000.0f);

		ImGui::Spacing();
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::TextUnformatted("GPU Times (ms)");	

	ImGui::Spacing();

	for (int i = 0; i < (int)GpuStats::COUNT; ++i)
	{
		ImGuiHelper::Text(s_sGpuStatNames[i], "%f", 150.0f, s_GpuStatTimes[i]);

		ImGui::Spacing();
	}
}

void DebugHelper::Init(ID3D12Device* pDevice, ID3D12CommandQueue* pQueue)
{
	D3D12_QUERY_HEAP_DESC desc;
	desc.Count = (int)GpuStats::COUNT * 2;
	desc.NodeMask = 0;
	desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;

	HRESULT hr = pDevice->CreateQueryHeap(&desc, IID_PPV_ARGS(&s_pQueryHeap));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create query heap!");

		return;
	}

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.Width = (int)GpuStats::COUNT * sizeof(UINT64) * 2;
	resourceDesc.Height = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_READBACK;

	// Create the timestamps resource
	hr = pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&s_pTimestampResource));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create buffer to contain query timestamps!");

		return;
	}

	hr = pQueue->GetTimestampFrequency(&s_uiTimestampFrequency);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to get timestamp frequency!");

		return;
	}

	s_GpuStatTimes.resize((int)GpuStats::COUNT);
}

void DebugHelper::BeginFrame(ID3D12GraphicsCommandList* pGraphicsCommandList)
{
	pGraphicsCommandList->EndQuery(s_pQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0);
}

void DebugHelper::EndFrame(ID3D12GraphicsCommandList* pGraphicsCommandList)
{
	pGraphicsCommandList->EndQuery(s_pQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 1);
}

void DebugHelper::ResolveTimestamps(ID3D12GraphicsCommandList* pGraphicsCommandList)
{
	pGraphicsCommandList->ResolveQueryData(s_pQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, (int)GpuStats::COUNT * 2, s_pTimestampResource.Get(), 0);
}

void DebugHelper::UpdateTimestamps()
{
	UINT64* pData = nullptr;

	std::vector<UINT64> gpuQueries;
	gpuQueries.resize((int)GpuStats::COUNT * 2);

	HRESULT hr = s_pTimestampResource->Map(0, nullptr, (void**)(&pData));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to map GPU stat resource!");

		return;
	}

	memcpy(gpuQueries.data(), pData, sizeof(UINT64) * (int)GpuStats::COUNT * 2);

	s_pTimestampResource->Unmap(0, nullptr);

	UINT64 elapsedTicks;
	for (int i = 0; i < (int)GpuStats::COUNT * 2; i += 2)
	{
		if (gpuQueries[i] == 0)
		{
			continue;
		}

		s_GpuStatTimes[(int)(i * 0.5f)] = (1000.0 * (double)(gpuQueries[i + 1] - gpuQueries[i])) / (double)s_uiTimestampFrequency;
	}
}

ID3D12QueryHeap* DebugHelper::GetQueryHeap()
{
	return s_pQueryHeap.Get();
}
