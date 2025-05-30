#pragma once

MCP_NAMESPACE_BEGIN

// The severity of a log message.
enum class LoggingLevel {
    Debug,
    Info,
    Notice,
    Warning,
    Error,
    Critical,
    Alert,
    Emergency,
};

MCP_NAMESPACE_END