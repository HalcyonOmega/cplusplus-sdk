#pragma once

#include "Core.h"
#include "Core/Constants/MessageConstants.h"
#include "Core/Messages/Messages.h"

MCP_NAMESPACE_BEGIN

// A notification which does not expect a response.
class NotificationMessage : public MessageBase {
  private:
    string m_Method;
    optional<unique_ptr<MessageParams>> m_Params = nullopt;

  public:
    // Constructors
    NotificationMessage(string Method, optional<unique_ptr<MessageParams>> Params = nullopt)
        : m_Method(std::move(Method)), m_Params(std::move(Params)) {}

    // Direct Getters
    [[nodiscard]] string_view GetMethod() const;
    [[nodiscard]] optional<const MessageParams*> GetParams() const;

    // MessageBase Overrides
    [[nodiscard]] JSON ToJSON() const override;
    [[nodiscard]] unique_ptr<MessageBase> FromJSON(const JSON& InJSON) override;
    [[nodiscard]] string Serialize() const override;
    [[nodiscard]] unique_ptr<MessageBase> Deserialize(string InString) override;
};

bool IsNotificationMessage(const JSON& value) {
    return value.is_object() && value.value(MSG_JSON_RPC, MSG_NULL) == MSG_JSON_RPC_VERSION
           && value.contains(MSG_METHOD) && !value.contains(MSG_ID);
}

MCP_NAMESPACE_END