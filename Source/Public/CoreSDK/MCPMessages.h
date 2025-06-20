#pragma once

#include <optional>
#include <string>
#include <variant>

#include "../Macros.h"
#include "../Proxies/JSONProxy.h"
#include "../Utilities/ThirdParty/json.hpp"
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
    struct InitializeParams {
        std::string ProtocolVersion;
        ClientCapabilities Capabilities;
        Implementation ClientInfo;
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(InitializeParams, ProtocolVersion, Capabilities, ClientInfo)
    };

    InitializeRequest() {
        Method = "initialize";
        Params = InitializeParams{};
    }
};

struct InitializeResult {
    std::string ProtocolVersion;
    ServerCapabilities Capabilities;
    Implementation ServerInfo;
    std::optional<JSONValue> Meta;
};

struct InitializeResponse : ResponseBase {
    InitializeResult ResponseResult;

    InitializeResponse() {
        Result = ResponseResult;
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InitializeRequest, Method, Params)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InitializeResult, ProtocolVersion, Capabilities,
                                                ServerInfo, Meta)

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

struct ListToolsResult {
    std::vector<Tool> Tools;
    std::optional<JSONValue> Meta;

    JKEY(TOOLSKEY, Tools, "tools")
    JKEY(METAKEY, Meta, "_meta")

    DEFINE_TYPE_JSON(ListToolsResult, TOOLSKEY, METAKEY)
};

struct ListToolsResponse : ResponseBase {
    ListToolsResult ResponseResult;

    ListToolsResponse() {
        Result = ResponseResult;
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ListToolsResult, Tools, Meta)

struct CallToolRequest : RequestBase {
    struct CallToolParams {
        std::string Name;
        std::optional<std::unordered_map<std::string, JSONValue>> Arguments;
    } RequestParams;

    CallToolRequest() {
        Method = "tools/call";
        Params = RequestParams;
    }
};

struct CallToolResult {
    std::vector<Content> Content;
    std::optional<bool> IsError;
    std::optional<JSONValue> Meta;
};

struct CallToolResponse : ResponseBase {
    CallToolResult ResponseResult;

    CallToolResponse() {
        Result = ResponseResult;
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CallToolRequest::CallToolParams, Name, Arguments)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CallToolResult, Content, IsError, Meta)

// Prompt-related messages
struct ListPromptsRequest : RequestBase {
    ListPromptsRequest() {
        Method = "prompts/list";
    }
};

struct ListPromptsResult {
    std::vector<Prompt> Prompts;
    std::optional<JSONValue> Meta;
};

struct ListPromptsResponse : ResponseBase {
    ListPromptsResult ResponseResult;

    ListPromptsResponse() {
        Result = ResponseResult;
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ListPromptsResult, Prompts, Meta)

struct GetPromptRequest : RequestBase {
    struct GetPromptParams {
        std::string Name;
        std::optional<std::unordered_map<std::string, std::string>> Arguments;
    } RequestParams;

    GetPromptRequest() {
        Method = "prompts/get";
        Params = RequestParams;
    }
};

struct GetPromptResult {
    std::optional<std::string> Description;
    std::vector<Content> Messages;
    std::optional<JSONValue> Meta;
};

struct GetPromptResponse : ResponseBase {
    GetPromptResult ResponseResult;

    GetPromptResponse() {
        Result = ResponseResult;
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GetPromptRequest::GetPromptParams, Name, Arguments)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GetPromptResult, Description, Messages, Meta)

// Resource-related messages
struct ListResourcesRequest : RequestBase {
    struct ListResourcesParams {
        std::optional<std::string>
            Cursor; // Added missing Cursor parameter for pagination (MCP 2025-03-26)
    } RequestParams;

    ListResourcesRequest() {
        Method = "resources/list";
        Params = RequestParams;
    }
};

struct ListResourcesResult {
    std::vector<Resource> Resources;
    std::optional<std::string> NextCursor;
    std::optional<JSONValue> Meta;
};

struct ListResourcesResponse : ResponseBase {
    ListResourcesResult ResponseResult;

    ListResourcesResponse() {
        Result = ResponseResult;
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ListResourcesRequest::ListResourcesParams, Cursor)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ListResourcesResult, Resources, NextCursor, Meta)

struct ReadResourceRequest : RequestBase {
    struct ReadResourceParams {
        std::string URI;
    } RequestParams;

    ReadResourceRequest() {
        Method = "resources/read";
        Params = RequestParams;
    }
};

struct ReadResourceResult {
    std::vector<Content> Contents;
    std::optional<JSONValue> Meta;
};

struct ReadResourceResponse : ResponseBase {
    ReadResourceResult ResponseResult;

    ReadResourceResponse() {
        Result = ResponseResult;
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ReadResourceRequest::ReadResourceParams, URI)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ReadResourceResult, Contents, Meta)

// Subscribe/Unsubscribe
struct SubscribeRequest : RequestBase {
    struct SubscribeParams {
        std::string URI;
    } RequestParams;

