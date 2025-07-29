// Copyright (c) 2015-2025 Vector 35 Inc
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#define _CRT_SECURE_NO_WARNINGS
#include <cstdarg>
#include <cstdio>
#include <thread>
#include "binaryninjaapi.h"

using namespace BinaryNinja;
using namespace std;

void LogListener::LogMessageCallback(void* ctxt, size_t session, BNLogLevel level, const char* msg, const char* logger_name, size_t tid)
{
	LogListener* listener = (LogListener*)ctxt;
	listener->LogMessage(session, level, msg, logger_name, tid);
}


void LogListener::LogMessageWithStackTraceCallback(void* ctxt, size_t session, BNLogLevel level, const char* stackTrace,
	const char* msg, const char* logger_name, size_t tid)
{
	LogListener* listener = (LogListener*)ctxt;
	listener->LogMessageWithStackTrace(session, level, stackTrace, msg, logger_name, tid);
}


void LogListener::CloseLogCallback(void* ctxt)
{
	LogListener* listener = (LogListener*)ctxt;
	listener->CloseLog();
}


BNLogLevel LogListener::GetLogLevelCallback(void* ctxt)
{
	LogListener* listener = (LogListener*)ctxt;
	return listener->GetLogLevel();
}


void LogListener::RegisterLogListener(LogListener* listener)
{
	BNLogListener callbacks;
	callbacks.context = listener;
	callbacks.log = LogMessageCallback;
	callbacks.logWithStackTrace = LogMessageWithStackTraceCallback;
	callbacks.close = CloseLogCallback;
	callbacks.getLogLevel = GetLogLevelCallback;
	BNRegisterLogListener(&callbacks);
}


void LogListener::UnregisterLogListener(LogListener* listener)
{
	BNLogListener callbacks;
	callbacks.context = listener;
	callbacks.log = LogMessageCallback;
	callbacks.logWithStackTrace = LogMessageWithStackTraceCallback;
	callbacks.close = CloseLogCallback;
	callbacks.getLogLevel = GetLogLevelCallback;
	BNUnregisterLogListener(&callbacks);
}


void LogListener::UpdateLogListeners()
{
	BNUpdateLogListeners();
}


void LogListener::LogMessageWithStackTrace(size_t session, BNLogLevel level, const std::string&, const std::string& msg,
	const std::string& logger_name, size_t tid)
{
	LogMessage(session, level, msg, logger_name, tid);
}


static void PerformLog(size_t session, BNLogLevel level, const string& logger_name, size_t tid, const char* fmt, va_list args)
{
#if defined(_MSC_VER)
	int len = _vscprintf(fmt, args);
	if (len < 0)
		return;
	char* msg = (char*)malloc(len + 1);
	if (!msg)
		return;
	if (vsnprintf(msg, len + 1, fmt, args) >= 0)
		BNLog(session, level, logger_name.c_str(), tid, "%s", msg);
	free(msg);
#else
	char* msg;
	if (vasprintf(&msg, fmt, args) < 0)
		return;
	BNLog(session, level, logger_name.c_str(), tid, "%s", msg);
	free(msg);
#endif
}


static std::optional<string> GetStackTraceForException(const std::exception& e)
{
	const ExceptionWithStackTrace* est = dynamic_cast<const ExceptionWithStackTrace*>(&e);
	if (est)
	{
		if (est->m_stackTrace.empty())
			return std::nullopt;
		return est->m_stackTrace;
	}
	return std::nullopt;
}


