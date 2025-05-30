#pragma once

#include <string>

namespace CPP_MCP {

class Transport {
  public:
    Transport() = default;
    virtual ~Transport() = default;
    virtual void Connect() = 0;
    virtual void Disconnect() = 0;
    virtual void Send(const std::string& message) = 0;
    virtual void Receive(std::string& message) = 0;
};

} // namespace CPP_MCP