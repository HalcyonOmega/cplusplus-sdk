#pragma once

#include <optional>
#include <string>
#include <utility>
#include <variant>

#include "CoreSDK/Common/Capabilities.h"
#include "CoreSDK/Common/Content.h"
#include "CoreSDK/Common/Implementation.h"
#include "CoreSDK/Common/Logging.h"
#include "CoreSDK/Common/Roles.h"
#include "CoreSDK/Features/PromptBase.h"
#include "CoreSDK/Features/ResourceBase.h"
#include "CoreSDK/Features/RootBase.h"
#include "CoreSDK/Features/SamplingBase.h"
#include "CoreSDK/Features/ToolBase.h"
#include "CoreSDK/Messages/NotificationBase.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "CoreSDK/Messages/ResponseBase.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

// InitializeRequest {
//   MSG_DESCRIPTION : "This request is sent from the client to the server when it "
//                   "first connects, asking it to begin initialization.",
//                   MSG_PROPERTIES
//       : {
//         MSG_METHOD : {MSG_CONST : MTHD_INITIALIZE, MSG_TYPE : MSG_STRING},
//         MSG_PARAMS : {
//           MSG_PROPERTIES : {
//             MSG_CAPABILITIES : {"$ref" : "#/definitions/ClientCapabilities"},
//             MSG_CLIENT_INFO : {"$ref" : "#/definitions/Implementation"},
//             MSG_PROTOCOL_VERSION : {
//               MSG_DESCRIPTION :
//                   "The latest version of the Model Context Protocol "
//                   "that the client supports. The client MAY decide to "
//                   "support older versions as well.",
//               MSG_TYPE : MSG_STRING
//             }
//           },
//           MSG_REQUIRED : [ MSG_CAPABILITIES, MSG_CLIENT_INFO, MSG_PROTOCOL_VERSION ],
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED : [ MSG_METHOD, MSG_PARAMS ],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * This request is sent from the client to the server when it first connects, asking it to begin
 * initialization.
 */
struct InitializeRequest : RequestBase {
    struct Params : RequestParams {
        std::string ProtocolVersion; // The latest version of the Model Context Protocol that the
                                     // client supports. The client MAY decide to support older
                                     // versions as well.
        ClientCapabilities Capabilities; // The capabilities of the client.
        Implementation ClientInfo;       // The implementation of the client.

        JKEY(PROTOCOLVERSIONKEY, ProtocolVersion, "protocolVersion")
        JKEY(CAPABILITIESKEY, Capabilities, "capabilities")
        JKEY(CLIENTINFOKEY, ClientInfo, "clientInfo")

        DEFINE_TYPE_JSON_DERIVED(InitializeRequest::Params, RequestParams, PROTOCOLVERSIONKEY,
                                 CAPABILITIESKEY, CLIENTINFOKEY)
    };

    InitializeRequest(const InitializeRequest::Params& InParams = InitializeRequest::Params{})
        : RequestBase("initialize", InParams) {}
};

// InitializeResult {
//   MSG_DESCRIPTION : "After receiving an initialize request from the client, "
//                   "the server sends this response.",
//                   MSG_PROPERTIES
//       : {
//         MSG_META : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_DESCRIPTION : "This result property is reserved by the protocol to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           MSG_TYPE : MSG_OBJECT
//         },
//         MSG_CAPABILITIES : {"$ref" : "#/definitions/ServerCapabilities"},
//         MSG_INSTRUCTIONS : {
//           MSG_DESCRIPTION :
//               "Instructions describing how to use the server and its "
//               "features.\n\nThis can be used by clients to improve the LLM's "
//               "understanding of available tools, resources, etc. It can be "
//               "thought of like a \"hint\" to the model. For example, this "
//               "information MAY be added to the system prompt.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_PROTOCOL_VERSION : {
//           MSG_DESCRIPTION : "The version of the Model Context Protocol that the "
//                           "server wants to use. This may not match the version "
//                           "that the client requested. If the client cannot "
//                           "support this version, it MUST disconnect.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_SERVER_INFO : {"$ref" : "#/definitions/Implementation"}
//       },
//         MSG_REQUIRED : [ MSG_CAPABILITIES, MSG_PROTOCOL_VERSION, MSG_SERVER_INFO ],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * After receiving an initialize request from the client, the server sends this response.
 */
