#pragma once

#include "Core.h"
#include "Message.h"

MCP_NAMESPACE_BEGIN

// Abstract base for requests (contains `ID`, `Method`).
class RequestBase : public MessageBase {
    RequestID m_ID;
    string m_Method;
};

template <typename ParamsType> class Request : public RequestBase {
    ParamsType m_Params; // The `Method` string is associated with the `ParamsType`.
};

MCP_NAMESPACE_END