static void PerformLogForException(size_t session, BNLogLevel level, const string& logger_name, size_t tid,
	const std::exception& e, const char* fmt, va_list args)
{
#if defined(_MSC_VER)
	int len = _vscprintf(fmt, args);
	if (len < 0)
		return;
	char* msg = (char*)malloc(len + 1);
	if (!msg)
		return;
	if (vsnprintf(msg, len + 1, fmt, args) >= 0)
	{
		auto stackTrace = GetStackTraceForException(e);
		if (stackTrace.has_value())
			BNLogWithStackTrace(session, level, logger_name.c_str(), tid, stackTrace.value().c_str(), "%s", msg);
		else
			BNLog(session, level, logger_name.c_str(), tid, "%s", msg);
	}
	free(msg);
#else
	char* msg;
	if (vasprintf(&msg, fmt, args) < 0)
		return;
	auto stackTrace = GetStackTraceForException(e);
	if (stackTrace.has_value())
		BNLogWithStackTrace(session, level, logger_name.c_str(), tid, stackTrace.value().c_str(), "%s", msg);
	else
		BNLog(session, level, logger_name.c_str(), tid, "%s", msg);
	free(msg);
#endif
}


void BinaryNinja::Log(BNLogLevel level, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLog(0, level, "", 0, fmt, args);
	va_end(args);
}


void BinaryNinja::LogTrace(const char* fmt, ...)
{
#ifdef _DEBUG
	va_list args;
	va_start(args, fmt);
	PerformLog(0, DebugLog, "", 0, fmt, args);
	va_end(args);
#endif
}


void BinaryNinja::LogDebug(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLog(0, DebugLog, "", 0, fmt, args);
	va_end(args);
}


void BinaryNinja::LogInfo(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLog(0, InfoLog, "", 0, fmt, args);
	va_end(args);
}


void BinaryNinja::LogWarn(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLog(0, WarningLog, "", 0, fmt, args);
	va_end(args);
}


void BinaryNinja::LogError(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLog(0, ErrorLog, "", 0, fmt, args);
	va_end(args);
}


void BinaryNinja::LogAlert(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLog(0, AlertLog, "", 0, fmt, args);
	va_end(args);
}


void BinaryNinja::LogForException(BNLogLevel level, const std::exception& e, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLogForException(0, level, "", 0, e, fmt, args);
	va_end(args);
}


void BinaryNinja::LogTraceForException(const std::exception& e, const char* fmt, ...)
{
#ifdef _DEBUG
	va_list args;
	va_start(args, fmt);
	PerformLogForException(0, DebugLog, "", 0, e, fmt, args);
	va_end(args);
#endif
}


void BinaryNinja::LogDebugForException(const std::exception& e, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLogForException(0, DebugLog, "", 0, e, fmt, args);
	va_end(args);
}


void BinaryNinja::LogInfoForException(const std::exception& e, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLogForException(0, InfoLog, "", 0, e, fmt, args);
	va_end(args);
}


void BinaryNinja::LogWarnForException(const std::exception& e, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLogForException(0, WarningLog, "", 0, e, fmt, args);
	va_end(args);
}


void BinaryNinja::LogErrorForException(const std::exception& e, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLogForException(0, ErrorLog, "", 0, e, fmt, args);
	va_end(args);
}


void BinaryNinja::LogAlertForException(const std::exception& e, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLogForException(0, AlertLog, "", 0, e, fmt, args);
	va_end(args);
}


void BinaryNinja::LogFV(BNLogLevel level, fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	Log(level, "%s", value.c_str());
}


void BinaryNinja::LogTraceFV(fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogTrace("%s", value.c_str());
}


void BinaryNinja::LogDebugFV(fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogDebug("%s", value.c_str());
}


void BinaryNinja::LogInfoFV(fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogInfo("%s", value.c_str());
}


void BinaryNinja::LogWarnFV(fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogWarn("%s", value.c_str());
}


void BinaryNinja::LogErrorFV(fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogError("%s", value.c_str());
}


void BinaryNinja::LogAlertFV(fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogAlert("%s", value.c_str());
}


void BinaryNinja::LogForExceptionFV(
	BNLogLevel level, const std::exception& e, fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogForException(level, e, "%s", value.c_str());
}


void BinaryNinja::LogTraceForExceptionFV(const std::exception& e, fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogTraceForException(e, "%s", value.c_str());
}


void BinaryNinja::LogDebugForExceptionFV(const std::exception& e, fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogDebugForException(e, "%s", value.c_str());
}


