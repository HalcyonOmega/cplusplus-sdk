#pragma once

#include "Core.h"
#include "Core/Messages/Messages.h"

MCP_NAMESPACE_BEGIN

// A successful (non-error) response to a request.
class ResponseMessage : public MessageBase {
  private:
    MessageID m_ID;
    unique_ptr<MessageParams> m_Result;

  public:
    // Constructors
    ResponseMessage(MessageID MessageID, unique_ptr<MessageParams> Result)
        : m_ID(std::move(MessageID)), m_Result(std::move(Result)) {}

    // Direct Getters
    [[nodiscard]] MessageID GetMessageID() const;
    [[nodiscard]] const MessageParams* GetResult() const;

    // MessageBase Overrides
    [[nodiscard]] JSON ToJSON() const override;
    [[nodiscard]] unique_ptr<MessageBase> FromJSON(const JSON& InJSON) override;
    [[nodiscard]] string Serialize() const override;
    [[nodiscard]] unique_ptr<MessageBase> Deserialize(string InString) override;
};

bool IsResponseMessage(const JSON& value) {
    return value.is_object() && value.value(MSG_JSON_RPC, MSG_NULL) == MSG_JSON_RPC_VERSION
           && value.contains(MSG_ID) && value.contains(MSG_RESULT) && !value.contains(MSG_ERROR);
}

MCP_NAMESPACE_END