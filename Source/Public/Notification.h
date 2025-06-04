#pragma once

#include "Core.h"
#include "Message.h"

struct ParamsType;

MCP_NAMESPACE_BEGIN

class NotificationBase : public MessageBase {
    string Method;
};

template <typename ParamsType> class Notification : public NotificationBase {
    ParamsType Params;
};

MCP_NAMESPACE_END
