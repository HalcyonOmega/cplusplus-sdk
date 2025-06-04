#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

/**
 * Validates a JSON-RPC message format
 * @param message The message to validate
 * @return true if the message is valid JSON-RPC format
 */
bool IsValidJSONRPC(const std::string& message);

/**
 * Validates UTF-8 encoding
 * @param message The message to validate
 * @return true if the message is valid UTF-8
 */
bool IsValidUTF8(const std::string& message);

/**
 * Validates protocol version
 * @param version The version string to validate
 * @return true if the version is supported
 */
bool IsValidProtocolVersion(const std::string& version);

/**
 * Validates transport state
 * @param isRunning Current running state
 * @param isConnected Current connection state
 * @return true if the state is valid for the requested operation
 */
bool IsValidState(bool isRunning, bool isConnected);

MCP_NAMESPACE_END