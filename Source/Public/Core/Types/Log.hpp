#pragma once

namespace MCP::Types {

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


} // namespace MCP::Types