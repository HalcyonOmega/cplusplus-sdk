#pragma once

#include <optional>
#include <string>
#include <variant>

#include "../Macros.h"
#include "../Proxies/JSONProxy.h"
#include "MCPTypes.h"

MCP_NAMESPACE_BEGIN

// Base message types
struct MessageBase {
    std::string_view JSONRPCVersion = "2.0";

    JKEY(JSONRPCKEY, JSONRPCVersion, "jsonrpc")

    DEFINE_TYPE_JSON(MessageBase, JSONRPCKEY)
};

struct RequestBase : MessageBase {
    RequestID ID;
    std::string Method;
    std::optional<JSONValue> Params;

    JKEY(IDKEY, ID, "id")
    JKEY(METHODKEY, Method, "method")
    JKEY(PARAMSKEY, Params, "params")

    DEFINE_TYPE_JSON_DERIVED(RequestBase, MessageBase, IDKEY, METHODKEY, PARAMSKEY)
};

struct ResponseBase : MessageBase {
    RequestID ID;
    std::optional<MCPError> Error;
    std::optional<JSONValue> Result;

    JKEY(IDKEY, ID, "id")
    JKEY(RESULTKEY, Result, "result")
    JKEY(ERRORKEY, Error, "error")

    DEFINE_TYPE_JSON_DERIVED(ResponseBase, MessageBase, IDKEY, RESULTKEY, ERRORKEY)
};

struct NotificationBase : MessageBase {
    std::string Method;
    std::optional<JSONValue> Params;

    JKEY(METHODKEY, Method, "method")
    JKEY(PARAMSKEY, Params, "params")

    DEFINE_TYPE_JSON_DERIVED(NotificationBase, MessageBase, METHODKEY, PARAMSKEY)
};

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

    InitializeRequest() {
        Method = "initialize";
        Params = InitializeRequestParams{};
    }
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

    InitializeResponse() {
        Result = InitializeResult{};
    }
};

// Initialized notification
struct InitializedNotification : NotificationBase {
    InitializedNotification() {
        Method = "notifications/initialized";
    }
};

// Ping request/response
struct PingRequest : RequestBase {
    PingRequest() {
        Method = "ping";
    }
};

struct PingResponse : ResponseBase {
    PingResponse() {
        Result = JSONValue::object();
    }
};

// Tool-related messages
struct ListToolsRequest : RequestBase {
    ListToolsRequest() {
        Method = "tools/list";
    }
};

struct ListToolsResponse : ResponseBase {
    struct ListToolsResult {
        std::vector<Tool> Tools;
        std::optional<JSONValue> Meta;

        JKEY(TOOLSKEY, Tools, "tools")
        JKEY(METAKEY, Meta, "_meta")

        DEFINE_TYPE_JSON(ListToolsResult, TOOLSKEY, METAKEY)
    };

    ListToolsResponse() {
        Result = ListToolsResult{};
    }
};

struct CallToolRequest : RequestBase {
    struct CallToolParams {
        std::string Name;
        std::optional<std::unordered_map<std::string, JSONValue>> Arguments;

        JKEY(NAMEKEY, Name, "name")
        JKEY(ARGUMENTSKEY, Arguments, "arguments")

        DEFINE_TYPE_JSON(CallToolParams, NAMEKEY, ARGUMENTSKEY)
    };

    CallToolRequest() {
        Method = "tools/call";
        Params = CallToolParams{};
    }
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

    CallToolResponse() {
        Result = CallToolResult{};
    }
};

// Prompt-related messages
struct ListPromptsRequest : RequestBase {
    ListPromptsRequest() {
        Method = "prompts/list";
    }
};

struct ListPromptsResponse : ResponseBase {
    struct ListPromptsResult {
        std::vector<Prompt> Prompts;
        std::optional<JSONValue> Meta;

        JKEY(PROMPTSKEY, Prompts, "prompts")
        JKEY(METAKEY, Meta, "_meta")

        DEFINE_TYPE_JSON(ListPromptsResult, PROMPTSKEY, METAKEY)
    };

    ListPromptsResponse() {
        Result = ListPromptsResult{};
    }
};

struct GetPromptRequest : RequestBase {
    struct GetPromptParams {
        std::string Name;
        std::optional<std::unordered_map<std::string, std::string>> Arguments;

        JKEY(NAMEKEY, Name, "name")
        JKEY(ARGUMENTSKEY, Arguments, "arguments")

        DEFINE_TYPE_JSON(GetPromptParams, NAMEKEY, ARGUMENTSKEY)
    };

    GetPromptRequest() {
        Method = "prompts/get";
        Params = GetPromptParams{};
    }
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

    GetPromptResponse() {
        Result = GetPromptResult{};
    }
};

// Resource-related messages
struct ListResourcesRequest : RequestBase {
    struct ListResourcesParams {
        std::optional<std::string> Cursor;

