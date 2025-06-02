#pragma once

#include "Core.h"
#include "MCP_Message.h"

MCP_NAMESPACE_BEGIN

// Standard structure for error details within responses.
class MCP_Error {
  public:
    MCP_Error(int code, const string& message, const optional<JSON>& data = nullopt)
        : Code(code), Message(message), Data(data) {}

    int GetCode() const {
        return Code;
    }
    const string& GetMessage() const {
        return Message;
    }
    const optional<JSON>& GetData() const {
        return Data;
    }

  private:
    int Code;
    string Message;
    optional<JSON> Data;
};

// Abstract base for responses (contains `ID`, `Error?`).
class MCP_ResponseBase : public MCP_MessageBase {
    RequestID ID;
    optional<MCP_Error> Error;
};

template <typename ResultType> class MCP_Response : public MCP_ResponseBase {
    ResultType Result; // Required result, ensure communication is operational
};

template <> class MCP_Response<void> : public MCP_ResponseBase {
    JSON Result = JSON::object(); // Empty JSON object for void specialization
    // TODO: Format JSON appropriately for the void specialization to share success with no
    // additional data
};

MCP_NAMESPACE_END