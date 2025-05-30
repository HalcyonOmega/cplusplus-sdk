#include "Utilities/Autocomplete/Completable.h"

MCP_NAMESPACE_BEGIN

constexpr string_view GetMCP_TypeKind(MCP_TypeKind Kind) {
    switch(Kind) {
        case MCP_TypeKind::Completable:
            return "McpCompletable";
        default:
            return "Unknown";
    }
}

MCP_NAMESPACE_END 