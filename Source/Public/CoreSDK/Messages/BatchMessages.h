#pragma once

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Messages/ErrorResponseBase.h"
#include "CoreSDK/Messages/MCPMessages.h"

MCP_NAMESPACE_BEGIN

// BatchRequestBase {
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
 * A JSON-RPC batch request, as described inhttps://www.jsonrpc.org/specification#batch.
 */
struct BatchRequestBase {
    std::vector<std::variant<RequestBase, NotificationBase>> Requests;
};

// BatchResponseBase {
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

// A JSON-RPC batch response, as described in https://www.jsonrpc.org/specification#batch.
struct BatchResponseBase {
    std::vector<std::variant<ResponseBase, ErrorResponseBase>> Responses;
};

// JSONRPC_Message {
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
 * Refers to any valid JSON-RPC object that can be decoded off the wire, or encoded to be sent.
 */
struct JSONRPC_Message {
    std::variant<RequestBase, NotificationBase, BatchRequestBase, BatchResponseBase, ResponseBase,
                 ErrorResponseBase>
        Message;
};

MCP_NAMESPACE_END
