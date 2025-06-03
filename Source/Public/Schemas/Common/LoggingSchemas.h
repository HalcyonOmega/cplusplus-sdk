#pragma once

#include "CommonSchemas.h"
#include "Constants.h"
#include "Core.h"
#include "NotificationSchemas.h"

MCP_NAMESPACE_BEGIN

// LoggingLevel {
//   "description" :
//       "The severity of a log message.\n\nThese map to syslog message "
//       "severities, as specified in "
//       "RFC-5424:\nhttps://datatracker.ietf.org/doc/html/rfc5424#section-6.2.1",
//   "enum" : [
//     "alert", "critical", "debug", "emergency", "error", "info", "notice",
//     "warning"
//   ],
//   "type" : "string"
// };

/**
 * The severity of a log message.
 *
 * These map to syslog message severities, as specified in RFC-5424:
 * https://datatracker.ietf.org/doc/html/rfc5424#section-6.2.1
 */
enum class LoggingLevel { debug, info, notice, warning, error, critical, alert, emergency };

struct SetLevelRequestParams {
    /**
     * The level of logging that the client wants to receive from the server. The
     * server should send all logs at this level and higher (i.e., more severe) to
     * the client as notifications/message.
     */
    LoggingLevel level;
};

// SetLevelRequest {
//   "description"
//       : "A request from the client to the server, to enable or adjust logging.",
//         "properties" : {
//           "method" : {"const" : "logging/setLevel", "type" : "string"},
//           "params" : {
//             "properties" : {
//               "level" : {
//                 "$ref" : "#/definitions/LoggingLevel",
//                 "description" :
//                     "The level of logging that the client wants to "
//                     "receive from the server. The server should send all "
//                     "logs at this level and higher (i.e., more severe) "
//                     "to the client as notifications/message."
//               }
//             },
//             "required" : ["level"],
//             "type" : "object"
//           }
//         },
//                        "required" : [ "method", "params" ],
//                                     "type" : "object"
// };
/* Logging */
/**
 * A request from the client to the server, to enable or adjust logging.
 */

struct SetLevelRequest : public Request {
    SetLevelRequestParams params;

    SetLevelRequest() {
        method = MTHD_LOGGING_SET_LEVEL;
    }
};

struct LoggingMessageNotificationParams {
    /**
     * The severity of this log message.
     */
    LoggingLevel level;
    /**
     * An optional name of the logger issuing this message.
     */
    optional<string> logger;
    /**
     * The data to be logged, such as a string message or an object. Any JSON
     * serializable type is allowed here.
     */
    any data;
};

// LoggingMessageNotification {
//   "description"
//       : "Notification of a log message passed from server to client. If no "
//         "logging/setLevel request has been sent from the client, the server "
//         "MAY decide which messages to send automatically.",
//         "properties" : {
//           "method" : {"const" : "notifications/message", "type" : "string"},
//           "params" : {
//             "properties" : {
//               "data" : {
//                 "description" :
//                     "The data to be logged, such as a string message or an "
//                     "object. Any JSON serializable type is allowed here."
//               },
//               "level" : {
//                 "$ref" : "#/definitions/LoggingLevel",
//                 "description" : "The severity of this log message."
//               },
//               "logger" : {
//                 "description" :
//                     "An optional name of the logger issuing this message.",
//                 "type" : "string"
//               }
//             },
//             "required" : [ "data", "level" ],
//             "type" : "object"
//           }
//         },
//                        "required" : [ "method", "params" ],
//                                     "type" : "object"
// };

/**
 * Notification of a log message passed from server to client. If no
 * logging/setLevel request has been sent from the client, the server MAY decide
 * which messages to send automatically.
 */
struct LoggingMessageNotification : public Notification {
    LoggingMessageNotificationParams params;

    LoggingMessageNotification() {
        method = MTHD_NOTIFICATIONS_MESSAGE;
    }
};

MCP_NAMESPACE_END