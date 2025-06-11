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
//   MSG_DESCRIPTION : "A request that expects a response.",
//                   MSG_PROPERTIES
//       : {
//         MSG_ID : {"$ref" : "#/definitions/RequestId"},
//         MSG_JSON_RPC : {MSG_CONST : MSG_JSON_RPC_VERSION, MSG_TYPE : MSG_STRING},
//         MSG_METHOD : {MSG_TYPE : MSG_STRING},
//         MSG_PARAMS : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_PROPERTIES : {
//             MSG_META : {
//               MSG_PROPERTIES : {
//                 MSG_PROGRESS_TOKEN : {
//                   "$ref" : "#/definitions/ProgressToken",
//                   MSG_DESCRIPTION :
//                       "If specified, the caller is requesting out-of-band "
//                       "progress notifications for this request (as represented "
//                       "by notifications/progress). The value of this parameter "
//                       "is an opaque token that will be attached to any "
//                       "subsequent notifications. The receiver is not obligated "
//                       "to provide these notifications."
//                 }
//               },
//               MSG_TYPE : MSG_OBJECT
//             }
//           },
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED : [ MSG_ID, MSG_JSON_RPC, MSG_METHOD ],
//                      MSG_TYPE : MSG_OBJECT
// };

// JSONRPCResponse {
//   MSG_DESCRIPTION : "A successful (non-error) response to a request.",
//                   MSG_PROPERTIES : {
//                     MSG_ID : {"$ref" : "#/definitions/RequestId"},
//                     MSG_JSON_RPC : {MSG_CONST : MSG_JSON_RPC_VERSION, MSG_TYPE : MSG_STRING},
//                     MSG_RESULT : {"$ref" : "#/definitions/Result"}
//                   },
//                                  MSG_REQUIRED : [ MSG_ID, MSG_JSON_RPC, MSG_RESULT ],
//                                               MSG_TYPE : MSG_OBJECT
// };

// JSONRPCNotification {
//   MSG_DESCRIPTION : "A notification which does not expect a response.",
//                   MSG_PROPERTIES
//       : {
//         MSG_JSON_RPC : {MSG_CONST : MSG_JSON_RPC_VERSION, MSG_TYPE : MSG_STRING},
//         MSG_METHOD : {MSG_TYPE : MSG_STRING},
//         MSG_PARAMS : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_PROPERTIES : {
//             MSG_META : {
//               MSG_ADDITIONAL_PROPERTIES : {},
//               MSG_DESCRIPTION : "This parameter name is reserved by MCP to allow "
//                               "clients and servers to attach additional "
//                               "metadata to their notifications.",
//               MSG_TYPE : MSG_OBJECT
//             }
//           },
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED : [ MSG_JSON_RPC, MSG_METHOD ],
//                      MSG_TYPE : MSG_OBJECT
// };

// JSONRPCBatchRequest {
//   MSG_DESCRIPTION : "A JSON-RPC batch request, as described in "
//                   "https://www.jsonrpc.org/specification#batch.",
//                   MSG_ITEMS : {
//                     "anyOf" : [
//                       {"$ref" : "#/definitions/JSONRPCRequest"},
//                       {"$ref" : "#/definitions/JSONRPCNotification"}
//                     ]
//                   },
//                             MSG_TYPE : MSG_ARRAY
// };

/**
 * A JSON-RPC batch request, as described in
 * https://www.jsonrpc.org/specification#batch.
 */
using BatchRequestMessage = vector<variant<RequestMessage, NotificationMessage>>;

// JSONRPCBatchResponse {
//   MSG_DESCRIPTION : "A JSON-RPC batch response, as described in "
//                   "https://www.jsonrpc.org/specification#batch.",
//                   MSG_ITEMS : {
//                     "anyOf" : [
//                       {"$ref" : "#/definitions/JSONRPCResponse"},
//                       {"$ref" : "#/definitions/JSONRPCError"}
//                     ]
//                   },
//                             MSG_TYPE : MSG_ARRAY
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
//       MSG_DESCRIPTION : "A JSON-RPC batch request, as described in "
//                       "https://www.jsonrpc.org/specification#batch.",
//       MSG_ITEMS : {
//         "anyOf" : [
//           {"$ref" : "#/definitions/JSONRPCRequest"},
//           {"$ref" : "#/definitions/JSONRPCNotification"}
//         ]
//       },
//       MSG_TYPE : MSG_ARRAY
//     },
//     {"$ref" : "#/definitions/JSONRPCResponse"},
//     {"$ref" : "#/definitions/JSONRPCError"}, {
//       MSG_DESCRIPTION : "A JSON-RPC batch response, as described in "
//                       "https://www.jsonrpc.org/specification#batch.",
//       MSG_ITEMS : {
//         "anyOf" : [
//           {"$ref" : "#/definitions/JSONRPCResponse"},
//           {"$ref" : "#/definitions/JSONRPCError"}
//         ]
//       },
//       MSG_TYPE : MSG_ARRAY
//     }
//   ],
//   MSG_DESCRIPTION : "Refers to any valid JSON-RPC object that can be "
//                   "decoded off the wire, or encoded to be sent."
// };

/**
 * Refers to any valid JSON-RPC object that can be decoded off the wire, or
 * encoded to be sent.
 */
using JSON_RPC_Message = variant<RequestMessage, NotificationMessage, ResponseMessage, ErrorMessage,
                                 BatchRequestMessage, BatchResponseMessage>;

MCP_NAMESPACE_END