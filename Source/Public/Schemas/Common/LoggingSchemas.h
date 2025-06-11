#pragma once

#include "CommonSchemas.h"
#include "Constants.h"
#include "Core.h"
#include "NotificationSchemas.h"

MCP_NAMESPACE_BEGIN

// LoggingLevel {
//   MSG_DESCRIPTION :
//       "The severity of a log message.\n\nThese map to syslog message "
//       "severities, as specified in "
//       "RFC-5424:\nhttps://datatracker.ietf.org/doc/html/rfc5424#section-6.2.1",
//   "enum" : [
//     "alert", "critical", "debug", "emergency", "error", "info", "notice",
//     "warning"
//   ],
//   MSG_TYPE : MSG_STRING
// };

/**
 * The severity of a log message.
 *
 * These map to syslog message severities, as specified in RFC-5424:
 * https://datatracker.ietf.org/doc/html/rfc5424#section-6.2.1
 */
enum class LoggingLevel { Debug, Info, Notice, Warning, Error, Critical, Alert, Emergency };

NLOHMANN_JSON_SERIALIZE_ENUM(LoggingLevel, {{LoggingLevel::Debug, "debug"},
                                            {LoggingLevel::Info, "info"},
                                            {LoggingLevel::Notice, "notice"},
                                            {LoggingLevel::Warning, "warning"},
                                            {LoggingLevel::Error, "error"},
                                            {LoggingLevel::Critical, "critical"},
                                            {LoggingLevel::Alert, "alert"},
                                            {LoggingLevel::Emergency, "emergency"}});

struct SetLevelRequestParams {
    /**
     * The level of logging that the client wants to receive from the server. The
     * server should send all logs at this level and higher (i.e., more severe) to
     * the client as notifications/message.
     */
    LoggingLevel level;
};

// SetLevelRequest {
//   MSG_DESCRIPTION
//       : "A request from the client to the server, to enable or adjust logging.",
//         MSG_PROPERTIES : {
//           MSG_METHOD : {MSG_CONST : MTHD_LOGGING_SET_LEVEL, MSG_TYPE : MSG_STRING},
//           MSG_PARAMS : {
//             MSG_PROPERTIES : {
//               "level" : {
//                 "$ref" : "#/definitions/LoggingLevel",
//                 MSG_DESCRIPTION :
//                     "The level of logging that the client wants to "
//                     "receive from the server. The server should send all "
//                     "logs at this level and higher (i.e., more severe) "
//                     "to the client as notifications/message."
//               }
//             },
//             MSG_REQUIRED : ["level"],
//             MSG_TYPE : MSG_OBJECT
//           }
//         },
//                        MSG_REQUIRED : [ MSG_METHOD, MSG_PARAMS ],
//                                     MSG_TYPE : MSG_OBJECT
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
//   MSG_DESCRIPTION
//       : "Notification of a log message passed from server to client. If no "
//         "logging/setLevel request has been sent from the client, the server "
//         "MAY decide which messages to send automatically.",
//         MSG_PROPERTIES : {
//           MSG_METHOD : {MSG_CONST : MTHD_NOTIFICATIONS_MESSAGE, MSG_TYPE : MSG_STRING},
//           MSG_PARAMS : {
//             MSG_PROPERTIES : {
//               MSG_DATA : {
//                 MSG_DESCRIPTION :
//                     "The data to be logged, such as a string message or an "
//                     "object. Any JSON serializable type is allowed here."
//               },
//               "level" : {
//                 "$ref" : "#/definitions/LoggingLevel",
//                 MSG_DESCRIPTION : "The severity of this log message."
//               },
//               "logger" : {
//                 MSG_DESCRIPTION :
//                     "An optional name of the logger issuing this message.",
//                 MSG_TYPE : MSG_STRING
//               }
//             },
//             MSG_REQUIRED : [ MSG_DATA, "level" ],
//             MSG_TYPE : MSG_OBJECT
//           }
//         },
//                        MSG_REQUIRED : [ MSG_METHOD, MSG_PARAMS ],
//                                     MSG_TYPE : MSG_OBJECT
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