// struct Notification {
//   "properties" : {
//     "method" : {"type" : "string"},
//     "params" : {
//       "additionalProperties" : {},
//       "properties" : {
//         "_meta" : {
//           "additionalProperties" : {},
//           "description" :
//               "This parameter name is reserved by MCP to allow clients and "
//               "servers to attach additional metadata to their
//               notifications.",
//           "type" : "object"
//         }
//       },
//       "type" : "object"
//     }
//   },
//                  "required" : ["method"],
//                               "type" : "object"
// };

// struct ProgressNotification {
//   "description" : "An out-of-band notification used to inform the receiver "
//                   "of a progress update for a long-running request.",
//                   "properties"
//       : {
//         "method" : {"const" : "notifications/progress", "type" : "string"},
//         "params" : {
//           "properties" : {
//             "message" : {
//               "description" :
//                   "An optional message describing the current progress.",
//               "type" : "string"
//             },
//             "progress" : {
//               "description" :
//                   "The progress thus far. This should increase every time "
//                   "progress is made, even if the total is unknown.",
//               "type" : "number"
//             },
//             "progressToken" : {
//               "$ref" : "#/definitions/ProgressToken",
//               "description" :
//                   "The progress token which was given in the initial request,
//                   " "used to associate this notification with the request
//                   that " "is proceeding."
//             },
//             "total" : {
//               "description" : "Total number of items to process (or total "
//                               "progress required), if known.",
//               "type" : "number"
//             }
//           },
//           "required" : [ "progress", "progressToken" ],
//           "type" : "object"
//         }
//       },
//         "required" : [ "method", "params" ],
//                      "type" : "object"
// };

#pragma once

#include "Constants.h"
#include "Core.h"

MCP_NAMESPACE_BEGIN

struct Notification {
  string method;
  optional < {
    /**
     * This parameter name is reserved by MCP to allow clients and servers to
     * attach additional metadata to their notifications.
     */
    _meta ?: {[key:string] : unknown};
    [key:string] : unknown;
  }
  > params;
};

/* Cancellation */
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
  params : {
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

  CancelledNotification() { method = MTHD_NOTIFICATIONS_CANCELLED; }
};

/* Progress notifications */
/**
 * A progress token, used to associate progress notifications with the original
 * request.
 */
using ProgressToken = std::variant<std::string, int>; // TODO: Verify `int` is
                                                      // the right integer type

/**
 * An out-of-band notification used to inform the receiver of a progress update
 * for a long-running request.
 */
struct ProgressNotification : public Notification {

  ProgressNotification() {
    method = MTHD_NOTIFICATIONS_PROGRESS;
    params = {
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
  };
}
}
;

// struct CancelledNotification {
//   "description"
//       : "This notification can be sent by either side to indicate that it is
//       "
//         "cancelling a previously-issued request.\n\nThe request SHOULD still
//         " "be in-flight, but due to communication latency, it is always "
//         "possible that this notification MAY arrive after the request has "
//         "already finished.\n\nThis notification indicates that the result "
//         "will be unused, so any associated processing SHOULD cease.\n\nA "
//         "client MUST NOT attempt to cancel its `initialize` request.",
//         "properties"
//       : {
//         "method" : {"const" : "notifications/cancelled", "type" : "string"},
//         "params" : {
//           "properties" : {
//             "reason" : {
//               "description" :
//                   "An optional string describing the reason for the "
//                   "cancellation. This MAY be logged or presented to the
//                   user.",
//               "type" : "string"
//             },
//             "requestId" : {
//               "$ref" : "#/definitions/RequestId",
//               "description" :
//                   "The ID of the request to cancel.\n\nThis MUST correspond
//                   to " "the ID of a request previously issued in the same
//                   direction."
//             }
//           },
//           "required" : ["requestId"],
//           "type" : "object"
//         }
//       },
//         "required" : [ "method", "params" ],
//                      "type" : "object"
// };

MCP_NAMESPACE_END