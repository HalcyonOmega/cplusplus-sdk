#include "CoreSDK/Transport/ITransport.h"

#include "CoreSDK/Common/RuntimeError.h"

MCP_NAMESPACE_BEGIN

// ITransport implementation
[[nodiscard]] bool ITransport::IsConnected() const { return m_CurrentState == ETransportState::Connected; }

[[nodiscard]] ETransportState ITransport::GetState() const { return m_CurrentState; }

void ITransport::SetState(const ETransportState InNewState)
{
	m_CurrentState = InNewState;
	LogMessage("Transport State: " + ToString(m_CurrentState));
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

// TransportFactory implementation
std::unique_ptr<ITransport> TransportFactory::CreateTransport(const ETransportType InType,
	const ETransportSide InSide,
	const std::optional<std::unique_ptr<TransportOptions>>& InOptions)
{
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

			if (InSide == ETransportSide::Server)
			{
				return CreateHTTPServerTransport(*HTTPOptions);
			}

			return CreateHTTPClientTransport(*HTTPOptions);
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

std::unique_ptr<ITransport> TransportFactory::CreateHTTPServerTransport(const HTTPTransportOptions& InOptions)
{
	// Forward declaration - will be implemented in HTTPTransport.cpp
	extern std::unique_ptr<ITransport> CreateHTTPServerTransportImpl(const HTTPTransportOptions& InImplOpts);
	return CreateHTTPServerTransportImpl(InOptions);
}

std::unique_ptr<ITransport> TransportFactory::CreateHTTPClientTransport(const HTTPTransportOptions& InOptions)
{
	// Forward declaration - will be implemented in HTTPTransport.cpp
	extern std::unique_ptr<ITransport> CreateHTTPClientTransportImpl(const HTTPTransportOptions& InImplOpts);
	return CreateHTTPClientTransportImpl(InOptions);
}

MCP_NAMESPACE_END