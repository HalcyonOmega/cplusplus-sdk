// JSON-RPC Schemas
#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

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

// JSONRPCError {
//   "description" : "A response to a request that indicates an error occurred.",
//                   "properties"
//       : {
//         "error" : {
//           "properties" : {
//             "code" : {
//               "description" : "The error type that occurred.",
//               "type" : "integer"
//             },
//             "data" : {
//               "description" :
//                   "Additional information about the error. The value of this "
//                   "member is defined by the sender (e.g. detailed error "
//                   "information, nested errors etc.)."
//             },
//             "message" : {
//               "description" : "A short description of the error. The message "
//                               "SHOULD be limited to a concise single sentence.",
//               "type" : "string"
//             }
//           },
//           "required" : [ "code", "message" ],
//           "type" : "object"
//         },
//         "id" : {"$ref" : "#/definitions/RequestId"},
//         "jsonrpc" : {"const" : "2.0", "type" : "string"}
//       },
//         "required" : [ "error", "id", "jsonrpc" ],
//                      "type" : "object"
// };

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

/* JSON-RPC types */

/**
 * Refers to any valid JSON-RPC object that can be decoded off the wire, or
 * encoded to be sent.
 */
export type JSONRPCMessage = | JSONRPCRequest | JSONRPCNotification | JSONRPCBatchRequest
                             | JSONRPCResponse | JSONRPCError | JSONRPCBatchResponse;

/**
 * A JSON-RPC batch request, as described in
 * https://www.jsonrpc.org/specification#batch.
 */
export type JSONRPCBatchRequest = (JSONRPCRequest | JSONRPCNotification)[];

/**
 * A JSON-RPC batch response, as described in
 * https://www.jsonrpc.org/specification#batch.
 */
export type JSONRPCBatchResponse = (JSONRPCResponse | JSONRPCError)[];

/**
 * A request that expects a response.
 */
export interface JSONRPCRequest extends Request {
jsonrpc:
    typeof JSONRPC_VERSION;
id:
    RequestId;
}

/**
 * A notification which does not expect a response.
 */
export interface JSONRPCNotification extends Notification {
jsonrpc:
    typeof JSONRPC_VERSION;
}

/**
 * A successful (non-error) response to a request.
 */
export interface JSONRPCResponse {
jsonrpc:
    typeof JSONRPC_VERSION;
id:
    RequestId;
result:
    Result;
}

// Standard JSON-RPC error codes
export const PARSE_ERROR = -32700;
export const INVALID_REQUEST = -32600;
export const METHOD_NOT_FOUND = -32601;
export const INVALID_PARAMS = -32602;
export const INTERNAL_ERROR = -32603;

/**
 * A response to a request that indicates an error occurred.
 */
export interface JSONRPCError {
jsonrpc:
    typeof JSONRPC_VERSION;
id:
    RequestId;
error: {
/**
 * The error type that occurred.
 */
code:
    number;
/**
 * A short description of the error. The message SHOULD be limited to a concise single sentence.
 */
message:
    string;
    /**
     * Additional information about the error. The value of this member is defined by the sender
     * (e.g. detailed error information, nested errors etc.).
     */
    data ?: unknown;
};
}
MCP_NAMESPACE_END