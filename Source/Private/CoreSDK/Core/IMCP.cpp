#include "CoreSDK/Core/IMCP.h"

#include "CoreSDK/Messages/NotificationBase.h"

MCP_NAMESPACE_BEGIN

MCPProtocol::MCPProtocol(std::unique_ptr<ITransport> InTransport, const bool InWarnOnDuplicateMessageHandlers) :
	m_State{ MCPProtocolState::Uninitialized },
	m_Transport{ std::move(InTransport) },
	m_MessageManager{ std::make_unique<MessageManager>(InWarnOnDuplicateMessageHandlers) }
{
	SetupTransportRouter();
}

bool MCPProtocol::IsInitialized() const { return m_State == MCPProtocolState::Initialized; }

MCPProtocolState MCPProtocol::GetState() const { return m_State; }

void MCPProtocol::SetState(const MCPProtocolState InNewState) { m_State = InNewState; }

bool MCPProtocol::IsConnected() const { return m_Transport->IsConnected(); }

Task<PingResponse> MCPProtocol::Ping(const PingRequest& InRequest)
{
	co_return PingResponse{ InRequest.GetRequestID() };
}

void MCPProtocol::ValidateProtocolVersion(const std::string& InVersion) {}

VoidTask MCPProtocol::SendResponse(const ResponseBase& InResponse) const
{
	co_await m_Transport->TransmitMessage(InResponse);
}

VoidTask MCPProtocol::SendNotification(const NotificationBase& InNotification) const
{
	co_await m_Transport->TransmitMessage(InNotification);
}

VoidTask MCPProtocol::SendErrorResponse(const ErrorResponseBase& InError) const
{
	co_await m_Transport->TransmitMessage(InError);
}

void MCPProtocol::SetupTransportRouter() const
{
	if (!m_Transport)
	{
		throw std::invalid_argument("Transport cannot be null");
	}

	// Set up transport handlers
	m_Transport->SetMessageRouter([this](const JSONData& InMessage) { m_MessageManager->RouteMessage(InMessage); });
}

MCP_NAMESPACE_END