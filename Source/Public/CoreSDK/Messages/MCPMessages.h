#pragma once

#include <optional>
#include <string>
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
    struct InitializeRequestParams {
        std::string ProtocolVersion;
        ClientCapabilities Capabilities;
        Implementation ClientInfo;

        JKEY(PROTOCOLVERSIONKEY, ProtocolVersion, "protocolVersion")
        JKEY(CAPABILITIESKEY, Capabilities, "capabilities")
        JKEY(CLIENTINFOKEY, ClientInfo, "clientInfo")

        DEFINE_TYPE_JSON(InitializeRequestParams, PROTOCOLVERSIONKEY, CAPABILITIESKEY,
                         CLIENTINFOKEY)
    };

    InitializeRequest(const InitializeRequestParams& InParams = InitializeRequestParams{})
        : RequestBase("initialize", InParams) {}
};

struct InitializeResponse : ResponseBase {
    struct InitializeResult {
        std::string ProtocolVersion;
        ServerCapabilities Capabilities;
        Implementation ServerInfo;
        std::optional<JSONValue> Meta;

        JKEY(PROTOCOLVERSIONKEY, ProtocolVersion, "protocolVersion")
        JKEY(CAPABILITIESKEY, Capabilities, "capabilities")
        JKEY(SERVERINFOKEY, ServerInfo, "serverInfo")
        JKEY(METAKEY, Meta, "_meta")

        DEFINE_TYPE_JSON(InitializeResult, PROTOCOLVERSIONKEY, CAPABILITIESKEY, SERVERINFOKEY,
                         METAKEY)
    };

    InitializeResponse(const RequestID& InRequestID,
                       const InitializeResult& InResult = InitializeResult{})
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
    ListToolsRequest() : RequestBase("tools/list") {}
};

struct ListToolsResponse : ResponseBase {
    struct ListToolsResult {
        std::vector<Tool> Tools;
        std::optional<JSONValue> Meta;

        JKEY(TOOLSKEY, Tools, "tools")
        JKEY(METAKEY, Meta, "_meta")

        DEFINE_TYPE_JSON(ListToolsResult, TOOLSKEY, METAKEY)
    };

    ListToolsResponse(const RequestID& InRequestID,
                      const ListToolsResult& InResult = ListToolsResult{})
        : ResponseBase(InRequestID, InResult) {}
};

struct CallToolRequest : RequestBase {
    struct CallToolParams {
        std::string Name;
        std::optional<std::unordered_map<std::string, JSONValue>> Arguments;

        JKEY(NAMEKEY, Name, "name")
        JKEY(ARGUMENTSKEY, Arguments, "arguments")

        DEFINE_TYPE_JSON(CallToolParams, NAMEKEY, ARGUMENTSKEY)
    };

    CallToolRequest(const CallToolParams& InParams = CallToolParams{})
        : RequestBase("tools/call", InParams) {}
};

struct CallToolResponse : ResponseBase {
    struct CallToolResult {
        std::vector<Content> Content;
        std::optional<bool> IsError;
        std::optional<JSONValue> Meta;

        JKEY(CONTENTKEY, Content, "content")
        JKEY(ISERRORKEY, IsError, "isError")
        JKEY(METAKEY, Meta, "_meta")

        DEFINE_TYPE_JSON(CallToolResult, CONTENTKEY, ISERRORKEY, METAKEY)
    };

    CallToolResponse(const RequestID& InRequestID, const CallToolResult& InResult)
        : ResponseBase(InRequestID, InResult) {}
};

// Prompt-related messages
struct ListPromptsRequest : RequestBase {
    ListPromptsRequest() : RequestBase("prompts/list") {}
};

struct ListPromptsResponse : ResponseBase {
    struct ListPromptsResult {
        std::vector<Prompt> Prompts;
        std::optional<JSONValue> Meta;

        JKEY(PROMPTSKEY, Prompts, "prompts")
        JKEY(METAKEY, Meta, "_meta")

        DEFINE_TYPE_JSON(ListPromptsResult, PROMPTSKEY, METAKEY)
    };

