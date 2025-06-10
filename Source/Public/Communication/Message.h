#pragma once

#include "Core.h"
#include "Core/Constants/MessageConstants.h"

MCP_NAMESPACE_BEGIN

struct MessageID {
  private:
    // TODO: Is LongLong the right type or should it be double?
    variant<string, int, long long> m_MessageID;

  public:
    string_view ToString() const {
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
    virtual string Serialize() const = 0;
    virtual MessageParams Deserialize(string) = 0;
};

class MessageBase {
  private:
    string m_JSONRPC = MSG_JSON_RPC_VERSION;

  public:
    string_view GetJSONRPCVersion() const {
        return m_JSONRPC;
    };

    virtual ~MessageBase() = default;
    virtual JSON ToJSON() const = 0;
};

// A request that expects a response.
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
        return GetMessageID().ToString();
    };

    string_view GetMethodString() const {
        return GetMethod();
    };

    string_view GetParamsString() const {
        if (m_Params.has_value()) { return m_Params.value().Serialize(); }
        return MSG_NULL;
    };

  public:
    JSON ToJSON() const override {
        JSON JSON_Object;
        JSON_Object[MSG_JSON_RPC] = GetJSONRPCVersion();
        JSON_Object[MSG_ID] = GetMessageIDString();
        JSON_Object[MSG_METHOD] = GetMethodString();
        JSON_Object[MSG_PARAMS] = GetParamsString();
        return JSON_Object;
    }
};

// A successful (non-error) response to a request.
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

  private:
    string_view GetIDString() const {
        return GetID().ToString();
    };

    string_view GetResultString() const {
        return GetResult().Serialize();
    };

    JSON ToJSON() const override {
        JSON JSON_Object;
        JSON_Object[MSG_JSON_RPC] = GetJSONRPCVersion();
        JSON_Object[MSG_ID] = GetIDString();
        JSON_Object[MSG_RESULT] = GetResultString();
        return JSON_Object;
    }
};

// A notification which does not expect a response.
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

  private:
    string_view GetParamsString() const {
        if (m_Params.has_value()) { return m_Params.value().Serialize(); }
        return MSG_NULL;
    };

  public:
    JSON ToJSON() const override {
        JSON JSON_Object;
        JSON_Object[MSG_JSON_RPC] = GetJSONRPCVersion();
        JSON_Object[MSG_METHOD] = GetMethod();
        JSON_Object[MSG_PARAMS] = GetParamsString();
        return JSON_Object;
    }
};

struct ErrorParams {
    int Code;           // The error type that occurred.
    string Message;     // A short description of the error. The message SHOULD be limited to a
                        // concise single sentence.
    optional<any> Data; // Additional information about the error. The value of this member is
                        // defined by the sender (e.g. detailed error information, nested errors
                        // etc.)
};

// A response to a request that indicates an error occurred.
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
    string_view GetErrorString() const {
        return GetError().Message;
    };

  public:
    JSON ToJSON() const override {
        JSON JSON_Object;
        JSON_Object[MSG_JSON_RPC] = GetJSONRPCVersion();
        JSON_Object[MSG_ID] = GetID().ToString();
        // TODO: Fix Accessor
        JSON_Object[MSG_ERROR] = GetError().Message;
        return JSON_Object;
    }
};

bool IsRequestMessage(const JSON& value) {
    return value.is_object() && value.value(MSG_JSON_RPC, MSG_NULL) == MSG_JSON_RPC_VERSION
           && value.contains(MSG_ID) && value.contains(MSG_METHOD) && !value.contains(MSG_ERROR)
           && !value.contains(MSG_RESULT);
}

bool IsResponseMessage(const JSON& value) {
    return value.is_object() && value.value(MSG_JSON_RPC, MSG_NULL) == MSG_JSON_RPC_VERSION
           && value.contains(MSG_ID) && value.contains(MSG_RESULT) && !value.contains(MSG_ERROR);
}

bool IsNotificationMessage(const JSON& value) {
    return value.is_object() && value.value(MSG_JSON_RPC, MSG_NULL) == MSG_JSON_RPC_VERSION
           && value.contains(MSG_METHOD) && !value.contains(MSG_ID);
}

bool IsErrorMessage(const JSON& value) {
    return value.is_object() && value.value(MSG_JSON_RPC, MSG_NULL) == MSG_JSON_RPC_VERSION
           && value.contains(MSG_ID) && value.contains(MSG_ERROR) && !value.contains(MSG_RESULT);
}

