// Client-side schemas for Model Context Protocol
#pragma once

#include "Constants.h"
#include "Core.h"
#include "Schemas/Common/CommonSchemas.h"

MCP_NAMESPACE_BEGIN

struct ClientCapabilitiesRoots {
    /**
     * Whether the client supports notifications for changes to the roots list.
     */
    optional<bool> listChanged;
};

// ClientCapabilities {
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

/**
 * Capabilities a client may support. Known capabilities are defined here, in
 * this schema, but this is not a closed set: any client can define its own,
 * additional capabilities.
 */
struct ClientCapabilities {
    /**
     * Experimental, non-standard capabilities that the client supports.
     */
    optional<AdditionalObjects> experimental;
    /**
     * Present if the client supports listing roots.
     */
    optional<ClientCapabilitiesRoots> roots;
    /**
     * Present if the client supports sampling from an LLM.
     */
    optional<JSON> sampling;
};

// Annotations {
//     "description" : "Optional annotations for the client. The client can use
//     annotations to inform "
//                     "how objects are used or displayed",
//                     "properties"
//         : {
//             "audience": {
//                 "description":
//                     "Describes who the intended customer of this object or
//                     data is.\n\nIt " "can include multiple entries to
//                     indicate content useful for multiple " "audiences (e.g.,
//                     `[\"user\", \"assistant\"]`).",
//                 "items": {"$ref": "#/definitions/Role"},
//                 "type": "array"
//             },
//             "priority": {
//                 "description":
//                     "Describes how important this data is for operating the
//                     server.\n\nA " "value of 1 means \"most important,\" and
//                     indicates that the data " "is\neffectively required,
//                     while 0 means \"least important,\" and " "indicates
//                     that\nthe data is entirely optional.",
//                 "maximum": 1,
//                 "minimum": 0,
//                 "type": "number"
//             }
//         },
//           "type" : "object"
// };

/**
 * Optional annotations for the client. The client can use annotations to inform
 * how objects are used or displayed
 */
struct Annotations {
    /**
     * Describes who the intended customer of this object or data is.
     *
     * It can include multiple entries to indicate content useful for multiple
     * audiences (e.g.,
     * `["user", "assistant"]`).
     */
    optional<vector<Role>> audience;

    /**
     * Describes how important this data is for operating the server.
     *
     * A value of 1 means "most important," and indicates that the data is
     * effectively required, while 0 means "least important," and indicates that
     * the data is entirely optional.
     *
     * @TJS-type number
     * @minimum 0
     * @maximum 1
     */
    optional<number> priority;
};

MCP_NAMESPACE_END