struct InitializeResponse : ResponseBase {
    struct Result : ResultParams {
        std::string ProtocolVersion; // The version of the Model Context Protocol that the server
                                     // wants to use. This may not match the version that the client
                                     // requested. If the client cannot support this version, it
                                     // MUST disconnect.
        ServerCapabilities Capabilities;         // The capabilities of the server.
        Implementation ServerInfo;               // The implementation of the server.
        std::optional<std::string> Instructions; // Instructions describing how to use the server
                                                 // and its features. This can be used by clients to
                                                 // improve the LLM's understanding of available
                                                 // tools, resources, etc. It can be thought of like
                                                 // a "hint" to the model. For example, this
                                                 // information MAY be added to the system prompt.

        JKEY(PROTOCOLVERSIONKEY, ProtocolVersion, "protocolVersion")
        JKEY(CAPABILITIESKEY, Capabilities, "capabilities")
        JKEY(SERVERINFOKEY, ServerInfo, "serverInfo")
        JKEY(INSTRUCTIONSKEY, Instructions, "instructions")

        DEFINE_TYPE_JSON_DERIVED(InitializeResponse::Result, ResultParams, PROTOCOLVERSIONKEY,
                                 CAPABILITIESKEY, SERVERINFOKEY, INSTRUCTIONSKEY)
    };

    InitializeResponse(const RequestID& InRequestID,
                       const InitializeResponse::Result& InResult = InitializeResponse::Result{})
        : ResponseBase(InRequestID, InResult) {}
};

// InitializedNotification {
//   MSG_DESCRIPTION : "This notification is sent from the client to the "
//                   "server after initialization has finished.",
//                   MSG_PROPERTIES
//       : {
//         MSG_METHOD : {MSG_CONST : MTHD_NOTIFICATIONS_INITIALIZED, MSG_TYPE : MSG_STRING},
//         MSG_PARAMS : {
//           MSG_ADDITIONAL_PROPERTIES : {},
//           MSG_PROPERTIES : {
//             MSG_META : {
//               MSG_ADDITIONAL_PROPERTIES : {},
//               MSG_DESCRIPTION : "This parameter name is reserved by MCP to "
//                               "allow clients and servers to attach "
//                               "additional metadata to their notifications.",
//               MSG_TYPE : MSG_OBJECT
//             }
//           },
//           MSG_TYPE : MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED : [MSG_METHOD],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * This notification is sent from the client to the server after initialization has finished.
 */
struct InitializedNotification : NotificationBase {
    InitializedNotification() : NotificationBase("notifications/initialized") {}
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

/**
 * A ping, issued by either the server or the client, to check that the other party is still alive.
 * The receiver must promptly respond, or else may be disconnected.
 */
struct PingRequest : RequestBase {
    PingRequest() : RequestBase("ping") {}
};

struct PingResponse : ResponseBase {
    PingResponse(const RequestID& InRequestID) : ResponseBase(InRequestID) {}
};

// Tool-related messages
struct ListToolsRequest : RequestBase {
    ListToolsRequest(const PaginatedRequestParams& InParams = PaginatedRequestParams{})
        : RequestBase("tools/list", InParams) {}
};

struct ListToolsResponse : ResponseBase {
    struct Result : PaginatedResultParams {
        std::vector<Tool> Tools;

        JKEY(TOOLSKEY, Tools, "tools")

        DEFINE_TYPE_JSON_DERIVED(ListToolsResponse::Result, PaginatedResultParams, TOOLSKEY)
    };

    ListToolsResponse(const RequestID& InRequestID,
                      const ListToolsResponse::Result& InResult = ListToolsResponse::Result{})
        : ResponseBase(InRequestID, InResult) {}
};

struct CallToolRequest : RequestBase {
    struct Params : RequestParams {
        std::string Name;
        std::optional<std::unordered_map<std::string, JSONData>> Arguments;

        JKEY(NAMEKEY, Name, "name")
        JKEY(ARGUMENTSKEY, Arguments, "arguments")

        DEFINE_TYPE_JSON_DERIVED(CallToolRequest::Params, RequestParams, NAMEKEY, ARGUMENTSKEY)
    };

