#pragma once

#include "Core.h"
#include "MCP_Error.h"
#include "Message.h"

MCP_NAMESPACE_BEGIN

// Abstract base for responses (contains `ID`, `Error?`).
class ResponseBase : public MessageBase {
  public:
    RequestID m_ID;
    optional<MCP_Error> m_Error;
};

template <typename ResultType> class Response : public ResponseBase {
  public:
    ResultType m_Result; // Required result, ensure communication is operational
};

MCP_NAMESPACE_END