void BinaryNinja::LogInfoForExceptionFV(const std::exception& e, fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogInfoForException(e, "%s", value.c_str());
}


void BinaryNinja::LogWarnForExceptionFV(const std::exception& e, fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogWarnForException(e, "%s", value.c_str());
}


void BinaryNinja::LogErrorForExceptionFV(const std::exception& e, fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogErrorForException(e, "%s", value.c_str());
}


void BinaryNinja::LogAlertForExceptionFV(const std::exception& e, fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogAlertForException(e, "%s", value.c_str());
}


void BinaryNinja::LogToStdout(BNLogLevel minimumLevel)
{
	BNLogToStdout(minimumLevel);
}


void BinaryNinja::LogToStderr(BNLogLevel minimumLevel)
{
	BNLogToStderr(minimumLevel);
}


bool BinaryNinja::LogToFile(BNLogLevel minimumLevel, const string& path, bool append)
{
	return BNLogToFile(minimumLevel, path.c_str(), append);
}


void BinaryNinja::CloseLogs()
{
	BNCloseLogs();
}

size_t Logger::GetThreadId() const
{
	return std::hash<std::thread::id>{}(std::this_thread::get_id());
}

Logger::Logger(BNLogger* logger)
{
	m_object = logger;
}


Logger::Logger(const string& loggerName, size_t sessionId)
{
	m_object = BNLogCreateLogger(loggerName.c_str(), sessionId);
}


void Logger::Log(BNLogLevel level, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLog(GetSessionId(), level, GetName(), GetThreadId(), fmt::format("{}{}", GetIndent(), fmt).c_str(), args);
	va_end(args);
}


void Logger::LogTrace(const char* fmt, ...)
{
#ifdef _DEBUG
	va_list args;
	va_start(args, fmt);
	PerformLog(GetSessionId(), DebugLog, GetName(), GetThreadId(), fmt::format("{}{}", GetIndent(), fmt).c_str(), args);
	va_end(args);
#endif
}


void Logger::LogDebug(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLog(GetSessionId(), DebugLog, GetName(), GetThreadId(), fmt::format("{}{}", GetIndent(), fmt).c_str(), args);
	va_end(args);
}


void Logger::LogInfo(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLog(GetSessionId(), InfoLog, GetName(), GetThreadId(), fmt::format("{}{}", GetIndent(), fmt).c_str(), args);
	va_end(args);
}


void Logger::LogWarn(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLog(GetSessionId(), WarningLog, GetName(), GetThreadId(), fmt::format("{}{}", GetIndent(), fmt).c_str(), args);
	va_end(args);
}


void Logger::LogError(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLog(GetSessionId(), ErrorLog, GetName(), GetThreadId(), fmt::format("{}{}", GetIndent(), fmt).c_str(), args);
	va_end(args);
}


void Logger::LogAlert(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLog(GetSessionId(), AlertLog, GetName(), GetThreadId(), fmt::format("{}{}", GetIndent(), fmt).c_str(), args);
	va_end(args);
}


void Logger::LogForException(BNLogLevel level, const std::exception& e, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLogForException(
		GetSessionId(), level, GetName(), GetThreadId(), e, fmt::format("{}{}", GetIndent(), fmt).c_str(), args);
	va_end(args);
}


void Logger::LogTraceForException(const std::exception& e, const char* fmt, ...)
{
#ifdef _DEBUG
	va_list args;
	va_start(args, fmt);
	PerformLogForException(
		GetSessionId(), DebugLog, GetName(), GetThreadId(), e, fmt::format("{}{}", GetIndent(), fmt).c_str(), args);
	va_end(args);
#endif
}


void Logger::LogDebugForException(const std::exception& e, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLogForException(
		GetSessionId(), DebugLog, GetName(), GetThreadId(), e, fmt::format("{}{}", GetIndent(), fmt).c_str(), args);
	va_end(args);
}