    CallToolRequest(const CallToolRequest::Params& InParams = CallToolRequest::Params{})
        : RequestBase("tools/call", InParams) {}
};

struct CallToolResponse : ResponseBase {
    struct Result : ResultParams {
        std::vector<Content> Content;
        std::optional<bool> IsError;

        JKEY(CONTENTKEY, Content, "content")
        JKEY(ISERRORKEY, IsError, "isError")

        DEFINE_TYPE_JSON_DERIVED(CallToolResponse::Result, ResultParams, CONTENTKEY, ISERRORKEY)
    };

    CallToolResponse(const RequestID& InRequestID,
                     const CallToolResponse::Result& InResult = CallToolResponse::Result{})
        : ResponseBase(InRequestID, InResult) {}
};

// Prompt-related messages
struct ListPromptsRequest : RequestBase {
    ListPromptsRequest(const PaginatedRequestParams& InParams = PaginatedRequestParams{})
        : RequestBase("prompts/list", InParams) {}
};

struct ListPromptsResponse : ResponseBase {
    struct Result : PaginatedResultParams {
        std::vector<Prompt> Prompts;

        JKEY(PROMPTSKEY, Prompts, "prompts")

        DEFINE_TYPE_JSON_DERIVED(ListPromptsResponse::Result, PaginatedResultParams, PROMPTSKEY)
    };

    ListPromptsResponse(const RequestID& InRequestID,
                        const ListPromptsResponse::Result& InResult = ListPromptsResponse::Result{})
        : ResponseBase(InRequestID, InResult) {}
};

struct GetPromptRequest : RequestBase {
    struct Params : RequestParams {
        std::string Name;
        std::optional<std::unordered_map<std::string, std::string>> Arguments;

        JKEY(NAMEKEY, Name, "name")
        JKEY(ARGUMENTSKEY, Arguments, "arguments")

        DEFINE_TYPE_JSON_DERIVED(GetPromptRequest::Params, RequestParams, NAMEKEY, ARGUMENTSKEY)
    };

    GetPromptRequest(const GetPromptRequest::Params& InParams = GetPromptRequest::Params{})
        : RequestBase("prompts/get", InParams) {}
};

struct GetPromptResponse : ResponseBase {
    struct Result : ResultParams {
        std::optional<std::string> Description;
        std::vector<Content> Messages;

        JKEY(DESCRIPTIONKEY, Description, "description")
        JKEY(MESSAGESKEY, Messages, "messages")

        DEFINE_TYPE_JSON_DERIVED(GetPromptResponse::Result, ResultParams, DESCRIPTIONKEY,
                                 MESSAGESKEY)
    };

    GetPromptResponse(const RequestID& InRequestID,
                      const GetPromptResponse::Result& InResult = GetPromptResponse::Result{})
        : ResponseBase(InRequestID, InResult) {}
};

// Resource-related messages
struct ListResourcesRequest : RequestBase {
    ListResourcesRequest(const PaginatedRequestParams& InParams = PaginatedRequestParams{})
        : RequestBase("resources/list", InParams) {}
};

struct ListResourcesResponse : ResponseBase {
    struct Result : PaginatedResultParams {
        std::vector<Resource> Resources;

        JKEY(RESOURCESKEY, Resources, "resources")

        DEFINE_TYPE_JSON_DERIVED(ListResourcesResponse::Result, PaginatedResultParams, RESOURCESKEY)
    };

    ListResourcesResponse(
        const RequestID& InRequestID,
        const ListResourcesResponse::Result& InResult = ListResourcesResponse::Result{})
        : ResponseBase(InRequestID, InResult) {}
};

struct ReadResourceRequest : RequestBase {
    struct Params : RequestParams {
        MCP::URI URI;

        JKEY(URIKEY, URI, "uri")

        DEFINE_TYPE_JSON_DERIVED(ReadResourceRequest::Params, RequestParams, URIKEY)
    };

    ReadResourceRequest(const ReadResourceRequest::Params& InParams = ReadResourceRequest::Params{})
        : RequestBase("resources/read", InParams) {}
};

struct ReadResourceResponse : ResponseBase {
    struct Result : ResultParams {
        std::vector<std::variant<TextResourceContents, BlobResourceContents>> Contents;

        JKEY(CONTENTSKEY, Contents, "contents")

