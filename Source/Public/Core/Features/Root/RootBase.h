#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// Root {
//     MSG_DESCRIPTION : "Represents a root directory or file that the server can operate on.",
//                     MSG_PROPERTIES
//         : {
//             MSG_NAME: {
//                 MSG_DESCRIPTION: "An optional name for the root. This can be used to provide a "
//                                "human-readable\nidentifier for the root, which may be useful for
//                                " "display purposes " "or for\nreferencing the root in other parts
//                                of the application.",
//                 MSG_TYPE: MSG_STRING
//             },
//             MSG_URI: {
//                 MSG_DESCRIPTION: "The URI identifying the root. This *must* start with file://
//                 for
//                 "
//                                "now.\nThis restriction may be relaxed in future versions of the "
//                                "protocol to allow\nother URI schemes.",
//                 MSG_FORMAT: MSG_URI,
//                 MSG_TYPE: MSG_STRING
//             }
//         },
//           MSG_REQUIRED : [MSG_URI],
//                        MSG_TYPE : MSG_OBJECT
// };

// Represents a root directory or file that the server can operate on.
struct Root {
    URIFile URI; // The URI identifying the root. This *must* start with file:// for now. This
                 // restriction may be relaxed in future versions of the protocol to allow other URI
                 // schemes.
    optional<string>
        Name; // An optional name for the root. This can be used to provide a human-readable
              // identifier for the root, which may be useful for display purposes or for
              // referencing the root in other parts of the application.
};

MCP_NAMESPACE_END