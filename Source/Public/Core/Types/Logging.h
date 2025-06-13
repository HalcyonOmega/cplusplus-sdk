#pragma once

#include "Core.h"
#include "Core/Constants/MethodConstants.h"
#include "Core/Messages/Messages.h"
#include "Core/Messages/Notifications/NotificationBase.h"
#include "Core/Messages/Requests/RequestBase.h"
#include "Core/Messages/Responses/ResponseBase.h"
#include "Utilities/JSON/JSONLayer.hpp"

MCP_NAMESPACE_BEGIN

static constexpr const char* LOG_DEBUG = "debug";
static constexpr const char* LOG_INFO = "info";
static constexpr const char* LOG_NOTICE = "notice";
static constexpr const char* LOG_WARNING = "warning";
static constexpr const char* LOG_ERROR = "error";
static constexpr const char* LOG_CRITICAL = "critical";
static constexpr const char* LOG_ALERT = "alert";
static constexpr const char* LOG_EMERGENCY = "emergency";

// LoggingLevel {
//   MSG_DESCRIPTION :
//       "The severity of a log message.\n\nThese map to syslog message "
//       "severities, as specified in "
//       "RFC-5424:\nhttps://datatracker.ietf.org/doc/html/rfc5424#section-6.2.1",
//   MSG_ENUM : [
//     LOG_ALERT, LOG_CRITICAL, LOG_DEBUG, LOG_EMERGENCY, LOG_ERROR, LOG_INFO, LOG_NOTICE,
//     LOG_WARNING
//   ],
//   MSG_TYPE : MSG_STRING
// };

// The severity of a log message. These map to syslog message severities, as specified in RFC-5424:
// https://datatracker.ietf.org/doc/html/rfc5424#section-6.2.1
enum class LoggingLevel { Debug, Info, Notice, Warning, Error, Critical, Alert, Emergency };

DEFINE_ENUM_JSON(LoggingLevel, {{LoggingLevel::Debug, LOG_DEBUG},
                                {LoggingLevel::Info, LOG_INFO},
                                {LoggingLevel::Notice, LOG_NOTICE},
                                {LoggingLevel::Warning, LOG_WARNING},
                                {LoggingLevel::Error, LOG_ERROR},
                                {LoggingLevel::Critical, LOG_CRITICAL},
                                {LoggingLevel::Alert, LOG_ALERT},
                                {LoggingLevel::Emergency, LOG_EMERGENCY}});

// SetLevelRequest {
//   MSG_DESCRIPTION
//       : "A request from the client to the server, to enable or adjust logging.",
//         MSG_PROPERTIES : {
//           MSG_METHOD : {MSG_CONST : MTHD_LOGGING_SET_LEVEL, MSG_TYPE : MSG_STRING},
//           MSG_PARAMS : {
//             MSG_PROPERTIES : {
//               MSG_LEVEL : {
//                 "$ref" : "#/definitions/LoggingLevel",
//                 MSG_DESCRIPTION :
//                     "The level of logging that the client wants to "
//                     "receive from the server. The server should send all "
//                     "logs at this level and higher (i.e., more severe) "
//                     "to the client as notifications/message."
//               }
//             },
//             MSG_REQUIRED : [MSG_LEVEL],
//             MSG_TYPE : MSG_OBJECT
//           }
//         },
//                        MSG_REQUIRED : [ MSG_METHOD, MSG_PARAMS ],
//                                     MSG_TYPE : MSG_OBJECT
// };

// A request from the client to the server, to enable or adjust logging.
struct SetLevelRequest : public RequestBase {
    struct SetLevelRequestParams {
        LoggingLevel Level; // The level of logging that the client wants to receive from the
                            // server. The server should send all logs at this level and higher
                            // (i.e., more severe) to the client as notifications/message.
    };

    SetLevelRequestParams Params;

    SetLevelRequest() : RequestBase(MTHD_LOGGING_SET_LEVEL) {}
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
//               MSG_LEVEL : {
//                 "$ref" : "#/definitions/LoggingLevel",
//                 MSG_DESCRIPTION : "The severity of this log message."
//               },
//               MSG_LOGGER : {
//                 MSG_DESCRIPTION :
//                     "An optional name of the logger issuing this message.",
//                 MSG_TYPE : MSG_STRING
//               }
//             },
//             MSG_REQUIRED : [ MSG_DATA, MSG_LEVEL ],
//             MSG_TYPE : MSG_OBJECT
//           }
//         },
//                        MSG_REQUIRED : [ MSG_METHOD, MSG_PARAMS ],
//                                     MSG_TYPE : MSG_OBJECT
// };

// Notification of a log message passed from server to client. If no logging/setLevel request has
// been sent from the client, the server MAY decide which messages to send automatically.
struct LoggingMessageNotification : public NotificationBase {
    struct LoggingMessageNotificationParams {
        LoggingLevel Level;      // The severity of this log message.
        optional<string> Logger; // An optional name of the logger issuing this message.
        JSON Data; // The data to be logged, such as a string message or an object. Any
                   // JSON serializable type is allowed here.
    };

    LoggingMessageNotificationParams Params;

    LoggingMessageNotification() : NotificationBase(MTHD_NOTIFICATIONS_MESSAGE) {}
};

MCP_NAMESPACE_END