    ListPromptsResponse(const RequestID& InRequestID,
                        const ListPromptsResult& InResult = ListPromptsResult{})
        : ResponseBase(InRequestID, InResult) {}
};

struct GetPromptRequest : RequestBase {
    struct GetPromptParams {
        std::string Name;
        std::optional<std::unordered_map<std::string, std::string>> Arguments;

        JKEY(NAMEKEY, Name, "name")
        JKEY(ARGUMENTSKEY, Arguments, "arguments")

        DEFINE_TYPE_JSON(GetPromptParams, NAMEKEY, ARGUMENTSKEY)
    };

    GetPromptRequest(const GetPromptParams& InParams = GetPromptParams{})
        : RequestBase("prompts/get", InParams) {}
};

struct GetPromptResponse : ResponseBase {
    struct GetPromptResult {
        std::optional<std::string> Description;
        std::vector<Content> Messages;
        std::optional<JSONValue> Meta;

        JKEY(DESCRIPTIONKEY, Description, "description")
        JKEY(MESSAGESKEY, Messages, "messages")
        JKEY(METAKEY, Meta, "_meta")

        DEFINE_TYPE_JSON(GetPromptResult, DESCRIPTIONKEY, MESSAGESKEY, METAKEY)
    };

    GetPromptResponse(const RequestID& InRequestID,
                      const GetPromptResult& InResult = GetPromptResult{})
        : ResponseBase(InRequestID, InResult) {}
};

// Resource-related messages
struct ListResourcesRequest : RequestBase {
    struct ListResourcesParams {
        std::optional<std::string> Cursor;

        JKEY(CURSORKEY, Cursor, "cursor")

        DEFINE_TYPE_JSON(ListResourcesParams, CURSORKEY)
    };

    ListResourcesRequest(const ListResourcesParams& InParams = ListResourcesParams{})
        : RequestBase("resources/list", InParams) {}
};

struct ListResourcesResponse : ResponseBase {
    struct ListResourcesResult {
        std::vector<Resource> Resources;
        std::optional<std::string> NextCursor;
        std::optional<JSONValue> Meta;

        JKEY(RESOURCESKEY, Resources, "resources")
        JKEY(NEXTCURSORKEY, NextCursor, "nextCursor")
        JKEY(METAKEY, Meta, "_meta")

        DEFINE_TYPE_JSON(ListResourcesResult, RESOURCESKEY, NEXTCURSORKEY, METAKEY)
    };

    ListResourcesResponse(const RequestID& InRequestID,
                          const ListResourcesResult& InResult = ListResourcesResult{})
        : ResponseBase(InRequestID, InResult) {}
};

struct ReadResourceRequest : RequestBase {
    struct ReadResourceParams {
        MCP::URI URI;

        JKEY(URIKEY, URI, "uri")

        DEFINE_TYPE_JSON(ReadResourceParams, URIKEY)
    };

    ReadResourceRequest(const ReadResourceParams& InParams = ReadResourceParams{})
        : RequestBase("resources/read", InParams) {}
};

struct ReadResourceResponse : ResponseBase {
    struct ReadResourceResult {
        std::vector<Content> Contents;
        std::optional<JSONValue> Meta;

        JKEY(CONTENTSKEY, Contents, "contents")
        JKEY(METAKEY, Meta, "_meta")

        DEFINE_TYPE_JSON(ReadResourceResult, CONTENTSKEY, METAKEY)
    };

    ReadResourceResponse(const RequestID& InRequestID,
                         const ReadResourceResult& InResult = ReadResourceResult{})
        : ResponseBase(InRequestID, InResult) {}
};

// Subscribe/Unsubscribe
struct SubscribeRequest : RequestBase {
    struct SubscribeParams {
        MCP::URI URI;

        JKEY(URIKEY, URI, "uri")

        DEFINE_TYPE_JSON(SubscribeParams, URIKEY)
    };

    SubscribeRequest(const SubscribeParams& InParams = SubscribeParams{})
        : RequestBase("resources/subscribe", InParams) {}
};

struct UnsubscribeRequest : RequestBase {
    struct UnsubscribeParams {
        MCP::URI URI;

        JKEY(URIKEY, URI, "uri")

