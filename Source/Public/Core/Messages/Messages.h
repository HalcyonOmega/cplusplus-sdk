#pragma once

#include "Core.h"
#include "ErrorBase.h"
#include "NotificationBase.h"
#include "RequestBase.h"
#include "ResponseBase.h"

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

// A JSON-RPC batch request, as described inhttps://www.jsonrpc.org/specification#batch.
struct BatchRequestBase {
    vector<variant<RequestBase, NotificationBase>> Items;
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
    vector<variant<ResponseBase, ErrorBase>> Items;
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

// Refers to any valid JSON-RPC object that can be decoded off the wire, orencoded to be sent.
class JSONRPC_Message {
  public:
    variant<RequestBase, NotificationBase, ResponseBase, ErrorBase, BatchRequestBase,
            BatchResponseBase>
        Message;
};

MCP_NAMESPACE_END