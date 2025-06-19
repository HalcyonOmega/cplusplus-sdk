#pragma once

#include "Core.h"
#include "Core/Types/Roles.h"

MCP_NAMESPACE_BEGIN

// Annotations {
//     MSG_DESCRIPTION : "Optional annotations for the client. The client can use
//     annotations to inform "
//                     "how objects are used or displayed",
//                     MSG_PROPERTIES
//         : {
//             MSG_AUDIENCE: {
//                 MSG_DESCRIPTION:
//                     "Describes who the intended customer of this object or
//                     data is.\n\nIt " "can include multiple entries to
//                     indicate content useful for multiple " "audiences (e.g.,
//                     `[\"user\", \"assistant\"]`).",
//                 MSG_ITEMS: {"$ref": "#/definitions/Role"},
//                 MSG_TYPE: MSG_ARRAY
//             },
//             MSG_PRIORITY: {
//                 MSG_DESCRIPTION:
//                     "Describes how important this data is for operating the
//                     server.\n\nA " "value of 1 means \"most important,\" and
//                     indicates that the data " "is\neffectively required,
//                     while 0 means \"least important,\" and " "indicates
//                     that\nthe data is entirely optional.",
//                 MSG_MAXIMUM: 1,
//                 MSG_MINIMUM: 0,
//                 MSG_TYPE: MSG_NUMBER
//             }
//         },
//           MSG_TYPE : MSG_OBJECT
// };

// Optional annotations for the client. The client can use annotations to inform how objects are
// used or displayed
struct Annotations {
    optional<vector<Role>>
        Audience; // Describes who the intended customer of this object or data is. It can include
                  // multiple entries to indicate content useful for multiple audiences (e.g.,
                  // `["user", "assistant"]`).
    // TODO: @HalcyonOmega Enforce min = 0, max = 1
    optional<double> Priority; // Describes how important this data is for operating the server. A
                               // value of 1 means "most important," and indicates that the data is
                               // effectively required, while 0 means "least important," and
                               // indicates that the data is entirely optional.
};

MCP_NAMESPACE_END