        DEFINE_TYPE_JSON_DERIVED(ReadResourceResponse::Result, ResultParams, CONTENTSKEY)
    };

    ReadResourceResponse(
        const RequestID& InRequestID,
        const ReadResourceResponse::Result& InResult = ReadResourceResponse::Result{})
        : ResponseBase(InRequestID, InResult) {}
};

// Subscribe/Unsubscribe
struct SubscribeRequest : RequestBase {
    struct Params : RequestParams {
        MCP::URI URI;

        JKEY(URIKEY, URI, "uri")

        DEFINE_TYPE_JSON_DERIVED(SubscribeRequest::Params, RequestParams, URIKEY)
    };

    SubscribeRequest(const SubscribeRequest::Params& InParams = SubscribeRequest::Params{})
        : RequestBase("resources/subscribe", InParams) {}
};

struct UnsubscribeRequest : RequestBase {
    struct Params : RequestParams {
        MCP::URI URI;

        JKEY(URIKEY, URI, "uri")

        DEFINE_TYPE_JSON_DERIVED(UnsubscribeRequest::Params, RequestParams, URIKEY)
    };

    UnsubscribeRequest(const UnsubscribeRequest::Params& InParams = UnsubscribeRequest::Params{})
        : RequestBase("resources/unsubscribe", InParams) {}
};

// Sampling-related messages
struct CreateMessageRequest : RequestBase {
    struct Params : RequestParams {
        std::vector<SamplingMessage> Messages;
        int64_t MaxTokens;
        std::optional<std::string> SystemPrompt;
        std::optional<std::string> IncludeContext; // "allServers", "thisServer", "none"
        std::optional<double> Temperature;
        std::optional<std::vector<std::string>> StopSequences;
        std::optional<ModelPreferences> ModelPrefs;
        std::optional<JSONData> Metadata;

        JKEY(MESSAGESKEY, Messages, "messages")
        JKEY(MAXTOKENSKEY, MaxTokens, "maxTokens")
        JKEY(SYSTEMPROMPTKEY, SystemPrompt, "systemPrompt")
        JKEY(INCLUDECONTEXTKEY, IncludeContext, "includeContext")
        JKEY(TEMPERATUREKEY, Temperature, "temperature")
        JKEY(STOPSEQUENCESKEY, StopSequences, "stopSequences")
        JKEY(MODELPREFSKEY, ModelPrefs, "modelPreferences")
        JKEY(METADATAKEY, Metadata, "metadata")

        DEFINE_TYPE_JSON_DERIVED(CreateMessageRequest::Params, RequestParams, MESSAGESKEY,
                                 MAXTOKENSKEY, SYSTEMPROMPTKEY, INCLUDECONTEXTKEY, TEMPERATUREKEY,
                                 STOPSEQUENCESKEY, MODELPREFSKEY, METADATAKEY)
    };

    CreateMessageRequest(
        const CreateMessageRequest::Params& InParams = CreateMessageRequest::Params{})
        : RequestBase("sampling/createMessage", InParams) {}
};

struct CreateMessageResponse : ResponseBase {
    struct Result : ResultParams {
        std::string Model;
        Role ResponseRole;
        Content ResponseContent;

        JKEY(MODELKEY, Model, "model")
        JKEY(RESPONSEROLEKEY, ResponseRole, "role")
        JKEY(RESPONSECONTENTKEY, ResponseContent, "content")

        DEFINE_TYPE_JSON_DERIVED(CreateMessageResponse::Result, ResultParams, MODELKEY,
                                 RESPONSEROLEKEY, RESPONSECONTENTKEY)
    };

    CreateMessageResponse(
        const RequestID& InRequestID,
        const CreateMessageResponse::Result& InResult = CreateMessageResponse::Result{})
        : ResponseBase(InRequestID, InResult) {}
};

// Roots-related messages
struct ListRootsRequest : RequestBase {
    ListRootsRequest(const PaginatedRequestParams& InParams = PaginatedRequestParams{})
        : RequestBase("roots/list", InParams) {}
};

struct ListRootsResponse : ResponseBase {
    struct Result : PaginatedResultParams {
        std::vector<Root> Roots;

        JKEY(ROOTSKEY, Roots, "roots")

        DEFINE_TYPE_JSON_DERIVED(ListRootsResponse::Result, PaginatedResultParams, ROOTSKEY)
    };

