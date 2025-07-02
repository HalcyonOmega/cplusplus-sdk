#pragma once

#include <string>

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

// Implementation {
//   MSG_DESCRIPTION : "Describes the name and version of an MCP implementation.",
//                   MSG_PROPERTIES
//       : {MSG_NAME : {MSG_TYPE : MSG_STRING}, MSG_VERSION : {MSG_TYPE : MSG_STRING}},
//         MSG_REQUIRED : [ MSG_NAME, MSG_VERSION ],
//                      MSG_TYPE : MSG_OBJECT
// };

// Describes the name and version of an MCP implementation.
struct Implementation {
    std::string Name;
    std::string Version;

    JKEY(NAMEKEY, Name, "name")
    JKEY(VERSIONKEY, Version, "version")

    DEFINE_TYPE_JSON(Implementation, NAMEKEY, VERSIONKEY)
};

MCP_NAMESPACE_END