#pragma once

#include "Core.h"
#include "Message.h"

MCP_NAMESPACE_BEGIN

// Abstract base for requests (contains `ID`, `Method`).
class RequestBase : public MessageBase {
    RequestID ID;
    string Method;
};

template <typename ParamsType> class Request : public RequestBase {
    ParamsType Params; // The `Method` string is associated with the `ParamsType`.
};

MCP_NAMESPACE_END