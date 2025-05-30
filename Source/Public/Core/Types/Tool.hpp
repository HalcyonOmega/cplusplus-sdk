#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

struct ToolInput {
    string Type = "object";
    optional<Passthrough> Properties;
    optional<vector<string>> Required;
    Passthrough Additional;
};

struct ToolOutput {
    string Type = "object";
    optional<Passthrough> Properties;
    optional<vector<string>> Required;
    Passthrough Additional;
};

/*
 * Additional properties describing a Tool to clients.
 *
 * NOTE: all properties in ToolAnnotations are **hints**.
 * They are not guaranteed to provide a faithful description of
 * tool behavior (including descriptive properties like `title`).
 *
 * Clients should never make tool use decisions based on ToolAnnotations
 * received from untrusted servers.
 */
struct ToolAnnotations {
    optional<string> Title; // A human-readable title for the tool.
    optional<bool>
        ReadOnlyHint; // If true, the tool does not modify its environment. Default: false
    optional<bool>
        DestructiveHint; // If true, the tool may perform destructive updates to its environment. If
                         // false, the tool performs only additive updates. (This property is
                         // meaningful only when `readOnlyHint == false`) Default: true
    optional<bool>
        IdempotentHint; // If true, calling the tool repeatedly with the same arguments will have no
                        // additional effect on the its environment. (This property is meaningful
                        // only when `readOnlyHint == false`) Default: false
    optional<bool> OpenWorldHint; // If true, this tool may interact with an "open world" of
                                  // external entities. If false, the tool's domain of interaction
                                  // is closed. For example, the world of a web search tool is open,
                                  // whereas that of a memory tool is not. Default: true
    Passthrough Additional;       // Additional properties.
};

// Definition for a tool the client can call.
struct Tool {
    string Name;                  // The name of the tool.
    optional<string> Description; // A human-readable description of the tool.
    ToolInput Input;              // A JSON object defining the expected parameters for the tool.
    optional<ToolOutput>
        Output; // An optional JSON object defining the structure of the tool's output returned in
                // the StructuredContent field of a CallToolResult.
    optional<ToolAnnotations> Annotations; // Optional additional tool information.
    Passthrough Additional;                // Additional properties.
};

MCP_NAMESPACE_END