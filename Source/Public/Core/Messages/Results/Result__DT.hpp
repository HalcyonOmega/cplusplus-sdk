#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// After receiving an initialize request from the client, the server sends this response.
struct InitializeResult : public Result {
    string ProtocolVersion; // The version of the Model Context Protocol that the server wants to
                            // use. This may not match the version that the client requested. If the
                            // client cannot support this version, it MUST disconnect.
    ServerCapabilities Capabilities; // The capabilities of the server.
    Implementation ServerInfo;       // Information about the server.
    optional<string>
        Instructions; // Instructions describing how to use the server and its features. This can be
                      // used by clients to improve the LLM's understanding of available tools,
                      // resources, etc. It can be thought of like a "hint" to the model. For
                      // example, this information MAY be added to the system prompt.
};

struct PaginatedResult : public Result {
    optional<Cursor>
        NextCursor; // An opaque token representing the pagination position after the last returned
                    // result. If present, there may be more results available.
};

// The server's response to a resources/list request from the client.
struct ListResourcesResult : public PaginatedResult {
    vector<Resource> Resources; // A list of resources.
};

// The server's response to a resources/templates/list request from the client.
struct ListResourceTemplatesResult : public PaginatedResult {
    vector<ResourceTemplate> ResourceTemplates; // A list of resource templates.
};

// The server's response to a resources/read request from the client.
struct ReadResourceResult : public Result {
    vector<variant<TextResourceContents, BLOB_ResourceContents>>
        Contents; // A list of resource contents.
};

// The server's response to a prompts/list request from the client.
struct ListPromptsResult : public PaginatedResult {
    vector<Prompt> Prompts; // A list of prompts.
};

// The server's response to a prompts/get request from the client.
struct GetPromptResult : public Result {
    optional<string> Description;   // An optional description for the prompt.
    vector<PromptMessage> Messages; // A list of prompt messages.
};

// The server's response to a tools/list request from the client.
struct ListToolsResult : public PaginatedResult {
    vector<Tool> Tools; // A list of tools.
};

// The server's response to a tool call.
struct CallToolResult : public Result {
    /*
     * A list of content objects that represent the result of the tool call.
     *
     * If the Tool does not define an output, this field MUST be present in the result.
     * For backwards compatibility, this field is always present, but it may be empty.
     */
    vector<variant<TextContent, ImageContent, AudioContent, EmbeddedResource, >> Content = {};

    optional<z.object({}).passthrough()>
        StructuredContent; // An object containing structured tool output. If the Tool defines an
                           // output, this field MUST be present in the result, and contain a JSON
                           // object that matches the schema.

    /*
     * Whether the tool call ended in an error.
     *
     * If not set, this is assumed to be false (the call was successful).
     *
     * Any errors that originate from the tool SHOULD be reported inside the result
     * object, with `isError` set to true, _not_ as an MCP protocol-level error
     * response. Otherwise, the LLM would not be able to see that an error occurred
     * and self-correct.
     *
     * However, any errors in _finding_ the tool, an error indicating that the
     * server does not support tool calls, or any other exceptional conditions,
     * should be reported as an MCP error response.
     */
    optional<bool> IsError;
};

// CallToolResult extended with backwards compatibility to protocol version 2024-10-07.
struct CompatibilityCallToolResult {
    variant < CallToolResult, struct {
        Result;
        any ToolResult;
    } > Result;
};

// The client's response to a sampling/create_message request from the server. The client should
// inform the user before returning the sampled message, to allow them to inspect the response
// (human in the loop) and decide whether to allow the server to see it.
struct CreateMessageResult : public Result {
    string Model; // The name of the model that generated the message.
    optional<z.enum(["endTurn", "stopSequence", "maxTokens"]).or (string)>
        StopReason; // The reason why sampling stopped.
    Role Role;      // The role of the message.

    // The content of the message.
    z.discriminatedUnion(MSG_TYPE, [TextContent, ImageContent, AudioContent]) Content;
};

// The server's response to a completion/complete request
struct CompleteResult : public Result {
    struct {
        // TODO: @HalcyonOmega MUST enforce max length of 100 items
        vector<string>.max(100) Values; // An array of completion values. Must not exceed 100
        items.optional<int> Total; // The total number of completion options available. This can
                                   // exceed the number of values actually sent in the response.
        optional<bool>
            HasMore; // Indicates whether there are additional completion options beyond those
                     // provided in the current response, even if the exact total is unknown.
        Passthrough Additional; // Additional properties.
    } Completion;
};

// The client's response to a roots/list request from the server.
struct ListRootsResult : public Result {
    vector<Root> Roots;
};

/* Client messages */
using ClientResult = variant<EmptyResult, CreateMessageResult, ListRootsResult>;

/* Server messages */
using ServerResult = variant<EmptyResult, InitializeResult, CompleteResult, GetPromptResult,
                             ListPromptsResult, ListResourcesResult, ListResourceTemplatesResult,
                             ReadResourceResult, CallToolResult, ListToolsResult>;

MCP_NAMESPACE_END