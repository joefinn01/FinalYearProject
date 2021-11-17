#pragma once

#include <utility>
#include <string>

enum class LogLevel
{
	VERBOSE_LOG = 0,
	WARNING_LOG,
	ERROR_LOG
};

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
protected:

private:

};