        DEFINE_TYPE_JSON(UnsubscribeParams, URIKEY)
    };

    UnsubscribeRequest(const UnsubscribeParams& InParams = UnsubscribeParams{})
        : RequestBase("resources/unsubscribe", InParams) {}
};

// Sampling-related messages
struct CreateMessageRequest : RequestBase {
    struct CreateMessageParams {
        std::vector<SamplingMessage> Messages;
        int64_t MaxTokens;
        std::optional<std::string> SystemPrompt;
        std::optional<std::string> IncludeContext; // "allServers", "thisServer", "none"
        std::optional<double> Temperature;
        std::optional<std::vector<std::string>> StopSequences;
        std::optional<ModelPreferences> ModelPrefs;
        std::optional<JSONValue> Metadata;

        JKEY(MESSAGESKEY, Messages, "messages")
        JKEY(MAXTOKENSKEY, MaxTokens, "maxTokens")
        JKEY(SYSTEMPROMPTKEY, SystemPrompt, "systemPrompt")
        JKEY(INCLUDECONTEXTKEY, IncludeContext, "includeContext")
        JKEY(TEMPERATUREKEY, Temperature, "temperature")
        JKEY(STOPSEQUENCESKEY, StopSequences, "stopSequences")
        JKEY(MODELPREFSKEY, ModelPrefs, "modelPreferences")
        JKEY(METADATAKEY, Metadata, "metadata")

        DEFINE_TYPE_JSON(CreateMessageParams, MESSAGESKEY, MAXTOKENSKEY, SYSTEMPROMPTKEY,
                         INCLUDECONTEXTKEY, TEMPERATUREKEY, STOPSEQUENCESKEY, MODELPREFSKEY,
                         METADATAKEY)
    };

    CreateMessageRequest(const CreateMessageParams& InParams = CreateMessageParams{})
        : RequestBase("sampling/createMessage", InParams) {}
};

struct CreateMessageResponse : ResponseBase {
    struct CreateMessageResult {
        std::string Model;
        Role ResponseRole;
        Content ResponseContent;
        std::optional<JSONValue> Meta;

        JKEY(MODELKEY, Model, "model")
        JKEY(RESPONSEROLEKEY, ResponseRole, "role")
        JKEY(RESPONSECONTENTKEY, ResponseContent, "content")
        JKEY(METAKEY, Meta, "_meta")

        DEFINE_TYPE_JSON(CreateMessageResult, MODELKEY, RESPONSEROLEKEY, RESPONSECONTENTKEY,
                         METAKEY)
    };

    CreateMessageResponse(const RequestID& InRequestID,
                          const CreateMessageResult& InResult = CreateMessageResult{})
        : ResponseBase(InRequestID, InResult) {}
};

// Roots-related messages
struct ListRootsRequest : RequestBase {
    ListRootsRequest() : RequestBase("roots/list") {}
};

struct ListRootsResponse : ResponseBase {
    struct ListRootsResult {
        std::vector<Root> Roots;
        std::optional<JSONValue> Meta;

        JKEY(ROOTSKEY, Roots, "roots")
        JKEY(METAKEY, Meta, "_meta")

        DEFINE_TYPE_JSON(ListRootsResult, ROOTSKEY, METAKEY)
    };

    ListRootsResponse(const RequestID& InRequestID,
                      const ListRootsResult& InResult = ListRootsResult{})
        : ResponseBase(InRequestID, InResult) {}
};

// Logging messages
struct SetLevelRequest : RequestBase {
    struct SetLevelParams {
        LoggingLevel Level;

        JKEY(LEVELKEY, Level, "level")

        DEFINE_TYPE_JSON(SetLevelParams, LEVELKEY)
    };

    SetLevelRequest(const SetLevelParams& InParams = SetLevelParams{})
        : RequestBase("logging/setLevel", InParams) {}
};

struct LoggingMessageNotification : NotificationBase {
    struct LoggingParams {
        LoggingLevel Level;
        std::string Logger;
        JSONValue Data;

        JKEY(LEVELKEY, Level, "level")
        JKEY(LOGGERKEY, Logger, "logger")
        JKEY(DATAKEY, Data, "data")

