#pragma once

#include <utility>

#include "Core.h"
#include "Core/Constants/ErrorConstants.h"
#include "Core/Constants/MessageConstants.h"

MCP_NAMESPACE_BEGIN

struct MessageID {
  private:
    // TODO: Is LongLong the right type or should it be double?
    variant<string, int, long long> m_MessageID;

  public:
    [[nodiscard]] string_view ToString() const;

    MessageID(string StringID) : m_MessageID(std::move(StringID)) {}
    MessageID(int IntID) : m_MessageID(IntID) {}
    MessageID(long long LongLongID) : m_MessageID(LongLongID) {}
};

struct MessageParams {
    [[nodiscard]] virtual string Serialize() const;
    virtual MessageParams Deserialize(string InString) = 0;
};

class MessageBase {
  private:
    string m_JSONRPC = MSG_JSON_RPC_VERSION;

  public:
    [[nodiscard]] string_view GetJSONRPCVersion() const;

    virtual ~MessageBase() = default;

    // Helpers
    [[nodiscard]] virtual JSON ToJSON() const = 0;
    [[nodiscard]] virtual MessageBase FromJSON(JSON InJSON) = 0;
    [[nodiscard]] virtual string Serialize() const;
    [[nodiscard]] virtual MessageBase Deserialize(string InString) = 0;
};

// A request that expects a response.
class RequestMessage : public MessageBase {
  private:
    MessageID m_ID;
    string m_Method;
    optional<unique_ptr<MessageParams>> m_Params = nullopt;

  public: // Direct Getters
    [[nodiscard]] MessageID GetMessageID() const;
    [[nodiscard]] string_view GetMethod() const;
    [[nodiscard]] optional<MessageParams> GetParams() const;

    // MessageBase Overrides
    [[nodiscard]] JSON ToJSON() const override;
    [[nodiscard]] MessageBase FromJSON(JSON InJSON) override;
    [[nodiscard]] string Serialize() const override;
    [[nodiscard]] MessageBase Deserialize(string InString) override;
};

// A successful (non-error) response to a request.
class ResponseMessage : public MessageBase {
  private:
    MessageID m_ID;
    unique_ptr<MessageParams> m_Result;

  public:
    [[nodiscard]] MessageID GetMessageID() const;
    [[nodiscard]] MessageParams& GetResult() const;

    // MessageBase Overrides
    [[nodiscard]] JSON ToJSON() const override;
    [[nodiscard]] MessageBase FromJSON(JSON InJSON) override;
    [[nodiscard]] string Serialize() const override;
    [[nodiscard]] MessageBase Deserialize(string InString) override;
};

// A notification which does not expect a response.
class NotificationMessage : public MessageBase {
  private:
    string m_Method;
    optional<unique_ptr<MessageParams>> m_Params = nullopt;

  public:
    [[nodiscard]] string_view GetMethod() const;
    [[nodiscard]] optional<MessageParams> GetParams() const;

    // MessageBase Overrides
    [[nodiscard]] JSON ToJSON() const override;
    [[nodiscard]] MessageBase FromJSON(JSON InJSON) override;
    [[nodiscard]] string Serialize() const override;
    [[nodiscard]] MessageBase Deserialize(string InString) override;
};

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
    MessageID m_ID;
    ErrorParams m_Error;

  public:
    // Direct Getters
    [[nodiscard]] MessageID GetID() const;
    [[nodiscard]] ErrorParams GetError() const;

    // MessageBase Overrides
    [[nodiscard]] JSON ToJSON() const override;
    [[nodiscard]] MessageBase FromJSON(JSON InJSON) override;
    [[nodiscard]] string Serialize() const override;
    [[nodiscard]] MessageBase Deserialize(string InString) override;

    // TODO: Check MessageID default IntID = 0 - should this be another default?
    ErrorMessage(Errors Code, string Message, optional<any> Data = nullopt)
        : m_ID(0), m_Error({.Code = Code, .Message = Message, .Data = Data}) {}

    ErrorMessage(MessageID MessageID, Errors Code, string Message)
        : m_ID(std::move(MessageID)), m_Error({.Code = Code, .Message = Message, .Data = nullopt}) {
    }

    ErrorMessage(MessageID MessageID, Errors Code, string Message, optional<any> Data = nullopt)
        : m_ID(std::move(MessageID)), m_Error({.Code = Code, .Message = Message, .Data = Data}) {}

    ErrorMessage(MessageID MessageID, ErrorParams Error)
        : m_ID(std::move(MessageID)), m_Error(std::move(Error)) {}
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