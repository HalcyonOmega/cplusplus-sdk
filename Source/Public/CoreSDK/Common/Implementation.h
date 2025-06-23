#pragma once

#include <string>

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

// Describes the name and version of an MCP implementation.
struct Implementation {
    std::string Name;
    std::string Version;

    JKEY(NAMEKEY, Name, "name")
    JKEY(VERSIONKEY, Version, "version")

    DEFINE_TYPE_JSON(Implementation, NAMEKEY, VERSIONKEY)
};

MCP_NAMESPACE_END