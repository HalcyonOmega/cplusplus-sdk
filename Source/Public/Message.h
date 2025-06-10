#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

class MessageBase {
  private:
    JSON m_Message;

  public:
    // Optionally, add getter/setter for encapsulation
    const JSON& GetMessage() const {
        return m_Message;
    }
    void SetMessage(const JSON& InMessage) {
        m_Message = InMessage;
    }
};

MCP_NAMESPACE_END