    ListRootsResponse(const RequestID& InRequestID,
                      const ListRootsResponse::Result& InResult = ListRootsResponse::Result{})
        : ResponseBase(InRequestID, InResult) {}
};

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

/**
 * A request from the client to the server, to enable or adjust logging.
 */
struct SetLevelRequest : RequestBase {
    struct Params : RequestParams {
        LoggingLevel Level; // The level of logging that the client wants to receive from the
                            // server. The server should send all logs at this level and higher
                            // (i.e., more severe) to the client as notifications/message.

        JKEY(LEVELKEY, Level, "level")

        DEFINE_TYPE_JSON_DERIVED(SetLevelRequest::Params, RequestParams, LEVELKEY)
    };

    SetLevelRequest(const SetLevelRequest::Params& InParams = SetLevelRequest::Params{})
        : RequestBase("logging/setLevel", InParams) {}
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

/**
 * Notification of a log message passed from server to client. If no logging/setLevel request has
 * been sent from the client, the server MAY decide which messages to send automatically.
 */
struct LoggingMessageNotification : NotificationBase {
    struct Params : NotificationParams {
        LoggingLevel Level; // The severity of this log message.
        JSONData Data; // The data to be logged, such as a string message or an object. Any JSON
                       // serializable type is allowed here.
        std::optional<std::string> Logger; // An optional name of the logger issuing this message.

        JKEY(LEVELKEY, Level, "level")
        JKEY(LOGGERKEY, Logger, "logger")
        JKEY(DATAKEY, Data, "data")

        DEFINE_TYPE_JSON_DERIVED(LoggingMessageNotification::Params, NotificationParams, LEVELKEY,
                                 LOGGERKEY, DATAKEY)

        Params(LoggingLevel InLevel = LoggingLevel::Info, JSONData InData = JSONData::object(),
               const std::optional<std::string>& InLogger = std::nullopt)
            : Level(InLevel), Data(std::move(InData)), Logger(InLogger) {}
    };

    LoggingMessageNotification(
        const LoggingMessageNotification::Params& InParams = LoggingMessageNotification::Params{})
        : NotificationBase("notifications/message", InParams) {}
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
 * An out-of-band notification used to inform the receiver of a progress update for a long-running
 * request.
 */
struct ProgressNotification : NotificationBase {
    struct Params : NotificationParams {
        std::optional<std::string> Message; // An optional message describing the current progress.
        ProgressToken
            ProgressToken; // The progress token which was given in the initial request, used to
                           // associate this notification with the request that is proceeding.
        // TODO: @HalcyonOmega - Enforce that the progress is between 0 and 1.
        double Progress; // Range from 0-1. The progress thus far. This should increase every time
                         // progress is made, even if the total is unknown.
        std::optional<int64_t>
            Total; // Total number of items to process (or total progress required), if known.

        JKEY(MESSAGEKEY, Message, "message")
        JKEY(PROGRESSTOKENKEY, ProgressToken, "progressToken")
        JKEY(PROGRESSKEY, Progress, "progress")
        JKEY(TOTALKEY, Total, "total")

        DEFINE_TYPE_JSON_DERIVED(ProgressNotification::Params, NotificationParams, MESSAGEKEY,
                                 PROGRESSTOKENKEY, PROGRESSKEY, TOTALKEY)
    };

    ProgressNotification(
        const ProgressNotification::Params& InParams = ProgressNotification::Params{})
        : NotificationBase("notifications/progress", InParams) {}
};

// Cancellation notification
struct CancelledNotification : NotificationBase {
    struct Params : NotificationParams {
        RequestID CancelRequestID;
        std::optional<std::string> Reason;

        JKEY(CANCELREQUESTIDKEY, CancelRequestID, "requestId")
        JKEY(REASONKEY, Reason, "reason")

        DEFINE_TYPE_JSON_DERIVED(CancelledNotification::Params, NotificationParams,
                                 CANCELREQUESTIDKEY, REASONKEY)
    };

    CancelledNotification(
        const CancelledNotification::Params& InParams = CancelledNotification::Params{})
        : NotificationBase("notifications/cancelled", InParams) {}
};

// Change notifications
struct ResourceListChangedNotification : NotificationBase {
    ResourceListChangedNotification() : NotificationBase("notifications/resources/list_changed") {}
};

struct ResourceUpdatedNotification : NotificationBase {
    struct Params : NotificationParams {
        MCP::URI URI;

