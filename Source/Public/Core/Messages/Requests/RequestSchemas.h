#pragma once

#include "Core.h"
#include "Core/Constants/MessageConstants.h"
#include "Utilities/JSON/JSONLayer.hpp"

MCP_NAMESPACE_BEGIN

// NOTE: @HalcyonOmega NON-JSON-RPC Types

using ProgressToken = variant<string, int>;

struct RequestParamsMeta {
    // If specified, the caller is requesting out-of-band progress notifications for this request
    // (as represented by notifications/progress). The value of this parameter is an opaque token
    // that will be attached to any subsequent notifications. The receiver is not obligated to
    // provide these notifications.
    optional<ProgressToken> ProgressToken;
};

struct RequestParams {
    optional<RequestParamsMeta> Meta;
    DEFINE_TYPE_JSON(RequestParams, JKEY(Meta, MSG_META))
};

// Request {
//   MSG_PROPERTIES : {
//     MSG_METHOD : {MSG_TYPE : MSG_STRING},
//     MSG_PARAMS : {
//       MSG_ADDITIONAL_PROPERTIES : {},
//       MSG_PROPERTIES : {
//         MSG_META : {
//           MSG_PROPERTIES : {
//             MSG_PROGRESS_TOKEN : {
//               "$ref" : "#/definitions/ProgressToken",
//               MSG_DESCRIPTION :
//                   "If specified, the caller is requesting out-of-band "
//                   "progress notifications for this request (as represented "
//                   "by notifications/progress). The value of this parameter "
//                   "is an opaque token that will be attached to any "
//                   "subsequent notifications. The receiver is not obligated "
//                   "to provide these notifications."
//             }
//           },
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//       MSG_TYPE : MSG_OBJECT
//     }
//   },
//                  MSG_REQUIRED : [MSG_METHOD],
//                               MSG_TYPE : MSG_OBJECT
// };

struct Request {
    string method;
    optional<RequestParams> params;
};

// PingRequest {
//   MSG_DESCRIPTION : "A ping, issued by either the server or the client, to "
//                   "check that the other party is still alive. The receiver "
//                   "must promptly respond, or else may be disconnected.",
//                   MSG_PROPERTIES
//       : {
//         MSG_METHOD : {MSG_CONST : MTHD_PING, MSG_TYPE : MSG_STRING},
//         MSG_PARAMS : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_PROPERTIES : {
//             MSG_META : {
//               MSG_PROPERTIES : {
//                 MSG_PROGRESS_TOKEN : {
//                   "$ref" : "#/definitions/ProgressToken",
//                   MSG_DESCRIPTION :
//                       "If specified, the caller is requesting out-of-band "
//                       "progress notifications for this request (as
//                       represented " "by notifications/progress). The value of
//                       this parameter " "is an opaque token that will be
//                       attached to any " "subsequent notifications. The
//                       receiver is not obligated " "to provide these
//                       notifications."
//                 }
//               },
//               MSG_TYPE : MSG_OBJECT
//             }
//           },
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED : [MSG_METHOD],
//                      MSG_TYPE : MSG_OBJECT
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
//   MSG_PROPERTIES : {
//     MSG_METHOD : {MSG_TYPE : MSG_STRING},
//     MSG_PARAMS : {
//       MSG_PROPERTIES : {
//         MSG_CURSOR : {
//           MSG_DESCRIPTION :
//               "An opaque token representing the current pagination "
//               "position.\nIf provided, the server should return results "
//               "starting after this cursor.",
//           MSG_TYPE : MSG_STRING
//         }
//       },
//       MSG_TYPE : MSG_OBJECT
//     }
//   },
//                  MSG_REQUIRED : [MSG_METHOD],
//                               MSG_TYPE : MSG_OBJECT
// };

struct PaginatedRequest : public Request {
    optional<PaginatedRequestParams> params;
};

MCP_NAMESPACE_END