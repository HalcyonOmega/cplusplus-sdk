#pragma once

#include "Core.h"
#include "Core/Constants/ErrorConstants.h"
#include "Core/Messages/MessageBase.h"

MCP_NAMESPACE_BEGIN

// TODO: @HalcyonOmega Consider subclassing ErrorMessage from ResponseMessage

struct ErrorParams {
    Errors Code;        // The error type that occurred.
    string Message;     // A short description of the error. The message SHOULD be limited to a
                        // concise single sentence.
    optional<any> Data; // Additional information about the error. The value of this member is
                        // defined by the sender (e.g. detailed error information, nested errors
                        // etc.)
};

// A response to a request that indicates an error occurred.
class ErrorMessage : public MessageBase {
  private:
    RequestID m_ID;
    ErrorParams m_Error;

  public:
    // Constructors
    // TODO: Check RequestID default IntID = 0 - should this be another default?
    ErrorMessage(Errors Code, string Message, optional<any> Data = nullopt)
        : m_ID(0), m_Error({.Code = Code, .Message = Message, .Data = Data}) {}

    ErrorMessage(RequestID RequestID, Errors Code, string Message)
        : m_ID(std::move(RequestID)), m_Error({.Code = Code, .Message = Message, .Data = nullopt}) {
    }

    ErrorMessage(RequestID RequestID, Errors Code, string Message, optional<any> Data = nullopt)
        : m_ID(std::move(RequestID)), m_Error({.Code = Code, .Message = Message, .Data = Data}) {}

    ErrorMessage(RequestID RequestID, ErrorParams Error)
        : m_ID(std::move(RequestID)), m_Error(std::move(Error)) {}

    // Direct Getters
    [[nodiscard]] RequestID GetID() const;
    [[nodiscard]] ErrorParams GetError() const;

    // MessageBase Overrides
    [[nodiscard]] JSON ToJSON() const override;
    [[nodiscard]] unique_ptr<MessageBase> FromJSON(const JSON& InJSON) override;
    [[nodiscard]] string Serialize() const override;
    [[nodiscard]] unique_ptr<MessageBase> Deserialize(string InString) override;
};

bool IsErrorMessage(const JSON& value) {
    return value.is_object() && value.value(MSG_JSON_RPC, MSG_NULL) == MSG_JSON_RPC_VERSION
           && value.contains(MSG_ID) && value.contains(MSG_ERROR) && !value.contains(MSG_RESULT);
}

MCP_NAMESPACE_END