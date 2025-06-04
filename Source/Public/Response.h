#pragma once

#include "Core.h"
#include "MCP_Error.h"
#include "Message.h"

MCP_NAMESPACE_BEGIN

// Abstract base for responses (contains `ID`, `Error?`).
class ResponseBase : public MessageBase {
  public:
    RequestID ID;
    optional<MCP_Error> Error;
};

template <typename ResultType> class Response : public ResponseBase {
  public:
    ResultType Result; // Required result, ensure communication is operational
};

template <> class Response<void> : public ResponseBase {
  public:
    JSON Result = JSON::object(); // Empty JSON object for void specialization
    // TODO: Format JSON appropriately for the void specialization to share success with no
    // additional data
};

MCP_NAMESPACE_END