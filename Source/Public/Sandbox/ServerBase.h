#pragma once

#include "Core.h"
#include "Sandbox/IProtocol.h"
#include "Sandbox/ISession.h"
#include "Sandbox/ITransport.h"

MCP_NAMESPACE_BEGIN

class ServerBase : public IProtocol {
  public:
    ServerBase(const ServerOptions& InOptions);
    ~ServerBase() override;

    MCPTask_Void Start();
    MCPTask_Void Stop();
};

MCP_NAMESPACE_END