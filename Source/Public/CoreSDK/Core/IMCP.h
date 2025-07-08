#pragma once

#include <chrono>
#include <future>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Messages/MCPMessages.h"
#include "CoreSDK/Messages/MessageManager.h"
#include "CoreSDK/Transport/ITransport.h"
#include "IMCP.h"
#include "Utilities/Async/MCPTask.h"

MCP_NAMESPACE_BEGIN

static constexpr double DEFAULT_TEMPERATURE{ 0.7 };

// Protocol state
enum class MCPProtocolState
{
	Uninitialized,
	Initializing,
	Initialized,
	Error,
	Shutdown
};

DEFINE_ENUM_JSON(MCPProtocolState,
	{ { MCPProtocolState::Uninitialized, "uninitialized" }, { MCPProtocolState::Initializing, "initializing" },
		{ MCPProtocolState::Initialized, "initialized" }, { MCPProtocolState::Error, "error" },
		{ MCPProtocolState::Shutdown, "shutdown" } });

// Base protocol handler
class MCPProtocol
{
public:
	explicit MCPProtocol(std::unique_ptr<ITransport> InTransport, bool InWarnOnDuplicateMessageHandlers);
	virtual ~MCPProtocol() = default;

	// Lifecycle
	virtual MCPTask_Void Start() = 0;
	virtual MCPTask_Void Stop() = 0;
	bool IsInitialized() const;
	MCPProtocolState GetState() const;
	void SetState(MCPProtocolState InNewState);
	ITransport* GetTransport() const; // TODO: @HalcyonOmega Consider converting to reference
	bool IsConnected() const;

	// Core protocol operations
	MCPTask<PingResponse> Ping(const PingRequest& InRequest);

	// Protocol version validation
	static const std::vector<std::string> SUPPORTED_PROTOCOL_VERSIONS;
	static void ValidateProtocolVersion(const std::string& InVersion);

	// Message sending utilities
	MCPTask_Void SendRequest(const RequestBase& InRequest);
	MCPTask_Void SendResponse(const ResponseBase& InResponse) const;
	MCPTask_Void SendNotification(const NotificationBase& InNotification) const;
	MCPTask_Void SendErrorResponse(const ErrorResponseBase& InError) const;

private:
	void SetupTransportRouter() const;

protected:
	MCPProtocolState m_State;
	std::shared_ptr<ITransport> m_Transport;
	std::shared_ptr<MessageManager> m_MessageManager;

	Implementation m_ServerInfo;
	Implementation m_ClientInfo;
	ServerCapabilities m_ServerCapabilities;
	ClientCapabilities m_ClientCapabilities;

public:
	void SetServerInfo(const Implementation& InServerInfo) { m_ServerInfo = InServerInfo; }
	void SetClientInfo(const Implementation& InClientInfo) { m_ClientInfo = InClientInfo; }
	void SetServerCapabilities(const ServerCapabilities& InServerCapabilities)
	{
		m_ServerCapabilities = InServerCapabilities;
	}
	void SetClientCapabilities(const ClientCapabilities& InClientCapabilities)
	{
		m_ClientCapabilities = InClientCapabilities;
	}

	Implementation GetServerInfo() const { return m_ServerInfo; }
	Implementation GetClientInfo() const { return m_ClientInfo; }
	ServerCapabilities GetServerCapabilities() const { return m_ServerCapabilities; }
	ClientCapabilities GetClientCapabilities() const { return m_ClientCapabilities; }

	template <typename T> struct ResponseTask
	{
		struct promise_type
		{
			std::optional<T> m_Response;
			std::exception_ptr m_Exception;

			[[nodiscard]] ResponseTask get_return_object() noexcept
			{
				return ResponseTask{ std::coroutine_handle<promise_type>::from_promise(*this) };
			}

			// On Task Begin
			std::suspend_always initial_suspend() noexcept { return {}; }
			// On Task Complete
			std::suspend_always final_suspend() noexcept { return {}; }
			void return_value(T Value) { m_Response.emplace(std::move(Value)); }
			void unhandled_exception() { m_Exception = std::current_exception(); }
		};

		struct ResponseTaskState
		{
			std::coroutine_handle<promise_type> Handle;
			std::shared_ptr<ITransport> Transport;
			std::shared_ptr<MessageManager> MessageManager;
			RequestBase Request;
		};

		std::shared_ptr<ResponseTaskState> m_State;

		// Awaitable Interface

		// Should the coroutine continue or suspend? If "false", suspend
		[[nodiscard]] bool await_ready() const noexcept { return m_State || m_State->m_Handle.done(); }

		// On Coroutine Await
		void await_suspend(std::coroutine_handle<> InAwaiter)
		{
			m_State->m_MessageManager->template RegisterResponseHandler<T>(m_State->Request.GetRequestID(),
				[State = this->m_State, InAwaiter](const T& InResponse) mutable
				{
					if (State->Handle && !State->Handle.done())
					{
						State->Handle.promise().return_value(InResponse);

						InAwaiter.resume();
					}
				});

			m_State->Transport->TransmitMessage(m_State->Request);
		}

		// On Coroutine Resume
		[[nodiscard]] T await_resume() const
		{
			m_State->MessageManager->UnregisterResponseHandler(m_State->Request.GetRequestID());

			if (m_State->Handle.promise().m_Exception)
			{
				std::rethrow_exception(m_State->Handle.promise().m_Exception);
			}

			return std::move(*m_State->Handle.promise().m_Response);
		}

		explicit ResponseTask(const std::coroutine_handle<promise_type> Handle, const RequestBase& InRequest) :
			m_Handle{ Handle }, m_Request(InRequest)
		{}
		ResponseTask() = default;
		~ResponseTask()
		{
			if (m_Handle)
			{
				m_Handle.destroy();
			}
		}

		// Non-copyable, movable
		ResponseTask(const ResponseTask&) = delete;
		ResponseTask(ResponseTask&& Other) noexcept :
			m_Handle{ std::exchange(Other.m_Handle, {}) }, m_Request(std::exchange(Other.m_Request, {}))
		{}
		ResponseTask& operator=(const ResponseTask&) = delete;
		ResponseTask& operator=(ResponseTask&& Other) noexcept
		{
			if (this != &Other)
			{
				if (m_Handle)
				{
					m_Handle.destroy();
				}
				m_Handle = std::exchange(Other.m_Handle, {});
				m_Request = std::exchange(Other.m_Request, {});
			}
			return *this;
		}
	};
};

MCP_NAMESPACE_END