    SubscribeRequest() {
        Method = "resources/subscribe";
        Params = RequestParams;
    }
};

struct UnsubscribeRequest : RequestBase {
    struct UnsubscribeParams {
        std::string URI;
    } RequestParams;

    UnsubscribeRequest() {
        Method = "resources/unsubscribe";
        Params = RequestParams;
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SubscribeRequest::SubscribeParams, URI)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(UnsubscribeRequest::UnsubscribeParams, URI)

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
    } RequestParams;

    CreateMessageRequest() {
        Method = "sampling/createMessage";
        Params = RequestParams;
    }
};

struct CreateMessageResult {
    std::string Model;
    Role ResponseRole;
    Content ResponseContent;
    std::optional<JSONValue> Meta;
};

struct CreateMessageResponse : ResponseBase {
    CreateMessageResult ResponseResult;

    CreateMessageResponse() {
        Result = ResponseResult;
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CreateMessageRequest::CreateMessageParams, Messages,
                                                MaxTokens, SystemPrompt, IncludeContext,
                                                Temperature, StopSequences, ModelPrefs, Metadata)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CreateMessageResult, Model, ResponseRole,
                                                ResponseContent, Meta)

// Roots-related messages
struct ListRootsRequest : RequestBase {
    ListRootsRequest() {
        Method = "roots/list";
    }
};

struct ListRootsResult {
    std::vector<Root> Roots;
    std::optional<JSONValue> Meta;
};

struct ListRootsResponse : ResponseBase {
    ListRootsResult ResponseResult;

    ListRootsResponse() {
        Result = ResponseResult;
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ListRootsResult, Roots, Meta)

// Logging messages
struct SetLevelRequest : RequestBase {
    struct SetLevelParams {
        LoggingLevel Level;
    } RequestParams;

    SetLevelRequest() {
        Method = "logging/setLevel";
        Params = RequestParams;
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SetLevelRequest::SetLevelParams, Level)

struct LoggingMessageNotification : NotificationBase {
    struct LoggingParams {
        LoggingLevel Level;
        std::string Logger;
        JSONValue Data;
    } NotificationParams;

    LoggingMessageNotification() {
        Method = "notifications/message";
        Params = NotificationParams;
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LoggingMessageNotification::LoggingParams, Level,
                                                Logger, Data)

// Progress notification
struct ProgressNotification : NotificationBase {
    struct ProgressParams {
        RequestID ProgressRequestID;
        double Progress; // 0-1
        std::optional<int64_t> Total;
    } NotificationParams;

    ProgressNotification() {
        Method = "notifications/progress";
        Params = NotificationParams;
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ProgressNotification::ProgressParams,
                                                ProgressRequestID, Progress, Total)

// Cancellation notification
struct CancelledNotification : NotificationBase {
    struct CancelledParams {
        RequestID CancelRequestID;
        std::optional<std::string> Reason;
    } NotificationParams;

    CancelledNotification() {
        Method = "notifications/cancelled";
        Params = NotificationParams;
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CancelledNotification::CancelledParams,
                                                CancelRequestID, Reason)

// Change notifications
struct ResourceListChangedNotification : NotificationBase {
    ResourceListChangedNotification() {
        Method = "notifications/resources/list_changed";
    }
};

struct ResourceUpdatedNotification : NotificationBase {
    struct ResourceUpdatedParams {
        std::string URI;
    } NotificationParams;

    ResourceUpdatedNotification() {
        Method = "notifications/resources/updated";
        Params = NotificationParams;
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

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ResourceUpdatedNotification::ResourceUpdatedParams,
                                                URI)

// Completion request/response
struct CompleteRequest : RequestBase {
    struct CompleteParams {
        struct CompletionRef {
            std::string Type; // "ref/prompt" or "ref/resource"
            std::string URI;
        } CompletionReference;

        struct CompletionArgument {
            std::string Name;
            std::string Value;
        } Argument;
    } RequestParams;

    CompleteRequest() {
        Method = "completion/complete";
        Params = RequestParams;
    }
};

struct CompleteResult {
    struct Completion {
        std::vector<std::string> Values;
        std::optional<int64_t> Total;
        std::optional<bool> HasMore;
    } CompletionData;
    std::optional<JSONValue> Meta;
};

struct CompleteResponse : ResponseBase {
    CompleteResult ResponseResult;

    CompleteResponse() {
        Result = ResponseResult;
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CompleteRequest::CompleteParams::CompletionRef,
                                                Type, URI)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CompleteRequest::CompleteParams::CompletionArgument,
                                                Name, Value)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CompleteRequest::CompleteParams,
                                                CompletionReference, Argument)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CompleteResult::Completion, Values, Total, HasMore)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CompleteResult, CompletionData, Meta)

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