        JKEY(URIKEY, URI, "uri")

        DEFINE_TYPE_JSON_DERIVED(ResourceUpdatedNotification::Params, NotificationParams, URIKEY)
    };

    ResourceUpdatedNotification(const Params& InParams = Params{})
        : NotificationBase("notifications/resources/updated", InParams) {}
};

struct PromptListChangedNotification : NotificationBase {
    PromptListChangedNotification() : NotificationBase("notifications/prompts/list_changed") {}
};

struct ToolListChangedNotification : NotificationBase {
    ToolListChangedNotification() : NotificationBase("notifications/tools/list_changed") {}
};

struct RootsListChangedNotification : NotificationBase {
    RootsListChangedNotification() : NotificationBase("notifications/roots/list_changed") {}
};

// Completion request/response
struct CompleteRequest : RequestBase {
    struct Params : RequestParams {
        struct Reference {
            // TODO: @HalcyonOmega - Consider using an enum for the type
            std::string Type; // "ref/prompt" or "ref/resource"
            MCP::URI URI;

            JKEY(TYPEKEY, Type, "type")
            JKEY(URIKEY, URI, "uri")

            DEFINE_TYPE_JSON(CompleteRequest::Params::Reference, TYPEKEY, URIKEY)

        } Reference;

        struct Argument {
            std::string Name;
            std::string Value;

            JKEY(NAMEKEY, Name, "name")
            JKEY(VALUEKEY, Value, "value")

            DEFINE_TYPE_JSON(CompleteRequest::Params::Argument, NAMEKEY, VALUEKEY)

        } Argument;

        JKEY(REFERENCEKEY, Reference, "ref")
        JKEY(ARGUMENTKEY, Argument, "argument")

        DEFINE_TYPE_JSON_DERIVED(CompleteRequest::Params, RequestParams, REFERENCEKEY, ARGUMENTKEY)
    };

    CompleteRequest(const CompleteRequest::Params& InParams = CompleteRequest::Params{})
        : RequestBase("completion/complete", InParams) {}
};

struct CompleteResponse : ResponseBase {
    struct Result : ResultParams {
        struct Completion {
            std::vector<std::string> Values;
            std::optional<int64_t> Total;
            std::optional<bool> HasMore;

            JKEY(VALUESKEY, Values, "values")
            JKEY(TOTALKEY, Total, "total")
            JKEY(HASMOREKEY, HasMore, "hasMore")

            DEFINE_TYPE_JSON(CompleteResponse::Result::Completion, VALUESKEY, TOTALKEY, HASMOREKEY)
        } CompletionData;

        JKEY(COMPLETIONDATAKEY, CompletionData, "completion")

        DEFINE_TYPE_JSON_DERIVED(CompleteResponse::Result, ResultParams, COMPLETIONDATAKEY)
    };

    CompleteResponse(const RequestID& InRequestID,
                     const CompleteResponse::Result& InResult = CompleteResponse::Result{})
        : ResponseBase(InRequestID, InResult) {}
};

// Union types for polymorphic handling
using AnyRequest =
    std::variant<InitializeRequest, PingRequest, ListToolsRequest, CallToolRequest,
                 ListPromptsRequest, GetPromptRequest, ListResourcesRequest, ReadResourceRequest,
                 SubscribeRequest, UnsubscribeRequest, CreateMessageRequest, ListRootsRequest,
                 SetLevelRequest, CompleteRequest>;

using AnyResponse =
    std::variant<InitializeResponse, PingResponse, ListToolsResponse, CallToolResponse,
                 ListPromptsResponse, GetPromptResponse, ListResourcesResponse,
                 ReadResourceResponse, CreateMessageResponse, ListRootsResponse, CompleteResponse>;

using AnyNotification =
    std::variant<InitializedNotification, ProgressNotification, CancelledNotification,
                 ResourceListChangedNotification, ResourceUpdatedNotification,
                 PromptListChangedNotification, ToolListChangedNotification,
                 RootsListChangedNotification, LoggingMessageNotification>;

MCP_NAMESPACE_END