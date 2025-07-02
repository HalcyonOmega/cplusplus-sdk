#include "CoreSDK/Core/IMCP.h"

#include "CoreSDK/Messages/NotificationBase.h"

MCP_NAMESPACE_BEGIN

MCPProtocol::MCPProtocol(std::unique_ptr<ITransport> InTransport)
    : m_Transport(std::move(InTransport)), m_MessageManager(std::make_unique<MessageManager>()) {
    SetupTransportRouter();
}

bool MCPProtocol::IsInitialized() const {
    return m_State == MCPProtocolState::Initialized;
}

MCPProtocolState MCPProtocol::GetState() const {
    return m_State;
}

void MCPProtocol::SetState(MCPProtocolState InNewState) {
    m_State = InNewState;
}

ITransport* MCPProtocol::GetTransport() const {
    return m_Transport.get();
}

bool MCPProtocol::IsConnected() const {
    return m_Transport->IsConnected();
}

MCPTask_Void MCPProtocol::SendRequest(const RequestBase& InRequest) {
    if (!m_Transport->IsConnected()) {
        HandleRuntimeError("Transport not connected");
        co_return;
    }

    // Create promise for response
    auto pendingResponse = std::make_unique<PendingResponse>();
    pendingResponse->RequestID = InRequest.ID;
    pendingResponse->StartTime = std::chrono::steady_clock::now();

    auto future = pendingResponse->Promise.get_future();
    std::string requestIDStr = InRequest.ID.ToString();

    {
        std::lock_guard<std::mutex> lock(m_ResponsesMutex);
        m_PendingResponses[requestIDStr] = std::move(pendingResponse);
    }

    try {
        // Send request via transport
        JSONData requestJSON;
        to_json(requestJSON, InRequest);
        co_await m_Transport->TransmitMessage(requestJSON);

    } catch (const std::exception&) {
        // Remove from pending requests if send failed
        std::lock_guard<std::mutex> lock(m_ResponsesMutex);
        m_PendingResponses.erase(requestIDStr);
        throw;
    }

    co_return;
}

MCPTask_Void MCPProtocol::SendResponse(const ResponseBase& InResponse) {
    JSONData responseJSON;
    to_json(responseJSON, InResponse);
    co_await m_Transport->TransmitMessage(responseJSON);
}

MCPTask_Void MCPProtocol::SendNotification(const NotificationBase& InNotification) {
    JSONData notificationJSON;
    to_json(notificationJSON, InNotification);
    co_await m_Transport->TransmitMessage(notificationJSON);
}

MCPTask_Void MCPProtocol::SendErrorResponse(const ErrorResponseBase& InErrorResponse) {
    JSONData errorJSON;
    to_json(errorJSON, InErrorResponse);
    co_await m_Transport->TransmitMessage(errorJSON);
}

void MCPProtocol::SetupTransportRouter() {
    if (!m_Transport) { throw std::invalid_argument("Transport cannot be null"); }

    // Set up transport handlers
    m_Transport->SetMessageRouter(
        [this](const JSONData& InMessage) { m_MessageManager->RouteMessage(InMessage); });
}

MCP_NAMESPACE_END