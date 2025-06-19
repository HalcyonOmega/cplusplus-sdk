#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// TODO: @HalcyonOmega Indicate all of these can be extended with additional properties

struct ExperimentalCapabilities {};

struct ToolsCapabilities {
    optional<bool>
        ListChanged; // Whether this server supports notifications for changes to the tool list.
};

struct ResourcesCapabilities {
    optional<bool> Subscribe; // Whether this server supports subscribing to resource updates.
    optional<bool>
        ListChanged; // Whether this server supports notifications for changes to the resource list.
};

struct PromptsCapabilities {
    optional<bool>
        ListChanged; // Whether this server supports notifications for changes to the prompt list.
};

struct LoggingCapabilities {};

struct CompletionCapabilities {};

struct SamplingCapabilities {};

struct RootsCapabilities {
    optional<bool>
        ListChanged; // Whether the client supports notifications for changes to the roots list.
};

// ClientCapabilities {
//     MSG_DESCRIPTION
//         : "Capabilities a client may support. Known capabilities are defined
//         here, in this schema, "
//           "but this is not a closed set: any client can define its own,
//           additional capabilities.", MSG_PROPERTIES
//         : {
//             MSG_EXPERIMENTAL: {
//                 MSG_ADDITIONAL_PROPERTIES:
//                     {MSG_ADDITIONAL_PROPERTIES: true, MSG_PROPERTIES: {}, MSG_TYPE:
//                     MSG_OBJECT},
//                 MSG_DESCRIPTION: "Experimental, non-standard capabilities that
//                 the client supports.", MSG_TYPE: MSG_OBJECT
//             },
//             MSG_ROOTS: {
//                 MSG_DESCRIPTION: "Present if the client supports listing
//                 roots.", MSG_PROPERTIES: {
//                     MSG_LIST_CHANGED: {
//                         MSG_DESCRIPTION: "Whether the client supports
//                         notifications for changes to "
//                                        "the roots list.",
//                         MSG_TYPE: MSG_BOOLEAN
//                     }
//                 },
//                 MSG_TYPE: MSG_OBJECT
//             },
//             MSG_SAMPLING: {
//                 MSG_ADDITIONAL_PROPERTIES: true,
//                 MSG_DESCRIPTION: "Present if the client supports sampling from
//                 an LLM.", MSG_PROPERTIES: {}, MSG_TYPE: MSG_OBJECT
//             }
//         },
//           MSG_TYPE : MSG_OBJECT
// };

// Capabilities a client may support. Known capabilities are defined here, in this schema, but this
// is not a closed set: any client can define its own, additional capabilities.
struct ClientCapabilities {
    optional<ExperimentalCapabilities>
        Experimental; // Experimental, non-standard capabilities that the client supports.
    optional<SamplingCapabilities> Sampling; // Present if the client supports sampling from an LLM.
    optional<RootsCapabilities> Roots;       // Present if the client supports listing roots.
};

// ServerCapabilities {
//     MSG_DESCRIPTION: "Capabilities that a server may support. Known
//     capabilities are defined here,
//     "
//                    "in this schema, but this is not a closed set: any server
//                    can define its own, " "additional capabilities.",
//     MSG_PROPERTIES: {
//         MSG_COMPLETIONS: {
//             MSG_ADDITIONAL_PROPERTIES: true,
//             MSG_DESCRIPTION: "Present if the server supports argument
//             autocompletion suggestions.", MSG_PROPERTIES: {}, MSG_TYPE: MSG_OBJECT
//         },
//         MSG_EXPERIMENTAL: {
//             MSG_ADDITIONAL_PROPERTIES:
//                 {MSG_ADDITIONAL_PROPERTIES: true, MSG_PROPERTIES: {}, MSG_TYPE:
//                 MSG_OBJECT},
//             MSG_DESCRIPTION: "Experimental, non-standard capabilities that the
//             server supports.", MSG_TYPE: MSG_OBJECT
//         },
//         MSG_LOGGING: {
//             MSG_ADDITIONAL_PROPERTIES: true,
//             MSG_DESCRIPTION: "Present if the server supports sending log
//             messages to the client.", MSG_PROPERTIES: {}, MSG_TYPE: MSG_OBJECT
//         },
//         MSG_PROMPTS: {
//             MSG_DESCRIPTION: "Present if the server offers any prompt
//             templates.", MSG_PROPERTIES: {
//                 MSG_LIST_CHANGED: {
//                     MSG_DESCRIPTION: "Whether this server supports
//                     notifications for changes to the
//                     "
//                                    "prompt list.",
//                     MSG_TYPE: MSG_BOOLEAN
//                 }
//             },
//             MSG_TYPE: MSG_OBJECT
//         },
//         MSG_RESOURCES: {
//             MSG_DESCRIPTION: "Present if the server offers any resources to
//             read.", MSG_PROPERTIES: {
//                 MSG_LIST_CHANGED: {
//                     MSG_DESCRIPTION: "Whether this server supports
//                     notifications for changes to the
//                     "
//                                    "resource list.",
//                     MSG_TYPE: MSG_BOOLEAN
//                 },
//                 MSG_SUBSCRIBE: {
//                     MSG_DESCRIPTION: "Whether this server supports subscribing
//                     to resource updates.", MSG_TYPE: MSG_BOOLEAN
//                 }
//             },
//             MSG_TYPE: MSG_OBJECT
//         },
//         MSG_TOOLS: {
//             MSG_DESCRIPTION: "Present if the server offers any tools to call.",
//             MSG_PROPERTIES: {
//                 MSG_LIST_CHANGED: {
//                     MSG_DESCRIPTION: "Whether this server supports
//                     notifications for changes to "
//                                    "the tool list.",
//                     MSG_TYPE: MSG_BOOLEAN
//                 }
//             },
//             MSG_TYPE: MSG_OBJECT
//         }
//     },
//     MSG_TYPE: MSG_OBJECT
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