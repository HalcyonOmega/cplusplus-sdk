#pragma once

#include "Constants.h"
#include "Core.h"

MCP_NAMESPACE_BEGIN

// RequestId {
//   "description" : "A uniquely identifying ID for a request in JSON-RPC.",
//                   "type" : [ "string", "integer" ]
// };

/**
 * A uniquely identifying ID for a request in JSON-RPC.
 */
using RequestId = variant<string, number>;

struct RequestParamsMeta {
    using ProgressToken = std::variant<std::string, int>;
    /**
     * If specified, the caller is requesting out-of-band progress
     * notifications for this request (as represented by
     * notifications/progress). The value of this parameter is an opaque
     * token that will be attached to any subsequent notifications. The
     * receiver is not obligated to provide these notifications.
     */
    optional<ProgressToken> progressToken;
};

struct RequestParams {
    optional<RequestParamsMeta> _meta;
};

// Request {
//   "properties" : {
//     "method" : {"type" : "string"},
//     "params" : {
//       "additionalProperties" : {},
//       "properties" : {
//         "_meta" : {
//           "properties" : {
//             "progressToken" : {
//               "$ref" : "#/definitions/ProgressToken",
//               "description" :
//                   "If specified, the caller is requesting out-of-band "
//                   "progress notifications for this request (as represented "
//                   "by notifications/progress). The value of this parameter "
//                   "is an opaque token that will be attached to any "
//                   "subsequent notifications. The receiver is not obligated "
//                   "to provide these notifications."
//             }
//           },
//           "type" : "object"
//         }
//       },
//       "type" : "object"
//     }
//   },
//                  "required" : ["method"],
//                               "type" : "object"
// };

struct Request {
    string method;
    optional<RequestParams> params;
};

// PingRequest {
//   "description" : "A ping, issued by either the server or the client, to "
//                   "check that the other party is still alive. The receiver "
//                   "must promptly respond, or else may be disconnected.",
//                   "properties"
//       : {
//         "method" : {"const" : "ping", "type" : "string"},
//         "params" : {
//           "additionalProperties" : {},
//           "properties" : {
//             "_meta" : {
//               "properties" : {
//                 "progressToken" : {
//                   "$ref" : "#/definitions/ProgressToken",
//                   "description" :
//                       "If specified, the caller is requesting out-of-band "
//                       "progress notifications for this request (as
//                       represented " "by notifications/progress). The value of
//                       this parameter " "is an opaque token that will be
//                       attached to any " "subsequent notifications. The
//                       receiver is not obligated " "to provide these
//                       notifications."
//                 }
//               },
//               "type" : "object"
//             }
//           },
//           "type" : "object"
//         }
//       },
//         "required" : ["method"],
//                      "type" : "object"
// };

/* Ping */
/**
 * A ping, issued by either the server or the client, to check that the other
 * party is still alive. The receiver must promptly respond, or else may be
 * disconnected.
 */
struct PingRequest : public Request {
    PingRequest() {
        method = MTHD_PING;
    }
};

/* Pagination */

struct PaginatedRequestParams : public RequestParams {
    /**
     * An opaque token representing the current pagination position.
     * If provided, the server should return results starting after this cursor.
     */
    optional<Cursor> cursor;
};

// PaginatedRequest {
//   "properties" : {
//     "method" : {"type" : "string"},
//     "params" : {
//       "properties" : {
//         "cursor" : {
//           "description" :
//               "An opaque token representing the current pagination "
//               "position.\nIf provided, the server should return results "
//               "starting after this cursor.",
//           "type" : "string"
//         }
//       },
//       "type" : "object"
//     }
//   },
//                  "required" : ["method"],
//                               "type" : "object"
// };

struct PaginatedRequest : public Request {
    optional<PaginatedRequestParams> params;
};

MCP_NAMESPACE_END