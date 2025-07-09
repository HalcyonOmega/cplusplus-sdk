#pragma once

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

// LoggingLevel {
//   MSG_DESCRIPTION:"The severity of a log message.\n\nThese map to syslog message severities, as specified in
//       RFC-5424:\nhttps://datatracker.ietf.org/doc/html/rfc5424#section-6.2.1",
//   MSG_ENUM: [
//     LOG_ALERT, LOG_CRITICAL, LOG_DEBUG, LOG_EMERGENCY, LOG_ERROR, LOG_INFO, LOG_NOTICE,
//     LOG_WARNING
//   ],
//   MSG_TYPE: MSG_STRING
// };

/**
 * The severity of a log message.
 * These map to syslog message severities, as specified in RFC-5424:
 * https://datatracker.ietf.org/doc/html/rfc5424#section-6.2.1
 */
enum class LoggingLevel
{
	Debug,
	Info,
	Notice,
	Warning,
	Error,
	Critical,
	Alert,
	Emergency
};

DEFINE_ENUM_JSON(LoggingLevel, { LoggingLevel::Debug, "debug" }, { LoggingLevel::Info, "info" },
	{ LoggingLevel::Notice, "notice" }, { LoggingLevel::Warning, "warning" }, { LoggingLevel::Error, "error" },
	{ LoggingLevel::Critical, "critical" }, { LoggingLevel::Alert, "alert" }, { LoggingLevel::Emergency, "emergency" })

class Logger
{
public:
	Logger();
	~Logger();

	static void Log(const std::string& InMessage, LoggingLevel InLevel = LoggingLevel::Info);

	static void Debug(const std::string& InMessage);
	static void Info(const std::string& InMessage);
	static void Notice(const std::string& InMessage);
	static void Warning(const std::string& InMessage);
	static void Error(const std::string& InMessage);
	static void Critical(const std::string& InMessage);
	static void Alert(const std::string& InMessage);
	static void Emergency(const std::string& InMessage);
};

MCP_NAMESPACE_END