#pragma once

#include <optional>
#include <vector>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Common/Roles.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

struct Annotations {
    std::optional<std::vector<Role>> Audience;
    std::optional<double> Priority; // 0-1 range

    JKEY(AUDIENCEKEY, Audience, "audience")
    JKEY(PRIORITYKEY, Priority, "priority")

    DEFINE_TYPE_JSON(Annotations, AUDIENCEKEY, PRIORITYKEY)
};

MCP_NAMESPACE_END