#pragma once

#include "Core.h"
#include "Core/Constants/MessageConstants.h"
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

    DEFINE_TYPE_JSON(Implementation, JKEY(Name, MSG_NAME) JKEY(Version, MSG_VERSION))
};

MCP_NAMESPACE_END