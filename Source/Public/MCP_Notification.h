#pragma once

#include "Core.h"
#include "MCP_Message.h"

struct ParamsType;

MCP_NAMESPACE_BEGIN

class MCP_NotificationBase : public MCP_MessageBase {
    string Method;
};

template <typename ParamsType> class MCP_Notification : public MCP_NotificationBase {
    ParamsType Params;
};

MCP_NAMESPACE_END
