#include "DebugHelper.h"
#include "Commons/ScopedTimer.h"
#include "Include/ImGui/imgui.h"
#include "Helpers/ImGuiHelper.h"

#include <stdio.h>
#include <iostream>
#include <cstdarg>
#include <Windows.h>
#include <sstream>

std::unordered_map<std::string, double> DebugHelper::m_TimerTimes = std::unordered_map<std::string, double>();

#define BUFFER_SIZE 256

const std::wstring ksVerboseTag = L"VERBOSE: ";
const std::wstring ksWarningTag = L"WARNING: ";
const std::wstring ksErrorTag = L"ERROR: ";

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
	if (m_TimerTimes.count(pTimer->GetName()) == 0)
	{
		m_TimerTimes[pTimer->GetName()] = 0;
	}

	m_TimerTimes[pTimer->GetName()] += pTimer->DeltaTime();
}

void DebugHelper::ResetFrameTimes()
{
	for (std::unordered_map<std::string, double>::iterator it = m_TimerTimes.begin(); it != m_TimerTimes.end(); ++it)
	{
		it->second = 0;
	}
}

std::unordered_map<std::string, double>* DebugHelper::GetFrameTimes()
{
	return &m_TimerTimes;
}

void DebugHelper::ShowUI()
{
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::TextUnformatted("Time (ms)");

	ImGui::Spacing();

	for (std::unordered_map<std::string, double>::iterator it = m_TimerTimes.begin(); it != m_TimerTimes.end(); ++it)
	{
		ImGuiHelper::Text(it->first, "%f", 100.0f, it->second * 1000.0f);

		ImGui::Spacing();
	}
}
