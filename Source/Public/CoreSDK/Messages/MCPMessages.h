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

// Initialize request/response
struct InitializeRequest : RequestBase {
    struct Params : RequestParams {
        std::string ProtocolVersion;
        ClientCapabilities Capabilities;
        Implementation ClientInfo;

        JKEY(PROTOCOLVERSIONKEY, ProtocolVersion, "protocolVersion")
        JKEY(CAPABILITIESKEY, Capabilities, "capabilities")
        JKEY(CLIENTINFOKEY, ClientInfo, "clientInfo")

        DEFINE_TYPE_JSON_DERIVED(InitializeRequest::Params, RequestParams, PROTOCOLVERSIONKEY,
                                 CAPABILITIESKEY, CLIENTINFOKEY)
    };

    InitializeRequest(const InitializeRequest::Params& InParams = InitializeRequest::Params{})
        : RequestBase("initialize", InParams) {}
};

struct InitializeResponse : ResponseBase {
    struct Result : ResultParams {
        std::string ProtocolVersion;
        ServerCapabilities Capabilities;
        Implementation ServerInfo;

        JKEY(PROTOCOLVERSIONKEY, ProtocolVersion, "protocolVersion")
        JKEY(CAPABILITIESKEY, Capabilities, "capabilities")
        JKEY(SERVERINFOKEY, ServerInfo, "serverInfo")

        DEFINE_TYPE_JSON_DERIVED(InitializeResponse::Result, ResultParams, PROTOCOLVERSIONKEY,
                                 CAPABILITIESKEY, SERVERINFOKEY)
    };

    InitializeResponse(const RequestID& InRequestID,
                       const InitializeResponse::Result& InResult = InitializeResponse::Result{})
        : ResponseBase(InRequestID, InResult) {}
};

// Initialized notification
struct InitializedNotification : NotificationBase {
    InitializedNotification() : NotificationBase("notifications/initialized") {}
};

// Ping request/response
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

// Logging messages
struct SetLevelRequest : RequestBase {
    struct Params : RequestParams {
        LoggingLevel Level;

        JKEY(LEVELKEY, Level, "level")

        DEFINE_TYPE_JSON_DERIVED(SetLevelRequest::Params, RequestParams, LEVELKEY)
    };

    SetLevelRequest(const SetLevelRequest::Params& InParams = SetLevelRequest::Params{})
        : RequestBase("logging/setLevel", InParams) {}
};

struct LoggingMessageNotification : NotificationBase {
    struct Params : NotificationParams {
        LoggingLevel Level;
        JSONData Data;
        std::optional<std::string> Logger;

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

// Progress notification
struct ProgressNotification : NotificationBase {
    struct Params : NotificationParams {
        std::optional<std::string> Message;
        ProgressToken ProgressToken;
        double Progress; // 0-1
        std::optional<int64_t> Total;

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