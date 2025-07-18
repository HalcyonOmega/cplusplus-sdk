#pragma once

#include <string_view>

#include "CoreSDK/Common/Macros.h"

MCP_NAMESPACE_BEGIN

void HandleRuntimeError(std::string_view InError);

void LogMessage(std::string_view InMessage);

MCP_NAMESPACE_END