#pragma once

#include "Core.h"
#include "Core/Messages/Errors/Errors.h"
#include "Core/Messages/Notifications/Notifications.h"
#include "Core/Messages/Requests/Requests.h"
#include "Core/Messages/Responses/Responses.h"

MCP_NAMESPACE_BEGIN

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

// BatchRequestMessage {
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
struct BatchRequestMessage {
    vector<variant<RequestMessage, NotificationMessage>> Items;
};

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
struct BatchResponseMessage {
    vector<variant<ResponseMessage, ErrorMessage>> Items;
};

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