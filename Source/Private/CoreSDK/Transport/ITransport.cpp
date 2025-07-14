#include "CoreSDK/Transport/ITransport.h"

MCP_NAMESPACE_BEGIN

// ITransport implementation
[[nodiscard]] bool ITransport::IsConnected() const { return m_CurrentState == ETransportState::Connected; }

[[nodiscard]] ETransportState ITransport::GetState() const { return m_CurrentState; }

void ITransport::SetState(const ETransportState InNewState)
{
	m_CurrentState = InNewState;
	// TODO: Implement state change handler
}
void ITransport::SetMessageRouter(std::function<void(const JSONData&)> InRouter)
{
	m_MessageRouter = std::move(InRouter);
}

void ITransport::CallMessageRouter(const JSONData& InMessage) const
{
	if (m_MessageRouter)
	{
		m_MessageRouter(InMessage);
	}
}

// Connection management implementations
void ITransport::RegisterConnection(const ConnectionID& InConnectionID) { m_ActiveConnections.insert(InConnectionID); }

void ITransport::UnregisterConnection(const ConnectionID& InConnectionID) { m_ActiveConnections.erase(InConnectionID); }

[[nodiscard]] bool ITransport::IsConnectionRegistered(const ConnectionID& InConnectionID) const
{
	return m_ActiveConnections.contains(InConnectionID);
}

[[nodiscard]] std::vector<ConnectionID> ITransport::GetActiveConnections() const
{
	return std::vector<ConnectionID>{ m_ActiveConnections.begin(), m_ActiveConnections.end() };
}

// TODO: @HalcyonOmega [Critical] - Implement this
// MCPTask_Void
// ITransport::TransmitMessage(const JSONData& InMessage,
//                             const std::optional<std::vector<ConnectionID>>& InConnectionIDs) {
//     if (InConnectionIDs.has_value()) {
//         for (const auto& ConnectionID : InConnectionIDs.value()) {
//             if (IsConnectionRegistered(ConnectionID)) {
//                 co_await TransmitMessageToConnection(ConnectionID, InMessage);
//             } else {
//                 Logger::Warning("Connection not registered: " + ConnectionID);
//             }
//         }
//     }
// }

// TransportFactory implementation
std::unique_ptr<ITransport> TransportFactory::CreateTransport(const ETransportType InType,
	const ETransportSide InSide,
	const std::optional<std::unique_ptr<TransportOptions>>& InOptions)
{
	(void)InSide; // TODO: Use transport side parameter
	if (!InOptions)
	{
		throw std::invalid_argument("Transport options are required");
	}

	switch (InType)
	{
		case ETransportType::Stdio:
		{
			const auto* StdioOptions = dynamic_cast<StdioClientTransportOptions*>(InOptions->get());
			if (StdioOptions == nullptr)
			{
				throw std::invalid_argument("Invalid options for stdio transport");
			}
			return CreateStdioClientTransport(*StdioOptions);
		}
		case ETransportType::StreamableHTTP:
		{
			const auto* HTTPOptions = dynamic_cast<HTTPTransportOptions*>(InOptions->get());
			if (HTTPOptions == nullptr)
			{
				throw std::invalid_argument("Invalid options for HTTP transport");
			}
			return CreateHTTPTransport(*HTTPOptions);
		}
		default:
			throw std::invalid_argument("Unsupported transport type");
	}
}

std::unique_ptr<ITransport> TransportFactory::CreateStdioClientTransport(const StdioClientTransportOptions& InOptions)
{
	// Forward declaration will be implemented in StdioClientTransport.cpp
	extern std::unique_ptr<ITransport> CreateStdioClientTransportImpl(const StdioClientTransportOptions& InImplOpts);
	return CreateStdioClientTransportImpl(InOptions);
}

std::unique_ptr<ITransport> TransportFactory::CreateHTTPTransport(const HTTPTransportOptions& InOptions)
{
	// Forward declaration - will be implemented in HTTPTransport.cpp
	extern std::unique_ptr<ITransport> CreateHTTPTransportImpl(const HTTPTransportOptions& InImplOpts);
	return CreateHTTPTransportImpl(InOptions);
}

MCP_NAMESPACE_END