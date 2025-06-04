#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

class MCP_MessageBase {
  public:
    JSON Message;
    // Optionally, add getter/setter for encapsulation
    const JSON& GetMessage() const {
        return Message;
    }
    void SetMessage(const JSON& msg) {
        Message = msg;
    }
};

MCP_NAMESPACE_END