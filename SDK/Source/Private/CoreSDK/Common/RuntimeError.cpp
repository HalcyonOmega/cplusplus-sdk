#include "CoreSDK/Common/RuntimeError.h"

#include <iostream>

MCP_NAMESPACE_BEGIN

void HandleRuntimeError(const std::string_view InError) {
    std::cout << "RuntimeError: " << InError << std::endl;
}

MCP_NAMESPACE_END