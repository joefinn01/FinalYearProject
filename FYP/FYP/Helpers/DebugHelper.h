#pragma once

#include "Commons/ScopedTimer.h"
#include "Include/DirectX/d3dx12.h"

#include <utility>
#include <string>
#include <unordered_map>
#include <wrl.h>
#include <deque>

enum class LogLevel
{
	VERBOSE_LOG = 0,
	WARNING_LOG,
	ERROR_LOG
};

enum class GpuStats
{
	FULL_FRAME = 0,
	DRAW_VOLUME,
	TRACE_RAYS,
	BLEND_PROBES,
	ATLAS_BLEND_PROBES,
	BORDER_BLEND_PROBES,
	DEFERRED_PASS,
	GBUFFER,
	LIGHT,

	COUNT
};

#define PROFILE_TIMERS 1

#if PROFILE_TIMERS
#define PROFILE_RUN 0
#endif

#if _DEBUG || PROFILE_TIMERS

#define PROFILE(name) ScopedTimer profileTimer = ScopedTimer(name);

#define GPU_PROFILE_BEGIN(index, pGraphicsCommandList) pGraphicsCommandList->EndQuery(DebugHelper::GetQueryHeap(), D3D12_QUERY_TYPE_TIMESTAMP, (int)index * 2);
#define GPU_PROFILE_END(index, pGraphicsCommandList) pGraphicsCommandList->EndQuery(DebugHelper::GetQueryHeap(), D3D12_QUERY_TYPE_TIMESTAMP, ((int)index * 2) + 1);

#define WRITE_PROFILE_TIMES(runName) DebugHelper::WriteTimes(runName);

#else

#define PROFILE(name)

#define GPU_PROFILE_BEGIN(index, pGraphicsCommandList)
#define GPU_PROFILE_END(index, pGraphicsCommandList)

#define WRITE_PROFILE_TIMES(runName)

#endif

#if _DEBUG

#define LOG_VERBOSE(tag, format, ...) DebugHelper::Log((LogLevel)0, tag, format, __VA_ARGS__);
#define LOG_WARNING(tag, format, ...) DebugHelper::Log((LogLevel)1, tag, format, __VA_ARGS__);
#define LOG_ERROR(tag, format, ...) DebugHelper::Log((LogLevel)2, tag, format, __VA_ARGS__);

#else

#define LOG_VERBOSE(tag, format, ...)
#define LOG_WARNING(tag, format, ...)
#define LOG_ERROR(tag, format, ...)

#endif

#if PIX

#define PIX_ONLY(code) code


#else

#define PIX_ONLY(code)

#endif

typedef const std::wstring Tag;

class DebugHelper
{
public:
	static void Log(LogLevel logLevel, std::wstring sTag, std::wstring sText, ...);

	static void AddTime(ScopedTimer* pTimer);

	static void ResetFrameTimes();

	static std::unordered_map<std::string, double>* GetFrameTimes();

	static void ShowUI();

	static void Init(ID3D12Device* pDevice, ID3D12CommandQueue* pQueue);

	static void BeginFrame(ID3D12GraphicsCommandList* pGraphicsCommandList);
	static void EndFrame(ID3D12GraphicsCommandList* pGraphicsCommandList);

	static void ResolveTimestamps(ID3D12GraphicsCommandList* pGraphicsCommandList);
	static void UpdateTimestamps();

	static ID3D12QueryHeap* GetQueryHeap();

	static void WriteTimes(const std::string& ksRunNumber);

protected:

private:
	static double GetGpuTime(const std::deque<double>& kQueue);

	static std::unordered_map<std::string, double> s_TimerTimes;

	static Microsoft::WRL::ComPtr<ID3D12QueryHeap> s_pQueryHeap;
	static Microsoft::WRL::ComPtr<ID3D12Resource> s_pTimestampResource;

	static std::vector<std::deque<double>> s_GpuStatTimes;

	static UINT64 s_uiTimestampFrequency;

	static std::vector<std::string> s_sGpuStatNames;
};

