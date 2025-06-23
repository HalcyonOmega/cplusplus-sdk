#pragma once

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

enum class LoggingLevel { Debug, Info, Notice, Warning, Error, Critical, Alert, Emergency };

DEFINE_ENUM_JSON(LoggingLevel, {LoggingLevel::Debug, "debug"}, {LoggingLevel::Info, "info"},
                 {LoggingLevel::Notice, "notice"}, {LoggingLevel::Warning, "warning"},
                 {LoggingLevel::Error, "error"}, {LoggingLevel::Critical, "critical"},
                 {LoggingLevel::Alert, "alert"}, {LoggingLevel::Emergency, "emergency"})

MCP_NAMESPACE_END