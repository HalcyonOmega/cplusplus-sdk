#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

struct ExperimentalCapabilities {
    Passthrough Additional; // For passthrough properties
};

struct ToolsCapabilities {
    optional<bool>
        ListChanged; // Whether this server supports notifications for changes to the tool list.
    Passthrough Additional; // For passthrough properties
};

struct ResourcesCapabilities {
    optional<bool> Subscribe; // Whether this server supports subscribing to resource updates.
    optional<bool>
        ListChanged; // Whether this server supports notifications for changes to the resource list.
    Passthrough Additional; // For passthrough properties
};

struct PromptsCapabilities {
    optional<bool>
        ListChanged; // Whether this server supports notifications for changes to the prompt list.
    Passthrough Additional; // For passthrough properties
};

struct LoggingCapabilities {
    Passthrough Additional; // For passthrough properties
};

struct CompletionCapabilities {
    Passthrough Additional; // For passthrough properties
};

struct SamplingCapabilities {
    Passthrough Additional; // For passthrough properties
};

struct RootsCapabilities {
    /**
     * Whether the client supports notifications for changes to the roots list.
     */
    optional<bool> ListChanged;
};

// JSON Schema - ClientCapabilities {
//     "description"
//         : "Capabilities a client may support. Known capabilities are defined
//         here, in this schema, "
//           "but this is not a closed set: any client can define its own,
//           additional capabilities.", "properties"
//         : {
//             "experimental": {
//                 "additionalProperties":
//                     {"additionalProperties": true, "properties": {}, "type":
//                     "object"},
//                 "description": "Experimental, non-standard capabilities that
//                 the client supports.", "type": "object"
//             },
//             "roots": {
//                 "description": "Present if the client supports listing
//                 roots.", "properties": {
//                     "listChanged": {
//                         "description": "Whether the client supports
//                         notifications for changes to "
//                                        "the roots list.",
//                         "type": "boolean"
//                     }
//                 },
//                 "type": "object"
//             },
//             "sampling": {
//                 "additionalProperties": true,
//                 "description": "Present if the client supports sampling from
//                 an LLM.", "properties": {}, "type": "object"
//             }
//         },
//           "type" : "object"
// };

// Capabilities a client may support. Known capabilities are defined here, in this schema, but this
// is not a closed set: any client can define its own, additional capabilities.
struct ClientCapabilities {
    optional<ExperimentalCapabilities>
        Experimental; // Experimental, non-standard capabilities that the client supports.
    optional<SamplingCapabilities> Sampling; // Present if the client supports sampling from an LLM.
    optional<RootsCapabilities> Roots;       // Present if the client supports listing roots.
};

// JSON Schema - ServerCapabilities {
//     "description": "Capabilities that a server may support. Known
//     capabilities are defined here,
//     "
//                    "in this schema, but this is not a closed set: any server
//                    can define its own, " "additional capabilities.",
//     "properties": {
//         "completions": {
//             "additionalProperties": true,
//             "description": "Present if the server supports argument
//             autocompletion suggestions.", "properties": {}, "type": "object"
//         },
//         "experimental": {
//             "additionalProperties":
//                 {"additionalProperties": true, "properties": {}, "type":
//                 "object"},
//             "description": "Experimental, non-standard capabilities that the
//             server supports.", "type": "object"
//         },
//         "logging": {
//             "additionalProperties": true,
//             "description": "Present if the server supports sending log
//             messages to the client.", "properties": {}, "type": "object"
//         },
//         "prompts": {
//             "description": "Present if the server offers any prompt
//             templates.", "properties": {
//                 "listChanged": {
//                     "description": "Whether this server supports
//                     notifications for changes to the
//                     "
//                                    "prompt list.",
//                     "type": "boolean"
//                 }
//             },
//             "type": "object"
//         },
//         "resources": {
//             "description": "Present if the server offers any resources to
//             read.", "properties": {
//                 "listChanged": {
//                     "description": "Whether this server supports
//                     notifications for changes to the
//                     "
//                                    "resource list.",
//                     "type": "boolean"
//                 },
//                 "subscribe": {
//                     "description": "Whether this server supports subscribing
//                     to resource updates.", "type": "boolean"
//                 }
//             },
//             "type": "object"
//         },
//         "tools": {
//             "description": "Present if the server offers any tools to call.",
//             "properties": {
//                 "listChanged": {
//                     "description": "Whether this server supports
//                     notifications for changes to "
//                                    "the tool list.",
//                     "type": "boolean"
//                 }
//             },
//             "type": "object"
//         }
//     },
//     "type": "object"
// };

// Capabilities that a server may support. Known capabilities are defined here, in this schema, but
// this is not a closed set: any server can define its own, additional capabilities.
struct ServerCapabilities {
    optional<ExperimentalCapabilities>
        Experimental; // Experimental, non-standard capabilities that the server supports.
    optional<LoggingCapabilities>
        Logging; // Present if the server supports sending log messages to the client.
    optional<CompletionCapabilities>
        Completions; // Present if the server supports sending completions to the client.
    optional<PromptsCapabilities> Prompts; // Present if the server offers any prompt templates.
    optional<ResourcesCapabilities>
        Resources;                     // Present if the server offers any resources to read.
    optional<ToolsCapabilities> Tools; // Present if the server offers any tools to call.
};

MCP_NAMESPACE_END