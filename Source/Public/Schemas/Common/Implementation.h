#pragma once

#include "Constants.h"
#include "Core.h"
#include "Utilities/JSON/JSONLayer.hpp"

MCP_NAMESPACE_BEGIN

// Implementation {
//   MSG_DESCRIPTION : "Describes the name and version of an MCP implementation.",
//                   MSG_PROPERTIES
//       : {MSG_NAME : {MSG_TYPE : MSG_STRING}, MSG_VERSION : {MSG_TYPE : MSG_STRING}},
//         MSG_REQUIRED : [ MSG_NAME, MSG_VERSION ],
//                      MSG_TYPE : MSG_OBJECT
// };

/**
 * Describes the name and version of an MCP implementation.
 */
struct Implementation {
    string Name;
    string Version;

    DEFINE_STRUCT_JSON(Implementation, Name, Version);
};

MCP_NAMESPACE_END