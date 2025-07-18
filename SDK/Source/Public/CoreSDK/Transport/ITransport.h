#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Common/ProtocolInfo.h"
#include "JSONProxy.h"
#include "Utilities/Async/Task.h"

MCP_NAMESPACE_BEGIN

// Transport events
enum class ETransportState : uint8_t
{
	Disconnected,
	Connecting,
	Connected,
	Error
};

// Connection identifier type
using ConnectionID = std::string;

// Transport options for different transport types
struct TransportOptions
{
	TransportOptions() noexcept = default;

	virtual ~TransportOptions() noexcept = default;
	TransportOptions(const TransportOptions&) noexcept = default;
	TransportOptions(TransportOptions&&) noexcept = default;
	TransportOptions& operator=(const TransportOptions&) noexcept = default;
	TransportOptions& operator=(TransportOptions&&) noexcept = default;
};

struct StdioClientTransportOptions final : TransportOptions
{
	bool UseStderr{ false };
	std::string Command;
	std::vector<std::string> Arguments;
};

struct HTTPTransportOptions final : TransportOptions
{
	static constexpr std::chrono::milliseconds DEFAULT_CONNECT_TIMEOUT{ 5000 };
	static constexpr std::chrono::milliseconds DEFAULT_REQUEST_TIMEOUT{ 30000 };
	static constexpr std::string_view DEFAULT_HOST{ "localhost" };
	static constexpr uint16_t DEFAULT_PORT{ 8080 };
	static constexpr std::string_view DEFAULT_PATH{ "/mcp" };

	bool UseHTTPS = false;
	uint16_t Port = DEFAULT_PORT;
	std::string Host = std::string{ DEFAULT_HOST };
	std::string Path = std::string{ DEFAULT_PATH };
	std::chrono::milliseconds ConnectTimeout{ DEFAULT_CONNECT_TIMEOUT };
	std::chrono::milliseconds RequestTimeout{ DEFAULT_REQUEST_TIMEOUT };
	EProtocolVersion ProtocolVersion{ EProtocolVersion::V2025_03_26 };
};

// Transport interface
class ITransport
{
public:
	ITransport() noexcept = default;
	virtual ~ITransport() noexcept = default;
	ITransport(const ITransport&) noexcept = default;
	ITransport(ITransport&&) noexcept = default;
	ITransport& operator=(const ITransport&) noexcept = default;
	ITransport& operator=(ITransport&&) noexcept = default;

	virtual VoidTask Connect() = 0;
	virtual VoidTask Disconnect() = 0;
	virtual void TransmitMessage(const JSONData& InMessage,
		const std::optional<std::vector<ConnectionID>>& InConnectionIDs)
		= 0;
	[[nodiscard]] virtual std::string GetConnectionInfo() const = 0;

	// Default Implementations
	[[nodiscard]] bool IsConnected() const;
	[[nodiscard]] ETransportState GetState() const;
	void SetState(ETransportState InNewState);

	void SetMessageRouter(std::function<void(const JSONData&)> InRouter);

	void CallMessageRouter(const JSONData& InMessage) const;

	// Connection management
	void RegisterConnection(const ConnectionID& InConnectionID);
	void UnregisterConnection(const ConnectionID& InConnectionID);
	[[nodiscard]] bool IsConnectionRegistered(const ConnectionID& InConnectionID) const;
	[[nodiscard]] std::vector<ConnectionID> GetActiveConnections() const;

private:
	ETransportState m_CurrentState{ ETransportState::Disconnected };
	std::function<void(const JSONData&)> m_MessageRouter;
	std::unordered_set<ConnectionID> m_ActiveConnections;
};

// Transport factory
enum class ETransportType : uint8_t
{
	Stdio,
	StreamableHTTP
};

enum class ETransportSide : uint8_t
{
	Client,
	Server
};

class TransportFactory
{
public:
	[[nodiscard]] static std::unique_ptr<ITransport> CreateTransport(ETransportType InType,
		ETransportSide InSide,
		const std::optional<std::unique_ptr<TransportOptions>>& InOptions);

	// Convenience factory methods
	[[nodiscard]] static std::unique_ptr<ITransport> CreateStdioClientTransport(
		const StdioClientTransportOptions& InOptions);
	[[nodiscard]] static std::unique_ptr<ITransport> CreateHTTPTransport(const HTTPTransportOptions& InOptions);
};

MCP_NAMESPACE_END