#pragma once

#include <string>
#include <variant>

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

// A progress token, used to associate progress notifications with the original request.
// TODO: @HalcyonOmega - Relook handling - schema does not have any subfields
struct ProgressToken {
    std::variant<std::string, int64_t> Token;

    JKEY(TOKENKEY, Token, "token")

    DEFINE_TYPE_JSON(ProgressToken, TOKENKEY)
};

MCP_NAMESPACE_END