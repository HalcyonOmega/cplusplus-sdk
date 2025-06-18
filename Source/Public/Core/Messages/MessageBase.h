#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "Macros.h"
#include "MessageConstants.h"

class JSON;

MCP_NAMESPACE_BEGIN

using std::string;
using std::string_view;
using std::unique_ptr;

struct MessageParams {
    [[nodiscard]] virtual string Serialize() const;
    virtual MessageParams Deserialize(string InString) = 0;
};

class MessageBase {
  private:
    string m_JSONRPC = MSG_JSON_RPC_VERSION;

  public:
    // Direct Getters
    [[nodiscard]] string_view GetJSONRPCVersion() const;

    virtual ~MessageBase() = default;

    // Interface Methods - concrete implementations for empty message
    [[nodiscard]] virtual JSON ToJSON() const;
    [[nodiscard]] virtual unique_ptr<MessageBase> FromJSON(const JSON& InJSON);
    [[nodiscard]] virtual string Serialize() const;
    [[nodiscard]] virtual unique_ptr<MessageBase> Deserialize(string InString);
};

MCP_NAMESPACE_END