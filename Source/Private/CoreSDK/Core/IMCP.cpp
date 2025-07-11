#include "CoreSDK/Core/IMCP.h"

#include "CoreSDK/Messages/NotificationBase.h"

MCP_NAMESPACE_BEGIN

MCPProtocol::MCPProtocol(std::unique_ptr<ITransport> InTransport, const bool InWarnOnDuplicateMessageHandlers)
	: m_State{ MCPProtocolState::Uninitialized },
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

void MCPProtocol::SendMessage(const MessageBase& InMessage,
	const std::optional<std::vector<ConnectionID>>& InConnections) const
{
	m_Transport->TransmitMessage(InMessage, InConnections);
}
void MCPProtocol::InvalidCursor(RequestID InRequestID, const std::string_view InCursor) {}

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