// JSONRPCRequest {
//   "description" : "A request that expects a response.",
//                   "properties"
//       : {
//         "id" : {"$ref" : "#/definitions/RequestId"},
//         "jsonrpc" : {"const" : "2.0", "type" : "string"},
//         "method" : {"type" : "string"},
//         "params" : {
//           "additionalProperties" : {},
//           "properties" : {
//             "_meta" : {
//               "properties" : {
//                 "progressToken" : {
//                   "$ref" : "#/definitions/ProgressToken",
//                   "description" :
//                       "If specified, the caller is requesting out-of-band "
//                       "progress notifications for this request (as represented "
//                       "by notifications/progress). The value of this parameter "
//                       "is an opaque token that will be attached to any "
//                       "subsequent notifications. The receiver is not obligated "
//                       "to provide these notifications."
//                 }
//               },
//               "type" : "object"
//             }
//           },
//           "type" : "object"
//         }
//       },
//         "required" : [ "id", "jsonrpc", "method" ],
//                      "type" : "object"
// };

// JSONRPCResponse {
//   "description" : "A successful (non-error) response to a request.",
//                   "properties" : {
//                     "id" : {"$ref" : "#/definitions/RequestId"},
//                     "jsonrpc" : {"const" : "2.0", "type" : "string"},
//                     "result" : {"$ref" : "#/definitions/Result"}
//                   },
//                                  "required" : [ "id", "jsonrpc", "result" ],
//                                               "type" : "object"
// };

// JSONRPCNotification {
//   "description" : "A notification which does not expect a response.",
//                   "properties"
//       : {
//         "jsonrpc" : {"const" : "2.0", "type" : "string"},
//         "method" : {"type" : "string"},
//         "params" : {
//           "additionalProperties" : {},
//           "properties" : {
//             "_meta" : {
//               "additionalProperties" : {},
//               "description" : "This parameter name is reserved by MCP to allow "
//                               "clients and servers to attach additional "
//                               "metadata to their notifications.",
//               "type" : "object"
//             }
//           },
//           "type" : "object"
//         }
//       },
//         "required" : [ "jsonrpc", "method" ],
//                      "type" : "object"
// };

// JSONRPCBatchRequest {
//   "description" : "A JSON-RPC batch request, as described in "
//                   "https://www.jsonrpc.org/specification#batch.",
//                   "items" : {
//                     "anyOf" : [
//                       {"$ref" : "#/definitions/JSONRPCRequest"},
//                       {"$ref" : "#/definitions/JSONRPCNotification"}
//                     ]
//                   },
//                             "type" : "array"
// };

/**
 * A JSON-RPC batch request, as described in
 * https://www.jsonrpc.org/specification#batch.
 */
using BatchRequestMessage = vector<variant<RequestMessage, NotificationMessage>>;

// JSONRPCBatchResponse {
//   "description" : "A JSON-RPC batch response, as described in "
//                   "https://www.jsonrpc.org/specification#batch.",
//                   "items" : {
//                     "anyOf" : [
//                       {"$ref" : "#/definitions/JSONRPCResponse"},
//                       {"$ref" : "#/definitions/JSONRPCError"}
//                     ]
//                   },
//                             "type" : "array"
// };

/**
 * A JSON-RPC batch response, as described in
 * https://www.jsonrpc.org/specification#batch.
 */
using BatchResponseMessage = vector<variant<ResponseMessage, ErrorMessage>>;

// MessageBase {
//   "anyOf" : [
//     {"$ref" : "#/definitions/JSONRPCRequest"},
//     {"$ref" : "#/definitions/JSONRPCNotification"}, {
//       "description" : "A JSON-RPC batch request, as described in "
//                       "https://www.jsonrpc.org/specification#batch.",
//       "items" : {
//         "anyOf" : [
//           {"$ref" : "#/definitions/JSONRPCRequest"},
//           {"$ref" : "#/definitions/JSONRPCNotification"}
//         ]
//       },
//       "type" : "array"
//     },
//     {"$ref" : "#/definitions/JSONRPCResponse"},
//     {"$ref" : "#/definitions/JSONRPCError"}, {
//       "description" : "A JSON-RPC batch response, as described in "
//                       "https://www.jsonrpc.org/specification#batch.",
//       "items" : {
//         "anyOf" : [
//           {"$ref" : "#/definitions/JSONRPCResponse"},
//           {"$ref" : "#/definitions/JSONRPCError"}
//         ]
//       },
//       "type" : "array"
//     }
//   ],
//   "description" : "Refers to any valid JSON-RPC object that can be "
//                   "decoded off the wire, or encoded to be sent."
// };

/**
 * Refers to any valid JSON-RPC object that can be decoded off the wire, or
 * encoded to be sent.
 */
using JSON_RPC_Message = variant<RequestMessage, NotificationMessage, ResponseMessage, ErrorMessage,
                                 BatchRequestMessage, BatchResponseMessage>;

MCP_NAMESPACE_END