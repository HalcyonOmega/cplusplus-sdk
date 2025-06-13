#pragma once

#include "Core.h"
#include "Core/Messages/Requests/Requests.h"

MCP_NAMESPACE_BEGIN

// Sent from the client to request a list of resources the server has.
struct ListResourcesRequest : public PaginatedRequest {
    ListResourcesRequest() {
        Method = MTHD_RESOURCES_LIST;
    }
};

// Sent from the client to request a list of resource templates the server has.
struct ListResourceTemplatesRequest : public PaginatedRequest {
    ListResourceTemplatesRequest() {
        Method = MTHD_RESOURCES_TEMPLATES_LIST;
    }
};

// Sent from the client to the server, to read a specific resource URI.
struct ReadResourceRequest : public RequestBase {
    struct {
        BaseRequestParams;
        string URI; // The URI of the resource to read. The URI can use any protocol; it is up to
                    // the server how to interpret it.
    } Params;

    ReadResourceRequest() {
        Method = MTHD_RESOURCES_READ;
    }
};

// Sent from the client to request resources/updated notifications from the server whenever a
// particular resource changes.
struct SubscribeRequest : public RequestBase {
    struct {
        BaseRequestParams;
        string URI; // The URI of the resource to subscribe to. The URI can use any protocol; it is
                    // up to the server how to interpret it.
    } Params;

    SubscribeRequest() {
        Method = MTHD_RESOURCES_SUBSCRIBE;
    }
};

// Sent from the client to request cancellation of resources/updated notifications from the server.
// This should follow a previous resources/subscribe request.
struct UnsubscribeRequest : public RequestBase {
    struct {
        BaseRequestParams;
        string URI; // The URI of the resource to unsubscribe from.
    } Params;

    UnsubscribeRequest() {
        Method = MTHD_RESOURCES_UNSUBSCRIBE;
    }
};

// Sent from the client to request a list of prompts and prompt templates the server has.
struct ListPromptsRequest : public PaginatedRequest {
    ListPromptsRequest() {
        Method = MTHD_PROMPTS_LIST;
    }
};

// Used by the client to get a prompt provided by the server.
struct GetPromptRequest : public RequestBase {
    struct {
        BaseRequestParams;
        string Name; // The name of the prompt or prompt template.
        optional<unordered_map<string, string>>
            Arguments; // Arguments to use for templating the prompt.
    } Params;

    GetPromptRequest() {
        Method = MTHD_PROMPTS_GET;
    }
};

// Sent from the client to request a list of tools the server has.
struct ListToolsRequest : public PaginatedRequest {
    ListToolsRequest() {
        Method = MTHD_TOOLS_LIST;
    }
};

// Used by the client to invoke a tool provided by the server.
struct CallToolRequest : public RequestBase {
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
struct SetLevelRequest : public RequestBase {
    struct {
        BaseRequestParams;
        LoggingLevel Level; // The level of logging that the client wants to receive from the
                            // server. The server should send all logs at this level and higher
                            // (i.e., more severe) to the client as notifications/logging/message.
    } Params;

    SetLevelRequest() {
        Method = MTHD_LOGGING_SET_LEVEL;
    }
};

// A request from the server to sample an LLM via the client. The client has full discretion over
// which model to select. The client should also inform the user before beginning sampling, to allow
// them to inspect the request (human in the loop) and decide whether to approve it.
struct CreateMessageRequest : public RequestBase {
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
        Method = MTHD_SAMPLING_CREATE_MESSAGE;
    }
};

// A request from the client to the server, to ask for completion options.
struct CompleteRequest : public RequestBase {
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
        Method = MTHD_COMPLETION_COMPLETE;
    }
};

// Sent from the server to request a list of root URIs from the client.
struct ListRootsRequest : public RequestBase {
    ListRootsRequest() {
        Method = MTHD_ROOTS_LIST;
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