        JKEY(CURSORKEY, Cursor, "cursor")

        DEFINE_TYPE_JSON(ListResourcesParams, CURSORKEY)
    };

    ListResourcesRequest() {
        Method = "resources/list";
        Params = ListResourcesParams{};
    }
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

    ListResourcesResponse() {
        Result = ListResourcesResult{};
    }
};

struct ReadResourceRequest : RequestBase {
    struct ReadResourceParams {
        std::string URI;

        JKEY(URIKEY, URI, "uri")

        DEFINE_TYPE_JSON(ReadResourceParams, URIKEY)
    };

    ReadResourceRequest() {
        Method = "resources/read";
        Params = ReadResourceParams{};
    }
};

struct ReadResourceResponse : ResponseBase {
    struct ReadResourceResult {
        std::vector<Content> Contents;
        std::optional<JSONValue> Meta;

        JKEY(CONTENTSKEY, Contents, "contents")
        JKEY(METAKEY, Meta, "_meta")

        DEFINE_TYPE_JSON(ReadResourceResult, CONTENTSKEY, METAKEY)
    };

    ReadResourceResponse() {
        Result = ReadResourceResult{};
    }
};

// Subscribe/Unsubscribe
struct SubscribeRequest : RequestBase {
    struct SubscribeParams {
        std::string URI;

        JKEY(URIKEY, URI, "uri")

        DEFINE_TYPE_JSON(SubscribeParams, URIKEY)
    };

    SubscribeRequest() {
        Method = "resources/subscribe";
        Params = SubscribeParams{};
    }
};

struct UnsubscribeRequest : RequestBase {
    struct UnsubscribeParams {
        std::string URI;

        JKEY(URIKEY, URI, "uri")

        DEFINE_TYPE_JSON(UnsubscribeParams, URIKEY)
    };

    UnsubscribeRequest() {
        Method = "resources/unsubscribe";
        Params = UnsubscribeParams{};
    }
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

    CreateMessageRequest() {
        Method = "sampling/createMessage";
        Params = CreateMessageParams{};
    }
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

    CreateMessageResponse() {
        Result = CreateMessageResult{};
    }
};

// Roots-related messages
struct ListRootsRequest : RequestBase {
    ListRootsRequest() {
        Method = "roots/list";
    }
};

struct ListRootsResponse : ResponseBase {
    struct ListRootsResult {
        std::vector<Root> Roots;
        std::optional<JSONValue> Meta;

        JKEY(ROOTSKEY, Roots, "roots")
        JKEY(METAKEY, Meta, "_meta")

        DEFINE_TYPE_JSON(ListRootsResult, ROOTSKEY, METAKEY)
    };

    ListRootsResponse() {
        Result = ListRootsResult{};
    }
};

// Logging messages
struct SetLevelRequest : RequestBase {
    struct SetLevelParams {
        LoggingLevel Level;

        JKEY(LEVELKEY, Level, "level")

        DEFINE_TYPE_JSON(SetLevelParams, LEVELKEY)
    };

    SetLevelRequest() {
        Method = "logging/setLevel";
        Params = SetLevelParams{};
    }
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

    LoggingMessageNotification() {
        Method = "notifications/message";
        Params = LoggingParams{};
    }
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

    ProgressNotification() {
        Method = "notifications/progress";
        Params = ProgressParams{};
    }
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

    CancelledNotification() {
        Method = "notifications/cancelled";
        Params = CancelledParams{};
    }
};

// Change notifications
struct ResourceListChangedNotification : NotificationBase {
    ResourceListChangedNotification() {
        Method = "notifications/resources/list_changed";
    }
};

struct ResourceUpdatedNotification : NotificationBase {
    struct ResourceUpdatedParams {
        std::string URI;

        JKEY(URIKEY, URI, "uri")

        DEFINE_TYPE_JSON(ResourceUpdatedParams, URIKEY)
    };

    ResourceUpdatedNotification() {
        Method = "notifications/resources/updated";
        Params = ResourceUpdatedParams{};
    }
};

struct PromptListChangedNotification : NotificationBase {
    PromptListChangedNotification() {
        Method = "notifications/prompts/list_changed";
    }
};

struct ToolListChangedNotification : NotificationBase {
    ToolListChangedNotification() {
        Method = "notifications/tools/list_changed";
    }
};

struct RootsListChangedNotification : NotificationBase {
    RootsListChangedNotification() {
        Method = "notifications/roots/list_changed";
    }
};

// Completion request/response
struct CompleteRequest : RequestBase {
    struct CompleteParams {
        struct CompletionRef {
            std::string Type; // "ref/prompt" or "ref/resource"
            std::string URI;

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

    CompleteRequest() {
        Method = "completion/complete";
        Params = CompleteParams{};
    }
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

    CompleteResponse() {
        Result = CompleteResult{};
    }
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