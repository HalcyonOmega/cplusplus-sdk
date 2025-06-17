#pragma once

#include "Core.h"
#include "Custom/Custom.h"
#include "ITransport.h"
#include "Stdio/Stdio.h"
#include "StreamableHTTP/StreamableHTTP.h"

MCP_NAMESPACE_BEGIN

// Transport Factory
class TransportFactory {
  public:
    static std::unique_ptr<ITransport>
    CreateStdioTransport(const std::string& InExecutablePath,
                         const std::vector<std::string>& InArguments) {
        return std::make_unique<StdioTransport>(InExecutablePath, InArguments);
    }

    static std::unique_ptr<ITransport>
    CreateStreamableHTTPTransport(const HTTPTransportConfig& InConfig) {
        return std::make_unique<StreamableHTTPTransport>(InConfig);
    }
};

MCP_NAMESPACE_END