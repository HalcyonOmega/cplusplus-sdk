#pragma once

#include "Core.h"
#include "Core/Messages/Notifications/NotificationBase.h"
#include "Core/Messages/Requests/RequestBase.h"

MCP_NAMESPACE_BEGIN

struct CancelledNotificationParams {
    RequestID RequestID;     // The ID of the request to cancel. This MUST correspond to the ID of a
                             // request previously issued in the same direction.
    optional<string> Reason; // An optional string describing the reason for the cancellation.
                             // This MAY be logged or presented to the user.
};

// CancelledNotification {
//   MSG_DESCRIPTION
//       : "This notification can be sent by either side to indicate that it is
//       "
//         "cancelling a previously-issued request.\n\nThe request SHOULD still
//         " "be in-flight, but due to communication latency, it is always "
//         "possible that this notification MAY arrive after the request has "
//         "already finished.\n\nThis notification indicates that the result "
//         "will be unused, so any associated processing SHOULD cease.\n\nA "
//         "client MUST NOT attempt to cancel its `initialize` request.",
//         MSG_PROPERTIES
//       : {
//         MSG_METHOD : {MSG_CONST : MTHD_NOTIFICATIONS_CANCELLED, MSG_TYPE : MSG_STRING},
//         MSG_PARAMS : {
//           MSG_PROPERTIES : {
//             "reason" : {
//               MSG_DESCRIPTION :
//                   "An optional string describing the reason for the "
//                   "cancellation. This MAY be logged or presented to the
//                   user.",
//               MSG_TYPE : MSG_STRING
//             },
//             MSG_REQUEST_ID : {
//               "$ref" : "#/definitions/RequestID",
//               MSG_DESCRIPTION :
//                   "The ID of the request to cancel.\n\nThis MUST correspond
//                   to " "the ID of a request previously issued in the same
//                   direction."
//             }
//           },
//           MSG_REQUIRED : [MSG_REQUEST_ID],
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED : [ MSG_METHOD, MSG_PARAMS ],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * This notification can be sent by either side to indicate that it is
 * cancelling a previously-issued request.
 *
 * The request SHOULD still be in-flight, but due to communication latency, it
 * is always possible that this notification MAY arrive after the request has
 * already finished.
 *
 * This notification indicates that the result will be unused, so any associated
 * processing SHOULD cease.
 *
 * A client MUST NOT attempt to cancel its `initialize` request.
 */
struct CancelledNotification : public NotificationBase {
    CancelledNotificationParams Params;

    CancelledNotification() : NotificationBase(MTHD_NOTIFICATIONS_CANCELLED) {}
};

MCP_NAMESPACE_END