        DEFINE_TYPE_JSON(LoggingParams, LEVELKEY, LOGGERKEY, DATAKEY)
    };

    LoggingMessageNotification(const LoggingParams& InParams = LoggingParams{})
        : NotificationBase("notifications/message", InParams) {}
};

// Progress notification
struct ProgressNotification : NotificationBase {
    struct ProgressParams {
        RequestID ProgressRequestID;
        double Progress; // 0-1
        std::optional<int64_t> Total;

        JKEY(PROGRESSREQUESTIDKEY, ProgressRequestID, "progressToken")
        JKEY(PROGRESSKEY, Progress, "progress")
        JKEY(TOTALKEY, Total, "total")

        DEFINE_TYPE_JSON(ProgressParams, PROGRESSREQUESTIDKEY, PROGRESSKEY, TOTALKEY)
    };

    ProgressNotification(const ProgressParams& InParams = ProgressParams{})
        : NotificationBase("notifications/progress", InParams) {}
};

// Cancellation notification
struct CancelledNotification : NotificationBase {
    struct CancelledParams {
        RequestID CancelRequestID;
        std::optional<std::string> Reason;

        JKEY(CANCELREQUESTIDKEY, CancelRequestID, "requestId")
        JKEY(REASONKEY, Reason, "reason")

        DEFINE_TYPE_JSON(CancelledParams, CANCELREQUESTIDKEY, REASONKEY)
    };

    CancelledNotification(const CancelledParams& InParams = CancelledParams{})
        : NotificationBase("notifications/cancelled", InParams) {}
};

// Change notifications
struct ResourceListChangedNotification : NotificationBase {
    ResourceListChangedNotification() : NotificationBase("notifications/resources/list_changed") {}
};

struct ResourceUpdatedNotification : NotificationBase {
    struct ResourceUpdatedParams {
        MCP::URI URI;

        JKEY(URIKEY, URI, "uri")

        DEFINE_TYPE_JSON(ResourceUpdatedParams, URIKEY)
    };

    ResourceUpdatedNotification()
        : NotificationBase("notifications/resources/updated", ResourceUpdatedParams{}) {}
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
    struct CompleteParams {
        struct CompletionRef {
            std::string Type; // "ref/prompt" or "ref/resource"
            MCP::URI URI;

            JKEY(TYPEKEY, Type, "type")
            JKEY(URIKEY, URI, "uri")

            DEFINE_TYPE_JSON(CompletionRef, TYPEKEY, URIKEY)
        } CompletionReference;

        struct CompletionArgument {
            std::string Name;
            std::string Value;

            JKEY(NAMEKEY, Name, "name")
            JKEY(VALUEKEY, Value, "value")

            DEFINE_TYPE_JSON(CompletionArgument, NAMEKEY, VALUEKEY)
        } Argument;

        JKEY(COMPLETIONREFERENCEKEY, CompletionReference, "ref")
        JKEY(ARGUMENTKEY, Argument, "argument")

        DEFINE_TYPE_JSON(CompleteParams, COMPLETIONREFERENCEKEY, ARGUMENTKEY)
    };

    CompleteRequest(const CompleteParams& InParams = CompleteParams{})
        : RequestBase("completion/complete", InParams) {}
};

struct CompleteResponse : ResponseBase {
    struct CompleteResult {
        struct Completion {
            std::vector<std::string> Values;
            std::optional<int64_t> Total;
            std::optional<bool> HasMore;

            JKEY(VALUESKEY, Values, "values")
            JKEY(TOTALKEY, Total, "total")
            JKEY(HASMOREKEY, HasMore, "hasMore")

            DEFINE_TYPE_JSON(Completion, VALUESKEY, TOTALKEY, HASMOREKEY)
        } CompletionData;
        std::optional<JSONValue> Meta;

        JKEY(COMPLETIONDATAKEY, CompletionData, "completion")
        JKEY(METAKEY, Meta, "_meta")

        DEFINE_TYPE_JSON(CompleteResult, COMPLETIONDATAKEY, METAKEY)
    };

    CompleteResponse(const RequestID& InRequestID,
                     const CompleteResult& InResult = CompleteResult{})
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