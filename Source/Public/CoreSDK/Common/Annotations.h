#pragma once

#include <optional>
#include <vector>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Common/Roles.h"
#include "JSONProxy.h"

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

/**
 * Optional annotations for the client. The client can use annotations to inform how objects are
 * used or displayed
 */
struct Annotations {
    std::optional<std::vector<Role>>
        Audience; // Describes who the intended customer of this object or data is. It can include
                  // multiple entries to indicate content useful for multiple audiences (e.g.,
                  // `["user", "assistant"]`).
    // TODO: @HalcyonOmega - Enforce that the priority is between 0 and 1.
    std::optional<double>
        Priority; // 0-1 range. Describes how important this data is for operating the server. A
                  // value of 1 means "most important," and indicates that the data is
                  // effectively required, while 0 means "least important," and
                  // indicates that the data is entirely optional.

    JKEY(AUDIENCEKEY, Audience, "audience")
    JKEY(PRIORITYKEY, Priority, "priority")

    DEFINE_TYPE_JSON(Annotations, AUDIENCEKEY, PRIORITYKEY)
};

MCP_NAMESPACE_END