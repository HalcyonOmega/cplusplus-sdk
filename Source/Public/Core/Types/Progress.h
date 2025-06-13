#pragma once

#include "Core.h"
#include "MethodConstants.h"
#include "NotificationBase.h"

MCP_NAMESPACE_BEGIN

// ProgressToken {
//   MSG_DESCRIPTION : "A progress token, used to associate progress "
//                   "notifications with the original request.",
//                   MSG_TYPE : [ MSG_STRING, MSG_INTEGER ]
// };

// A progress token, used to associate progress notifications with the original request.
struct ProgressToken {
    variant<string, int> Value;
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

// An out-of-band notification used to inform the receiver of a progress update for a long-running
// request.
struct ProgressNotification : public NotificationBase {
    struct ProgressNotificationParams : public Notification::NotificationParams {
        ProgressToken
            ProgressToken; // The progress token which was given in the initial request, used to
                           // associate this notification with the request that is proceeding.
        double Progress; // The progress thus far. This should increase every time progress is made,
                         // even if the total is unknown.
        optional<int>
            Total; // Total number of items to process (or total progress required), if known.
        optional<string> Message; // An optional message describing the current progress.

        // TODO: @HalcyonOmega - Is this the best way to handle additional properties?
        ProgressNotificationParams() {
            AdditionalProperties = {{MSG_PROGRESS_TOKEN, ProgressToken},
                                    {MSG_PROGRESS, Progress},
                                    {MSG_TOTAL, Total},
                                    {MSG_MESSAGE, Message}};
        }
    };

    ProgressNotificationParams Params;

    ProgressNotification() : NotificationBase(MTHD_NOTIFICATIONS_PROGRESS) {}
};

MCP_NAMESPACE_END