void Logger::LogInfoForException(const std::exception& e, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLogForException(
		GetSessionId(), InfoLog, GetName(), GetThreadId(), e, fmt::format("{}{}", GetIndent(), fmt).c_str(), args);
	va_end(args);
}


void Logger::LogWarnForException(const std::exception& e, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLogForException(
		GetSessionId(), WarningLog, GetName(), GetThreadId(), e, fmt::format("{}{}", GetIndent(), fmt).c_str(), args);
	va_end(args);
}


void Logger::LogErrorForException(const std::exception& e, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLogForException(
		GetSessionId(), ErrorLog, GetName(), GetThreadId(), e, fmt::format("{}{}", GetIndent(), fmt).c_str(), args);
	va_end(args);
}


void Logger::LogAlertForException(const std::exception& e, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PerformLogForException(
		GetSessionId(), AlertLog, GetName(), GetThreadId(), e, fmt::format("{}{}", GetIndent(), fmt).c_str(), args);
	va_end(args);
}


void Logger::LogFV(BNLogLevel level, fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	Log(level, "%s", value.c_str());
}


void Logger::LogTraceFV(fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogTrace("%s", value.c_str());
}


void Logger::LogDebugFV(fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogDebug("%s", value.c_str());
}


void Logger::LogInfoFV(fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogInfo("%s", value.c_str());
}


void Logger::LogWarnFV(fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogWarn("%s", value.c_str());
}


void Logger::LogErrorFV(fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogError("%s", value.c_str());
}


void Logger::LogAlertFV(fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogAlert("%s", value.c_str());
}


void Logger::LogForExceptionFV(
	BNLogLevel level, const std::exception& e, fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogForException(level, e, "%s", value.c_str());
}


void Logger::LogTraceForExceptionFV(const std::exception& e, fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogTraceForException(e, "%s", value.c_str());
}


void Logger::LogDebugForExceptionFV(const std::exception& e, fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogDebugForException(e, "%s", value.c_str());
}


void Logger::LogInfoForExceptionFV(const std::exception& e, fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogInfoForException(e, "%s", value.c_str());
}


void Logger::LogWarnForExceptionFV(const std::exception& e, fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogWarnForException(e, "%s", value.c_str());
}


void Logger::LogErrorForExceptionFV(const std::exception& e, fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogErrorForException(e, "%s", value.c_str());
}


void Logger::LogAlertForExceptionFV(const std::exception& e, fmt::string_view format, fmt::format_args args)
{
	std::string value = fmt::vformat(format, args);
	LogAlertForException(e, "%s", value.c_str());
}


string Logger::GetName()
{
	char* name = BNLoggerGetName(m_object);
	string result = name;
	BNFreeString(name);
	return result;
}


size_t Logger::GetSessionId()
{
	return BNLoggerGetSessionId(m_object);
}


void Logger::Indent()
{
	BNLoggerIndent(m_object);
}


void Logger::Dedent()
{
	BNLoggerDedent(m_object);
}


void Logger::ResetIndent()
{
	BNLoggerResetIndent(m_object);
}


string Logger::GetIndent() const
{
	char* indent = BNGetLoggerIndent(m_object);
	if (!indent)
	{
		return "";
	}
	string result = indent;
	BNFreeString(indent);
	return result;
}


Ref<Logger> LogRegistry::CreateLogger(const std::string& loggerName, size_t sessionId)
{
	return new Logger(BNLogCreateLogger(loggerName.c_str(), sessionId));
}


Ref<Logger> LogRegistry::GetLogger(const std::string& loggerName, size_t sessionId)
{
	return new Logger(BNLogGetLogger(loggerName.c_str(), sessionId));
}


vector<string> LogRegistry::GetLoggerNames()
{
	size_t count = 0;
	char** names = BNLogGetLoggerNames(&count);
	vector<string> result;
	result.reserve(count);
	for (size_t i = 0; i < count; ++i)
		result.push_back(names[i]);
	BNFreeStringList(names, count);
	return result;
}
