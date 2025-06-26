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
    const TransportState oldState = m_CurrentState;
    m_CurrentState = InNewState;

    CallStateChangeHandler(oldState, InNewState);
}

void ITransport::SetRequestHandler(RequestHandler InHandler) {
    m_RequestHandler = InHandler;
}

void ITransport::SetResponseHandler(ResponseHandler InHandler) {
    m_ResponseHandler = InHandler;
}

void ITransport::SetNotificationHandler(NotificationHandler InHandler) {
    m_NotificationHandler = InHandler;
}

void ITransport::SetErrorResponseHandler(ErrorResponseHandler InHandler) {
    m_ErrorResponseHandler = InHandler;
}

void ITransport::CallRequestHandler(const RequestBase& InRequest) {
    if (m_RequestHandler) { m_RequestHandler(InRequest); }
}

void ITransport::CallResponseHandler(const ResponseBase& InResponse) {
    if (m_ResponseHandler) { m_ResponseHandler(InResponse); }
}

void ITransport::CallNotificationHandler(const NotificationBase& InNotification) {
    if (m_NotificationHandler) { m_NotificationHandler(InNotification); }
}

void ITransport::CallErrorResponseHandler(const ErrorResponseBase& InError) {
    if (m_ErrorResponseHandler) { m_ErrorResponseHandler(InError); }
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