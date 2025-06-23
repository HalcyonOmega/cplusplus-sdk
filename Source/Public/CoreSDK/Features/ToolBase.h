#pragma once

#include <optional>
#include <string>

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

// Additional properties describing a Tool to clients.
// NOTE: all properties in ToolAnnotations are **hints**.
// They are not guaranteed to provide a faithful description of
// tool behavior (including descriptive properties like `title`).
// Clients should never make tool use decisions based on ToolAnnotations
// received from untrusted servers.
struct ToolAnnotations {
    std::optional<std::string> Title; // A human-readable title for the tool.
    std::optional<bool>
        ReadOnlyHint; // If true, the tool does not modify its environment. Default: false
    std::optional<bool> DestructiveHint; // If true, the tool may perform destructive updates to
                                         // its environment. If false, the tool performs only
                                         // additive updates. (This property is meaningful only
                                         // when `readOnlyHint == false`) Default: true
    std::optional<bool>
        IdempotentHint; // If true, calling the tool repeatedly with the same arguments will
                        // have no additional effect on the its environment. (This property is
                        // meaningful only when `readOnlyHint == false`) Default: false
    std::optional<bool>
        OpenWorldHint; // If true, this tool may interact with an "open world" of
                       // external entities. If false, the tool's domain of interaction
                       // is closed. For example, the world of a web search tool is open,
                       // whereas that of a memory tool is not. Default: true

    JKEY(TITLEKEY, Title, "title")
    JKEY(READONLYHINTKEY, ReadOnlyHint, "readOnlyHint")
    JKEY(DESTRUCTIVEHINTKEY, DestructiveHint, "destructiveHint")
    JKEY(IDEMPOTENTHINTKEY, IdempotentHint, "idempotentHint")
    JKEY(OPENWORLDHINTKEY, OpenWorldHint, "openWorldHint")

    DEFINE_TYPE_JSON(ToolAnnotations, TITLEKEY, READONLYHINTKEY, DESTRUCTIVEHINTKEY,
                     IDEMPOTENTHINTKEY, OPENWORLDHINTKEY)
};

// Definition for a tool the client can call.
struct Tool {
    std::string Name; // The name of the tool.
    std::optional<std::string>
        Description;        // A human-readable description of the tool. This can be used by
                            // clients to improve the LLM's understanding of available tools.
                            // It can be thought of like a "hint" to the model.
    JSONSchema InputSchema; // A JSON Schema object defining the expected parameters for the tool.
    std::optional<JSONSchema>
        OutputSchema; // An optional JSON object defining the structure of the tool's output
                      // returned in the StructuredContent field of a CallToolResult.
    std::optional<ToolAnnotations> Annotations; // Optional additional tool information.

    JKEY(NAMEKEY, Name, "name")
    JKEY(DESCRIPTIONKEY, Description, "description")
    JKEY(INPUTSCHEMAKEY, InputSchema, "inputSchema")
    JKEY(OUTPUTSCHEMAKEY, OutputSchema, "outputSchema")
    JKEY(ANNOTATIONSKEY, Annotations, "annotations")

    DEFINE_TYPE_JSON(Tool, NAMEKEY, DESCRIPTIONKEY, INPUTSCHEMAKEY, OUTPUTSCHEMAKEY, ANNOTATIONSKEY)
};

MCP_NAMESPACE_END