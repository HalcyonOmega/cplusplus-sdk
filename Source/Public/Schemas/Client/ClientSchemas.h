// Client-side schemas for Model Context Protocol
#pragma once

#include "Core.h"
#include "Schemas/Common/CommonSchemas.h"

MCP_NAMESPACE_BEGIN

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
    optional<vector<Role>> Audience;

    /**
     * Describes how important this data is for operating the server.
     *
     * A value of 1 means "most important," and indicates that the data is
     * effectively required, while 0 means "least important," and indicates that
     * the data is entirely optional.

     * TODO: Fix External Ref: @TJS-type number
     * @TJS-type number
     * @minimum 0
     * @maximum 1
     */
    optional<number> Priority;
};

MCP_NAMESPACE_END