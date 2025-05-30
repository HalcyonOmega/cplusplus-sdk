#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

struct RequestMeta {
    optional<ProgressToken>
        ProgressToken; // If specified, the caller is requesting out-of-band progress notifications
                       // for this request (as represented by notifications/progress). The value of
                       // this parameter is an opaque token that will be attached to any subsequent
                       // notifications. The receiver is not obligated to provide these
                       // notifications.
    Passthrough Additional;
};

struct BaseRequestParams {
    optional<RequestMeta> _meta;
    Passthrough Additional;
};

struct Request {
    string Method;
    optional<BaseRequestParams> Params;
};

// This request is sent from the client to the server when it first connects, asking it to begin
// initialization.
struct InitializeRequest : public Request {
    struct {
        BaseRequestParams;
        string
            ProtocolVersion; // The latest version of the Model Context Protocol that the client
                             // supports. The client MAY decide to support older versions as well.
        ClientCapabilities Capabilities;
        Implementation ClientInfo;
    } Params;

    InitializeRequest() {
        Method = MTHD_INITIALIZE;
    }
};

const isInitializeRequest =
    (value : unknown) : value is InitializeRequest = > InitializeRequest.safeParse(value).success;

/* Ping */
// A ping, issued by either the server or the client, to check that the other party is still alive.
// The receiver must promptly respond, or else may be disconnected.
struct PingRequest : public Request {
    PingRequest() {
        Method = MTHD_PING;
    }
};

/* Pagination */
struct PaginatedRequest : public Request {
    optional < struct {
        BaseRequestParams;
        optional<Cursor>
            Cursor; // An opaque token representing the current pagination position. If provided,
                    // the server should return results starting after this cursor.
    } > Params;
};

// Sent from the client to request a list of resources the server has.
struct ListResourcesRequest : public PaginatedRequest {
    ListResourcesRequest() {
        Method = "resources/list";
    }
};

// Sent from the client to request a list of resource templates the server has.
struct ListResourceTemplatesRequest : public PaginatedRequest {
    ListResourceTemplatesRequest() {
        Method = "resources/templates/list";
    }
};

// Sent from the client to the server, to read a specific resource URI.
struct ReadResourceRequest : public Request {
    struct {
        BaseRequestParams;
        string URI; // The URI of the resource to read. The URI can use any protocol; it is up to
                    // the server how to interpret it.
    } Params;

    ReadResourceRequest() {
        Method = "resources/read";
    }
};

// Sent from the client to request resources/updated notifications from the server whenever a
// particular resource changes.
struct SubscribeRequest : public Request {
    struct {
        BaseRequestParams;
        string URI; // The URI of the resource to subscribe to. The URI can use any protocol; it is
                    // up to the server how to interpret it.
    } Params;

    SubscribeRequest() {
        Method = "resources/subscribe";
    }
};

// Sent from the client to request cancellation of resources/updated notifications from the server.
// This should follow a previous resources/subscribe request.
struct UnsubscribeRequest : public Request {
    struct {
        BaseRequestParams;
        string URI; // The URI of the resource to unsubscribe from.
    } Params;

    UnsubscribeRequest() {
        Method = "resources/unsubscribe";
    }
};

// Sent from the client to request a list of prompts and prompt templates the server has.
struct ListPromptsRequest : public PaginatedRequest {
    ListPromptsRequest() {
        Method = "prompts/list";
    }
};

// Used by the client to get a prompt provided by the server.
struct GetPromptRequest : public Request {
    struct {
        BaseRequestParams;
        string Name; // The name of the prompt or prompt template.
        optional<unordered_map<string, string>>
            Arguments; // Arguments to use for templating the prompt.
    } Params;

    GetPromptRequest() {
        Method = "prompts/get";
    }
};

// Sent from the client to request a list of tools the server has.
struct ListToolsRequest : public PaginatedRequest {
    ListToolsRequest() {
        Method = MTHD_TOOLS_LIST;
    }
};

// Used by the client to invoke a tool provided by the server.
struct CallToolRequest : public Request {
    struct {
        BaseRequestParams;
        string Name;
        optional<unordered_map<string, any>> Arguments;
    } Params;

    CallToolRequest() {
        Method = MTHD_TOOLS_CALL;
    }
};

// A request from the client to the server, to enable or adjust logging.
struct SetLevelRequest : public Request {
    struct {
        BaseRequestParams;
        LoggingLevel Level; // The level of logging that the client wants to receive from the
                            // server. The server should send all logs at this level and higher
                            // (i.e., more severe) to the client as notifications/logging/message.
    } Params;

    SetLevelRequest() {
        Method = "logging/setLevel";
    }
};

// A request from the server to sample an LLM via the client. The client has full discretion over
// which model to select. The client should also inform the user before beginning sampling, to allow
// them to inspect the request (human in the loop) and decide whether to approve it.
struct CreateMessageRequest : public Request {
    struct {
        BaseRequestParams;
        vector<SamplingMessage> Messages;
        optional<string> SystemPrompt; // An optional system prompt the server wants to use for
                                       // sampling. The client MAY modify or omit this prompt.
        optional<z.enum(["none", "thisServer", "allServers"])>
            IncludeContext; // A request to include context from one or more MCP servers (including
                            // the caller), to be attached to the prompt. The client MAY ignore this
                            // request.
        optional<double> Temperature;
        int MaxTokens; // The maximum number of tokens to sample, as requested by the server. The
                       // client MAY choose to sample fewer tokens than requested.
        optional<vector<string>> StopSequences;
        optional<z.object({}).passthrough()>
            Metadata; // Optional metadata to pass through to the LLM provider. The format of this
                      // metadata is provider-specific.
        optional<ModelPreferences>
            ModelPreferences; // The server's preferences for which model to select.
    } Params;

    CreateMessageRequest() {
        Method = "sampling/createMessage";
    }
};

// A request from the client to the server, to ask for completion options.
struct CompleteRequest : public Request {
    struct {
        BaseRequestParams;
        variant<PromptReference, ResourceReference> Ref;
        struct {
            string Name;  // The name of the argument
            string Value; // The value of the argument to use for completion matching.
            Passthrough Additional;
        } Argument; // The argument's information
    } Params;

    CompleteRequest() {
        Method = "completion/complete";
    }
};

// Sent from the server to request a list of root URIs from the client.
struct ListRootsRequest : public Request {
    ListRootsRequest() {
        Method = "roots/list";
    }
};

/* Client messages */
using ClientRequest = variant<PingRequest, InitializeRequest, CompleteRequest, SetLevelRequest,
                              GetPromptRequest, ListPromptsRequest, ListResourcesRequest,
                              ListResourceTemplatesRequest, ReadResourceRequest, SubscribeRequest,
                              UnsubscribeRequest, CallToolRequest, ListToolsRequest>;

/* Server messages */
using ServerRequest = variant<PingRequest, CreateMessageRequest, ListRootsRequest>;

MCP_NAMESPACE_END