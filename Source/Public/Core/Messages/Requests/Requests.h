#pragma once

#include "Core.h"
#include "Core/Messages/Messages.h"

MCP_NAMESPACE_BEGIN

// A request that expects a response.
class RequestMessage : public MessageBase {
  private:
    MessageID m_ID;
    string m_Method;
    optional<unique_ptr<MessageParams>> m_Params = nullopt;

  public:
    // Constructors
    RequestMessage(string Method, optional<unique_ptr<MessageParams>> Params = nullopt)
        : m_ID(0), m_Method(std::move(Method)), m_Params(std::move(Params)) {}

    // Direct Getters
    [[nodiscard]] MessageID GetMessageID() const;
    [[nodiscard]] string_view GetMethod() const;
    [[nodiscard]] optional<const MessageParams*> GetParams() const;

    // MessageBase Overrides
    [[nodiscard]] JSON ToJSON() const override;
    [[nodiscard]] unique_ptr<MessageBase> FromJSON(const JSON& InJSON) override;
    [[nodiscard]] string Serialize() const override;
    [[nodiscard]] unique_ptr<MessageBase> Deserialize(string InString) override;
};

bool IsRequestMessage(const JSON& value) {
    return value.is_object() && value.value(MSG_JSON_RPC, MSG_NULL) == MSG_JSON_RPC_VERSION
           && value.contains(MSG_ID) && value.contains(MSG_METHOD) && !value.contains(MSG_ERROR)
           && !value.contains(MSG_RESULT);
}

MCP_NAMESPACE_END