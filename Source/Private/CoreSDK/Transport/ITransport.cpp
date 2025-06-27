#include "CoreSDK/Transport/ITransport.h"

MCP_NAMESPACE_BEGIN

// ITransport implementation
[[nodiscard]] bool ITransport::IsConnected() const {
    return m_CurrentState == TransportState::Connected;
}

[[nodiscard]] TransportState ITransport::GetState() const {
    return m_CurrentState;
}

void ITransport::SetState(TransportState InNewState) {
    m_CurrentState = InNewState;
    // TODO: Implement state change handler
}

void ITransport::SetRequestRouter(std::function<void(const RequestBase&)> InRouter) {
    m_RequestRouter = std::move(InRouter);
}

void ITransport::SetResponseRouter(std::function<void(const ResponseBase&)> InRouter) {
    m_ResponseRouter = std::move(InRouter);
}

void ITransport::SetNotificationRouter(std::function<void(const NotificationBase&)> InRouter) {
    m_NotificationRouter = std::move(InRouter);
}

void ITransport::SetErrorResponseRouter(std::function<void(const ErrorResponseBase&)> InRouter) {
    m_ErrorResponseRouter = std::move(InRouter);
}

void ITransport::CallRequestRouter(const RequestBase& InRequest) {
    if (m_RequestRouter) { m_RequestRouter(InRequest); }
}

void ITransport::CallResponseRouter(const ResponseBase& InResponse) {
    if (m_ResponseRouter) { m_ResponseRouter(InResponse); }
}

void ITransport::CallNotificationRouter(const NotificationBase& InNotification) {
    if (m_NotificationRouter) { m_NotificationRouter(InNotification); }
}

void ITransport::CallErrorResponseRouter(const ErrorResponseBase& InError) {
    if (m_ErrorResponseRouter) { m_ErrorResponseRouter(InError); }
}

// Connection management implementations
void ITransport::RegisterConnection(const ConnectionID& InConnectionID) {
    m_ActiveConnections.insert(InConnectionID);
}

void ITransport::UnregisterConnection(const ConnectionID& InConnectionID) {
    m_ActiveConnections.erase(InConnectionID);
}

[[nodiscard]] bool ITransport::IsConnectionRegistered(const ConnectionID& InConnectionID) const {
    return m_ActiveConnections.find(InConnectionID) != m_ActiveConnections.end();
}

[[nodiscard]] std::vector<ConnectionID> ITransport::GetActiveConnections() const {
    return std::vector<ConnectionID>(m_ActiveConnections.begin(), m_ActiveConnections.end());
}

MCPTask_Void
ITransport::TransmitMessageToConnections(const std::vector<ConnectionID>& InConnectionIDs,
                                         const JSONValue& InMessage) {
    for (const auto& ConnectionID : InConnectionIDs) {
        if (IsConnectionRegistered(ConnectionID)) {
            co_await TransmitMessageToConnection(ConnectionID, InMessage);
        }
    }
}

MCPTask_Void ITransport::TransmitMessageToAllConnections(const JSONValue& InMessage) {
    auto Connections = GetActiveConnections();
    co_await TransmitMessageToConnections(Connections, InMessage);
}

// TransportFactory implementation
std::unique_ptr<ITransport>
TransportFactory::CreateTransport(TransportType InType, TransportSide InSide,
                                  std::optional<std::unique_ptr<TransportOptions>> InOptions) {
    (void)InSide; // TODO: Use transport side parameter
    if (!InOptions.has_value()) { throw std::invalid_argument("Transport options are required"); }

    switch (InType) {
        case TransportType::Stdio: {
            auto* StdioOptions = dynamic_cast<StdioClientTransportOptions*>(InOptions->get());
            if (StdioOptions == nullptr) {
                throw std::invalid_argument("Invalid options for stdio transport");
            }
            return CreateStdioClientTransport(*StdioOptions);
        }
        case TransportType::StreamableHTTP: {
            auto* HTTPOptions = dynamic_cast<HTTPTransportOptions*>(InOptions->get());
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