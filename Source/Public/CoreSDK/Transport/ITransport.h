#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"
#include "Utilities/Async/MCPTask.h"

MCP_NAMESPACE_BEGIN

// Transport events
enum class TransportState : uint8_t
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

struct StdioClientTransportOptions : TransportOptions
{
	bool UseStderr{ false };
	std::string Command;
	std::vector<std::string> Arguments;
};

struct HTTPTransportOptions : TransportOptions
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
};

// Transport interface
class ITransport
{
public:
	virtual ~ITransport() noexcept = default;
	ITransport(const ITransport&) noexcept = default;
	ITransport(ITransport&&) noexcept = default;
	ITransport& operator=(const ITransport&) noexcept = default;
	ITransport& operator=(ITransport&&) noexcept = default;

	virtual MCPTask_Void Connect() = 0;
	virtual MCPTask_Void Disconnect() = 0;
	virtual MCPTask_Void TransmitMessage(
		const JSONData& InMessage, const std::optional<std::vector<ConnectionID>>& InConnectionIDs = std::nullopt)
		= 0;
	virtual MCPTask<JSONData> TransmitRequest(
		const JSONData& InRequest, const std::optional<std::vector<ConnectionID>>& InConnectionIDs = std::nullopt)
		= 0;
	[[nodiscard]] virtual std::string GetConnectionInfo() const = 0;

	// Default Implementations
	[[nodiscard]] bool IsConnected() const;
	[[nodiscard]] TransportState GetState() const;
	void SetState(TransportState InNewState);

	void SetMessageRouter(std::function<void(const JSONData&)> InRouter);

	void CallMessageRouter(const JSONData& InMessage);

	// Connection management
	void RegisterConnection(const ConnectionID& InConnectionID);
	void UnregisterConnection(const ConnectionID& InConnectionID);
	[[nodiscard]] bool IsConnectionRegistered(const ConnectionID& InConnectionID) const;
	[[nodiscard]] std::vector<ConnectionID> GetActiveConnections() const;

private:
	TransportState m_CurrentState{ TransportState::Disconnected };

	// Event handlers
	std::function<void(const JSONData&)> m_MessageRouter;

	// Connection tracking
	std::unordered_set<ConnectionID> m_ActiveConnections;
};

// Transport factory
enum class TransportType : uint8_t
{
	Stdio,
	StreamableHTTP
};

enum class TransportSide : uint8_t
{
	Client,
	Server
};

class TransportFactory
{
public:
	[[nodiscard]] static std::unique_ptr<ITransport> CreateTransport(
		TransportType InType, TransportSide InSide, std::optional<std::unique_ptr<TransportOptions>> InOptions);

	// Convenience factory methods
	[[nodiscard]] static std::unique_ptr<ITransport> CreateStdioClientTransport(
		const StdioClientTransportOptions& InOptions);
	[[nodiscard]] static std::unique_ptr<ITransport> CreateHTTPTransport(const HTTPTransportOptions& InOptions);
};

MCP_NAMESPACE_END