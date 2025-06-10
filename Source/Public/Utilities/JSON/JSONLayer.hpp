#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

class JSON {
  public:
    JSON();
    JSON(const JSON& InJSON);
    JSON(JSON&& InJSON) noexcept;
    JSON(const JSON_RPC_Request& InRequest);
    JSON(const JSON_RPC_Notification& InNotification);
    JSON(const JSON_RPC_Response& InResponse);
};

MCP_NAMESPACE_END