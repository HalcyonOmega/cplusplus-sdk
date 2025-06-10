#pragma once

#include "Core.h"
#include "Core/Constants/TransportConstants.h"

MCP_NAMESPACE_BEGIN

struct MessageID {
  private:
    // TODO: Is LongLong the right type or should it be double?
    variant<string, int, long long> m_MessageID;

  public:
    string_view GetIDString() const {
        return visit(
            [](const auto& VisitID) -> string_view {
                if constexpr (is_same_v<decay_t<decltype(VisitID)>, string>) {
                    return VisitID;
                } else {
                    static thread_local string Buffer;
                    Buffer = to_string(VisitID);
                    return Buffer;
                }
            },
            m_MessageID);
    }

    MessageID(string StringID) : m_MessageID(std::move(StringID)) {}
    MessageID(int IntID) : m_MessageID(IntID) {}
    MessageID(long long LongLongID) : m_MessageID(LongLongID) {}
};

struct MessageParams {
    JSON JSON_Object;
    virtual string Serialize() const = 0;
    virtual MessageParams Deserialize(string) = 0;
};

class MessageBase {
  private:
    string m_JSONRPC = TRANSPORT_JSONRPC_VERSION;

  public:
    string_view GetJSONRPCVersion() const {
        return m_JSONRPC;
    };

    virtual ~MessageBase() = default;
    virtual JSON ToJSON() const = 0;
};

class RequestMessage : public MessageBase {
  private:
    MessageID m_ID;
    string m_Method;
    optional<MessageParams> m_Params = nullopt;

  public: // Direct Getters
    MessageID GetMessageID() const {
        return m_ID;
    };

    string_view GetMethod() const {
        return m_Method;
    };

    optional<MessageParams> GetParams() const {
        if (m_Params.has_value()) { return m_Params.value(); }
        return nullopt;
    };

  private: // String Getters
    string_view GetMessageIDString() const {
        return GetMessageID().GetIDString();
    };

    string_view GetMethodString() const {
        return GetMethod();
    };

    string_view GetParamsString() const {
        if (m_Params.has_value()) { return m_Params.value().Serialize(); }
        return "";
    };

  public:
    JSON ToJSON() const override {
        JSON JSON_Object;
        JSON_Object["jsonrpc"] = GetJSONRPCVersion();
        JSON_Object["id"] = GetMessageIDString();
        JSON_Object["method"] = GetMethodString();
        JSON_Object["params"] = GetParamsString();
        return JSON_Object;
    }
};

class ResponseMessage : public MessageBase {
  private:
    MessageID m_ID;
    MessageParams m_Result;

  public:
    MessageID GetID() const {
        return m_ID;
    };

    MessageParams GetResult() const {
        return m_Result;
    };

    JSON ToJSON() const override {
        JSON j;
        j["jsonrpc"] = GetJSONRPCVersion();
        j["id"] = id_;
        j["result"] = result_;
        return j;
    }
};

class NotificationMessage : public MessageBase {
  private:
    string m_Method;
    optional<MessageParams> m_Params = nullopt;

  public:
    string GetMethod() const {
        return m_Method;
    };

    optional<MessageParams> GetParams() const {
        if (m_Params.has_value()) { return m_Params.value(); }
        return nullopt;
    };

    JSON ToJSON() const override {
        JSON j;
        j["jsonrpc"] = GetJSONRPCVersion();
        j["method"] = method_;
        j["params"] = params_;
        return j;
    }
};

struct ErrorParams {
    int Code;
    string Message;
    optional<any> Data;
};

class ErrorMessage : public MessageBase {
  private:
    MessageID m_ID;
    ErrorParams m_Error;

  public:
    MessageID GetID() const {
        return m_ID;
    };

    ErrorParams GetError() const {
        return m_Error;
    };

  private:
    string_view GetIDString() const {
        return GetID().GetIDString();
    };

    string_view GetErrorString() const {
        return GetError().Message;
    };

  public:
    JSON ToJSON() const override {
        JSON JSON_Object;
        JSON_Object["jsonrpc"] = GetJSONRPCVersion();
        JSON_Object["id"] = GetIDString();
        JSON_Object["error"] = GetErrorString();
        return JSON_Object;
    }
};

MCP_NAMESPACE_END