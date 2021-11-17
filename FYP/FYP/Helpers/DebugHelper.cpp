#include "DebugHelper.h"

#include <stdio.h>
#include <iostream>
#include <cstdarg>
#include <Windows.h>
#include <sstream>

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
	//fprintf(stdout, buffer);
	//std::cout << std::endl;
	va_end(args);
}
