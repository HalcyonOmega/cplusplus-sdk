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

ITransport* MCPProtocol::GetTransport() const { return m_Transport.get(); }

bool MCPProtocol::IsConnected() const { return m_Transport->IsConnected(); }

MCPTask<PingResponse> MCPProtocol::Ping(const PingRequest& InRequest)
{
	co_return PingResponse{ InRequest.GetRequestID() };
}

void MCPProtocol::ValidateProtocolVersion(const std::string& InVersion) {}

template <ConcreteResponse T> MCPTask<T> MCPProtocol::SendRequest(const RequestBase& InRequest)
{
	// Verify Connection
	// If failed, check reconnect settings -> try reconnect if valid
	// Create PendingResponse paired with request, register in Message Manager
	// Transmit request over transport
	// Await message manager route to callback handler to fulfill promise for response

	if (!m_Transport->IsConnected())
	{
		HandleRuntimeError("Transport not connected");
		co_return;
	}

	m_MessageManager->RegisterResponseHandler<T>(InRequest.GetRequestID(), [this](const T& InResponse) {});

	// Create promise for response
	auto pendingResponse = std::make_unique<PendingResponse>();
	pendingResponse->RequestID = InRequest.ID;
	pendingResponse->StartTime = std::chrono::steady_clock::now();

	auto future = pendingResponse->Promise.get_future();

	{
		std::lock_guard<std::mutex> lock(m_ResponsesMutex);
		m_PendingResponses[RequestID] = std::move(pendingResponse);
	}

	try
	{
		co_await m_Transport->TransmitMessage(InRequest);
	}
	catch (const std::exception&)
	{
		// Remove from pending requests if send failed
		std::lock_guard<std::mutex> lock(m_ResponsesMutex);
		m_PendingResponses.erase(RequestID);
		throw;
	}

	co_return;
}

MCPTask_Void MCPProtocol::SendResponse(const ResponseBase& InResponse) const
{
	co_await m_Transport->TransmitMessage(InResponse);
}

MCPTask_Void MCPProtocol::SendNotification(const NotificationBase& InNotification) const
{
	co_await m_Transport->TransmitMessage(InNotification);
}

MCPTask_Void MCPProtocol::SendErrorResponse(const ErrorResponseBase& InError) const
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