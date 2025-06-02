#pragma once

#include "Core.h"
#include "MCP_Message.h"

MCP_NAMESPACE_BEGIN

// Abstract base for requests (contains `ID`, `Method`).
class MCP_RequestBase : public MCP_MessageBase {
    RequestID ID;
    string Method;
};

template <typename ParamsType> class MCP_Request : public MCP_RequestBase {
    ParamsType Params; // The `Method` string is associated with the `ParamsType`.
};

MCP_NAMESPACE_END