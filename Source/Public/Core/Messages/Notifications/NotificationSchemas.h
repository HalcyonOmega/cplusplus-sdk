#pragma once

#include "Constants.h"
#include "Core.h"
#include "RequestSchemas.h"

MCP_NAMESPACE_BEGIN

struct NotificationParamsMeta {
    unordered_map<string, any> additionalProperties;
};

struct NotificationParams {
    /**
     * This parameter name is reserved by MCP to allow clients and servers to
     * attach additional metadata to their notifications.
     */
    optional<NotificationParamsMeta> _meta;
    unordered_map<string, any> additionalProperties;
};

// Notification {
//   MSG_PROPERTIES : {
//     MSG_METHOD : {MSG_TYPE : MSG_STRING},
//     MSG_PARAMS : {
//       MSG_ADDITIONAL_PROPERTIES : {},
//       MSG_PROPERTIES : {
//         MSG_META : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_DESCRIPTION :
//               "This parameter name is reserved by MCP to allow clients and "
//               "servers to attach additional metadata to their
//               notifications.",
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//       MSG_TYPE : MSG_OBJECT
//     }
//   },
//                  MSG_REQUIRED : [MSG_METHOD],
//                               MSG_TYPE : MSG_OBJECT
// };

struct Notification {
    string method;
    optional<NotificationParams> params;
};

/* Cancellation */
struct CancelledNotificationParams {
    /**
     * The ID of the request to cancel.
     *
     * This MUST correspond to the ID of a request previously issued in the same
     * direction.
     */
    RequestId requestId;

    /**
     * An optional string describing the reason for the cancellation. This MAY
     * be logged or presented to the user.
     */
    optional<string> reason;
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
//             "requestId" : {
//               "$ref" : "#/definitions/RequestId",
//               MSG_DESCRIPTION :
//                   "The ID of the request to cancel.\n\nThis MUST correspond
//                   to " "the ID of a request previously issued in the same
//                   direction."
//             }
//           },
//           MSG_REQUIRED : ["requestId"],
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
struct CancelledNotification : public Notification {
    CancelledNotificationParams params;

    CancelledNotification() {
        method = MTHD_NOTIFICATIONS_CANCELLED;
    }
};

/* Progress notifications */
// ProgressToken {
//   MSG_DESCRIPTION : "A progress token, used to associate progress "
//                   "notifications with the original request.",
//                   MSG_TYPE : [ MSG_STRING, MSG_INTEGER ]
// };

/**
 * A progress token, used to associate progress notifications with the original
 * request.
 */
using ProgressToken = std::variant<std::string, int>; // TODO: Verify `int` is
                                                      // the right integer type

struct ProgressNotificationParams : public NotificationParams {
    /**
     * The progress token which was given in the initial request, used to
     * associate this notification with the request that is proceeding.
     */
    ProgressToken progressToken;
    /**
     * The progress thus far. This should increase every time progress is made,
     * even if the total is unknown.
     *
     * @TJS-type number
     */
    number progress;
    /**
     * Total number of items to process (or total progress required), if known.
     *
     * @TJS-type number
     */
    optional<number> total;
    /**
     * An optional message describing the current progress.
     */
    optional<string> message;

    ProgressNotificationParams() {
        additionalProperties = {{MSG_PROGRESS_TOKEN, progressToken},
                                {MSG_PROGRESS, progress},
                                {"total", total},
                                {MSG_MESSAGE, message}};
    }
};

// ProgressNotification {
//   MSG_DESCRIPTION : "An out-of-band notification used to inform the receiver "
//                   "of a progress update for a long-running request.",
//                   MSG_PROPERTIES
//       : {
//         MSG_METHOD : {MSG_CONST : MTHD_NOTIFICATIONS_PROGRESS, MSG_TYPE : MSG_STRING},
//         MSG_PARAMS : {
//           MSG_PROPERTIES : {
//             MSG_MESSAGE : {
//               MSG_DESCRIPTION :
//                   "An optional message describing the current progress.",
//               MSG_TYPE : MSG_STRING
//             },
//             MSG_PROGRESS : {
//               MSG_DESCRIPTION :
//                   "The progress thus far. This should increase every time "
//                   "progress is made, even if the total is unknown.",
//               MSG_TYPE : MSG_NUMBER
//             },
//             MSG_PROGRESS_TOKEN : {
//               "$ref" : "#/definitions/ProgressToken",
//               MSG_DESCRIPTION :
//                   "The progress token which was given in the initial request,
//                   " "used to associate this notification with the request
//                   that " "is proceeding."
//             },
//             "total" : {
//               MSG_DESCRIPTION : "Total number of items to process (or total "
//                               "progress required), if known.",
//               MSG_TYPE : MSG_NUMBER
//             }
//           },
//           MSG_REQUIRED : [ MSG_PROGRESS, MSG_PROGRESS_TOKEN ],
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED : [ MSG_METHOD, MSG_PARAMS ],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * An out-of-band notification used to inform the receiver of a progress update
 * for a long-running request.
 */
struct ProgressNotification : public Notification {
    ProgressNotificationParams params;

    ProgressNotification() {
        method = MTHD_NOTIFICATIONS_PROGRESS;
    }
};

MCP_NAMESPACE_END