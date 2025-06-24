#include "CoreSDK/Transport/ITransport.h"

MCP_NAMESPACE_BEGIN

// ITransport implementation
void ITransport::TriggerStateChange(TransportState InNewState) {
    const TransportState oldState = m_CurrentState;
    m_CurrentState = InNewState;

    if (m_StateChangeHandler && oldState != InNewState) {
        m_StateChangeHandler(oldState, InNewState);
    }
}

// TransportFactory implementation
std::unique_ptr<ITransport>
TransportFactory::CreateTransport(TransportType InType,
                                  std::unique_ptr<TransportOptions> InOptions) {
    switch (InType) {
        case TransportType::Stdio: {
            auto* StdioOptions = dynamic_cast<StdioClientTransportOptions*>(InOptions.get());
            if (StdioOptions == nullptr) {
                throw std::invalid_argument("Invalid options for stdio transport");
            }
            return CreateStdioClientTransport(*StdioOptions);
        }
        case TransportType::StreamableHTTP: {
            auto* HTTPOptions = dynamic_cast<HTTPTransportOptions*>(InOptions.get());
            if (HTTPOptions == nullptr) {
                throw std::invalid_argument("Invalid options for HTTP transport");
            }
            return CreateHTTPTransport(*HTTPOptions);
        }
        default: throw std::invalid_argument("Unsupported transport type");
    }
}

std::unique_ptr<ITransport>
TransportFactory::CreateStdioClientTransport(const StdioClientTransportOptions& InOptions) {
    // Forward declaration - will be implemented in StdioClientTransport.cpp
    extern std::unique_ptr<ITransport> CreateStdioClientTransportImpl(
        const StdioClientTransportOptions& InOptions);
    return CreateStdioClientTransportImpl(InOptions);
}

std::unique_ptr<ITransport>
TransportFactory::CreateHTTPTransport(const HTTPTransportOptions& InOptions) {
    // Forward declaration - will be implemented in HTTPTransport.cpp
    extern std::unique_ptr<ITransport> CreateHTTPTransportImpl(
        const HTTPTransportOptions& InOptions);
    return CreateHTTPTransportImpl(InOptions);
}

MCP_NAMESPACE_END