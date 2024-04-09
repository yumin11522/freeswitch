//
// Created by xiongqimin on 2023/12/14.
//

#ifndef GMEET_CLIENT_LOGGER_H
#define GMEET_CLIENT_LOGGER_H

#include <cstdint>	// uint8_t
#include <cstdio>	// std::snprintf(), std::fprintf(), stdout, stderr
#include <cstdlib>	// std::abort()
#include <cstring>

namespace xmppclient
{
class Log
{
   public:
	enum class LLevel : uint8_t { LOG_NONE = 0, LOG_ERROR = 1, LOG_WARN = 2, LOG_DEBUG = 3, LOG_TRACE = 4 };
	class LogHandlerInterface
	{
	   public:
		virtual ~LogHandlerInterface() = default;

		virtual void OnLog(Log::LLevel level, char *payload, size_t len) = 0;
	};

	class DefaultLogHandler : public LogHandlerInterface
	{
		void OnLog(Log::LLevel level, char *payload, size_t len) override;
	};

	static void SetLogLevel(Log::LLevel level);

	static void SetHandler(LogHandlerInterface *handler);

	static void SetDefaultHandler();

   public:
	static Log::LLevel logLevel;
	static LogHandlerInterface *handler;
	static const size_t bufferSize{50000};
	static char buffer[];
};
}  // namespace xmppclient

// clang-format off

/* Logging macros. */

using Logger = xmppclient::Log;

#define _GMTC_SEPARATOR_CHAR "\n"

#ifdef GMTC_FILE_LINE
#define _GMTC_STR " %s:%d | %s::%s()"
#define _GMTC_STR_DESC _GMTC_STR " | "
#define _GMTC_FILE (std::strchr(__FILE__, '/') ? std::strchr(__FILE__, '/') + 1 : __FILE__)
#define _GMTC_ARG _GMTC_FILE, __LINE__, GMTC_CLASS, __FUNCTION__
#else
#define _GMTC_STR " %s::%s()"
#define _GMTC_STR_DESC _GMTC_STR " | "
#define _GMTC_ARG GMTC_CLASS, __FUNCTION__
#endif

#ifdef GMTC_TRACE
#define GMTC_TRACE() \
        do \
        { \
            if (xmppclient::Log::handler && xmppclient::Log::logLevel == xmppclient::Log::LLevel::LOG_DEBUG) \
            { \
                int loggerWritten = std::snprintf(xmppclient::Log::buffer, xmppclient::Log::bufferSize, _GMTC_STR, _GMTC_ARG); \
                xmppclient::Log::handler->OnLog(xmppclient::Log::LLevel::LOG_TRACE, xmppclient::Log::buffer, loggerWritten); \
            } \
        } \
        while (false)
#else
#define GMTC_TRACE() ;
#endif

#define GMTC_DEBUG(desc, ...) \
    do \
    { \
        if (xmppclient::Log::handler && xmppclient::Log::logLevel == xmppclient::Log::LLevel::LOG_DEBUG) \
        { \
            int loggerWritten = std::snprintf(Log::buffer, Log::bufferSize, _GMTC_STR_DESC desc, _GMTC_ARG, ##__VA_ARGS__); \
            xmppclient::Log::handler->OnLog(xmppclient::Log::LLevel::LOG_DEBUG, xmppclient::Log::buffer, loggerWritten); \
        } \
    } \
    while (false)

#define GMTC_WARN(desc, ...) \
    do \
    { \
        if (xmppclient::Log::handler && xmppclient::Log::logLevel >= xmppclient::Log::LLevel::LOG_WARN) \
        { \
            int loggerWritten = std::snprintf(xmppclient::Log::buffer, xmppclient::Log::bufferSize, _GMTC_STR_DESC desc, _GMTC_ARG, ##__VA_ARGS__); \
            xmppclient::Log::handler->OnLog(xmppclient::Log::LLevel::LOG_WARN, xmppclient::Log::buffer, loggerWritten); \
        } \
    } \
    while (false)

#define GMTC_ERROR(desc, ...) \
    do \
    { \
        if (xmppclient::Log::handler && xmppclient::Log::logLevel >= xmppclient::Log::LLevel::LOG_ERROR) \
        { \
            int loggerWritten = std::snprintf(xmppclient::Log::buffer, xmppclient::Log::bufferSize, _GMTC_STR_DESC desc, _GMTC_ARG, ##__VA_ARGS__); \
            xmppclient::Log::handler->OnLog(xmppclient::Log::LLevel::LOG_ERROR, xmppclient::Log::buffer, loggerWritten); \
        } \
    } \
    while (false)

#define GMTC_DUMP(desc, ...) \
    do \
    { \
        std::fprintf(stdout, _GMTC_STR_DESC desc _GMTC_SEPARATOR_CHAR, _GMTC_ARG, ##__VA_ARGS__); \
        std::fflush(stdout); \
    } \
    while (false)

#define GMTC_ABORT(desc, ...) \
    do \
    { \
        std::fprintf(stderr, "[ABORT]" _GMTC_STR_DESC desc _GMTC_SEPARATOR_CHAR, _GMTC_ARG, ##__VA_ARGS__); \
        std::fflush(stderr); \
        std::abort(); \
    } \
    while (false)

#define GMTC_ASSERT(condition, desc, ...) \
    if (!(condition)) \
    { \
        GMTC_ABORT("failed assertion `%s': " desc, #condition, ##__VA_ARGS__); \
    }

#endif //GMEET_CLIENT_LOGGER_H
