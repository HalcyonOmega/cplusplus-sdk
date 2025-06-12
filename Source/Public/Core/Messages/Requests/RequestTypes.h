#pragma once

#include "Core.h"
#include "Core/Constants/MessageConstants.h"
#include "Core/Constants/MethodConstants.h"
#include "Core/Messages/Requests/Requests.h"
#include "Utilities/JSON/JSONLayer.hpp"

MCP_NAMESPACE_BEGIN

//! NOTE: @HalcyonOmega NON-JSON-RPC Types in JSON

/* Ping */

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

// A ping, issued by either the server or the client, to check that the other
// party is still alive. The receiver must promptly respond, or else may be
// disconnected.
struct PingRequest : public RequestMessage {
    PingRequest() : RequestMessage(MTHD_PING) {}
};

/* Pagination */

struct PaginatedRequestParams : public RequestParams {
    optional<Cursor>
        Cursor; // An opaque token representing the current pagination position. If
                // provided, the server should return results starting after this cursor.
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

struct PaginatedRequest : public RequestMessage {
    optional<PaginatedRequestParams> Params;
};

MCP_NAMESPACE_END