#pragma once

#include <memory>
#include <utility>

#include "Core.h"
#include "Core/Constants/MessageConstants.h"

MCP_NAMESPACE_BEGIN

struct MessageID {
  private:
    // TODO: Is LongLong the right type or should it be double?
    variant<string, int, long long> m_MessageID;

  public:
    // Constructors
    MessageID(string StringID) : m_MessageID(std::move(StringID)) {}
    MessageID(int IntID) : m_MessageID(IntID) {}
    MessageID(long long LongLongID) : m_MessageID(LongLongID) {}

    // Direct Getters
    [[nodiscard]] string_view ToString() const;
};

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

    // Interface Methods
    [[nodiscard]] virtual JSON ToJSON() const = 0;
    [[nodiscard]] virtual unique_ptr<MessageBase> FromJSON(const JSON& InJSON) = 0;
    [[nodiscard]] virtual string Serialize() const;
    [[nodiscard]] virtual unique_ptr<MessageBase> Deserialize(string InString) = 0;
};

MCP_NAMESPACE_END