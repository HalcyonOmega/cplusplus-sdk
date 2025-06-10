/* Begin Direct Translation JSON RPC Types */
#pragma once

#include "Constants.h"
#include "Core.h"
#include "Core/Types/Notification.hpp"

MCP_NAMESPACE_BEGIN

// A notification which does not expect a response.
struct JSON_RPC_Notification : public Notification {
    string JSON_RPC = MSG_KEY_JSON_RPC_VERSION;
};

bool IsJSON_RPC_Notification(const JSON& value) {
    return value.is_object() && value.value(MSG_KEY_JSON_RPC, "") == MSG_KEY_JSON_RPC_VERSION
           && value.contains(MSG_KEY_METHOD) && !value.contains(MSG_KEY_ID);
}

// A response to a request that indicates an error occurred.
struct JSON_RPC_Error {
    string JSON_RPC = MSG_KEY_JSON_RPC_VERSION;
    RequestID ID;
    struct {
        ErrorCode Code; // The error type that occurred.
        string Message; // A short description of the error. The message SHOULD be limited to a
                        // concise single sentence.
        optional<std::any>
            Data; // Additional information about the error. The value of this member is defined by
                  // the sender (e.g. detailed error information, nested errors etc.).
    } Error;
};

bool IsJSON_RPC_Error(const JSON& value) {
    return value.is_object() && value.value(MSG_KEY_JSON_RPC, "") == MSG_KEY_JSON_RPC_VERSION
           && value.contains(MSG_KEY_ID) && value.contains(MSG_KEY_ERROR)
           && !value.contains(MSG_KEY_RESULT);
}

// A request that expects a response.
struct JSON_RPC_Request : public Request {
    string JSON_RPC = MSG_KEY_JSON_RPC_VERSION;
    RequestID ID;
};
bool IsJSON_RPC_Request(const JSON& value) {
    return value.is_object() && value.value(MSG_KEY_JSON_RPC, "") == MSG_KEY_JSON_RPC_VERSION
           && value.contains(MSG_KEY_ID) && value.contains(MSG_KEY_METHOD)
           && !value.contains(MSG_KEY_ERROR) && !value.contains(MSG_KEY_RESULT);
}

// A successful (non-error) response to a request.
struct JSON_RPC_Response {
    string JSON_RPC = MSG_KEY_JSON_RPC_VERSION;
    RequestID ID;
    Result Result;
};

bool IsJSON_RPC_Response(const JSON& value) {
    return value.is_object() && value.value(MSG_KEY_JSON_RPC, "") == MSG_KEY_JSON_RPC_VERSION
           && value.contains(MSG_KEY_ID) && value.contains(MSG_KEY_RESULT)
           && !value.contains(MSG_KEY_ERROR);
}

using JSON_RPC_Message =
    variant<JSON_RPC_Request, JSON_RPC_Notification, JSON_RPC_Response, JSON_RPC_Error>;

MCP_NAMESPACE_END
/* End Direct Translation JSON RPC Types */

/* Begin New JSON RPC Layer Types */
#include "Core.h"
#include "MCP_Error.h"
#include "NotificationSchemas.h"
#include "RequestSchemas.h"
#include "ResultSchemas.h"

MCP_NAMESPACE_BEGIN

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

/**
 * A request that expects a response.
 */
struct JSON_RPC_Request : public Request {
    string JSON_RPC = MSG_KEY_JSON_RPC_VERSION;
    RequestID ID;
};

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

/**
 * A notification which does not expect a response.
 */
struct JSON_RPC_Notification : public Notification {
    string JSON_RPC = MSG_KEY_JSON_RPC_VERSION;
};

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

/**
 * A successful (non-error) response to a request.
 */
struct JSON_RPC_Response {
    string JSON_RPC = MSG_KEY_JSON_RPC_VERSION;
    RequestID ID;
    Result Result;
};

/**
 * A response to a request that indicates an error occurred.
 */
struct JSON_RPC_Error {
    string JSON_RPC = MSG_KEY_JSON_RPC_VERSION;
    RequestID ID;
    MCP_Error Error;
};

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
using JSON_RPC_BatchRequest = vector<variant<JSON_RPC_Request, JSON_RPC_Notification>>;

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
using JSON_RPC_BatchResponse = vector<variant<JSON_RPC_Response, JSON_RPC_Error>>;

// JSONRPCMessage {
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
using JSON_RPC_Message = variant<JSON_RPC_Request, JSON_RPC_Notification, JSON_RPC_BatchRequest,
                               JSON_RPC_Response, JSON_RPC_Error, JSON_RPC_BatchResponse>;

MCP_NAMESPACE_END
/* End New JSON RPC Layer Types */