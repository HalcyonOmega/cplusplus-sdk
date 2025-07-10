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
#include "Utilities/Async/Task.h"

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
	virtual VoidTask Start() = 0;
	virtual VoidTask Stop() = 0;
	[[nodiscard]] bool IsInitialized() const;
	[[nodiscard]] MCPProtocolState GetState() const;
	void SetState(MCPProtocolState InNewState);
	[[nodiscard]] bool IsConnected() const;

	// Core protocol operations
	Task<PingResponse> Ping(const PingRequest& InRequest);

	// Protocol version validation
	static const std::vector<std::string> SUPPORTED_PROTOCOL_VERSIONS;
	static void ValidateProtocolVersion(const std::string& InVersion);

	void SendMessage(const MessageBase& InMessage,
		const std::optional<std::vector<ConnectionID>>& InConnections = std::nullopt) const;

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

	[[nodiscard]] const Implementation& GetServerInfo() const { return m_ServerInfo; }
	[[nodiscard]] const Implementation& GetClientInfo() const { return m_ClientInfo; }
	[[nodiscard]] const ServerCapabilities& GetServerCapabilities() const { return m_ServerCapabilities; }
	[[nodiscard]] const ClientCapabilities& GetClientCapabilities() const { return m_ClientCapabilities; }

	template <ConcreteResponse T> struct ResponseTask
	{
		struct promise_type
		{
			JSONData m_RawResponse;
			mutable std::unique_ptr<T> m_Response;
			std::exception_ptr m_Exception;

			ResponseTask get_return_object() noexcept
			{
				return ResponseTask{ std::coroutine_handle<promise_type>::from_promise(*this) };
			}

			// On Task Begin
			std::suspend_always initial_suspend() noexcept { return {}; }
			// On Task Complete
			std::suspend_always final_suspend() noexcept { return {}; }
			void unhandled_exception() { m_Exception = std::current_exception(); }
			void return_void() {}

			void SetRawResponse(const JSONData& RawResponse) { m_RawResponse = RawResponse; }

			[[nodiscard]] std::optional<T> Get() const
			{
				// If we already have a successfully parsed response, return it.
				if (m_Response)
				{
					return *m_Response;
				}

				// Otherwise, try to parse the raw JSON.
				try
				{
					T Response = m_RawResponse.get<T>();
					// Cache the successful conversion.
					m_Response = std::make_unique<T>(Response);
					return Response;
				}
				catch (const nlohmann::json::type_error& Exception)
				{
					(void)Exception;
					return std::nullopt;
				}
			}

			[[nodiscard]] std::optional<JSONData> Raw() const
			{
				if (!m_RawResponse.is_null())
				{
					return m_RawResponse;
				}
				return std::nullopt;
			}

			template <ExpectedResponseFunction<T> Expected, UnexpectedResponseFunction Unexpected>
			void Handle(Expected&& OnExpected, Unexpected&& OnUnexpected) const
			{
				if (const auto& ExpectedResponse = Expected())
				{
					OnExpected(ExpectedResponse.value());
				}
				else if (const auto& UnexpectedResponse = Unexpected())
				{
					OnUnexpected(UnexpectedResponse.value());
				}
			}

			[[nodiscard]] std::optional<typename T::Result> Result() const
			{
				if (const auto& ExpectedResponse = Get())
				{
					return GetResponseResult<T::Result>(ExpectedResponse.value());
				}
				return std::nullopt;
			}
		};

		// ====================================================================
		// AWAITABLE INTERFACE
		// ====================================================================
		// Should the coroutine continue or suspend? If "false", suspend
		[[nodiscard]] bool await_ready() const noexcept { return !m_Handle || m_Handle.done(); }

		// On Coroutine Suspend
		void await_suspend(std::coroutine_handle<> InAwaiter)
		{
			if (!m_Transport->IsConnected())
			{
				HandleRuntimeError("Transport not connected");
				return;
			}

			m_MessageManager->RegisterResponseHandler(m_Request->GetRequestID(),
				[Handle = this->m_Handle, InAwaiter](const JSONData& InRawResponse) mutable
				{
					if (Handle && !Handle.done())
					{
						Handle.promise().SetRawResponse(InRawResponse);
						InAwaiter.resume();
					}
				});

			// TODO: @HalcyonOmega - Update with support for sending to specific connections
			m_Transport->TransmitMessage(*m_Request, std::nullopt);
		}

		// On Coroutine Resume
		[[nodiscard]] promise_type& await_resume() const
		{
			m_MessageManager->UnregisterResponseHandler(m_Request->GetRequestID());

			if (m_Handle.promise().m_Exception)
			{
				std::rethrow_exception(m_Handle.promise().m_Exception);
			}

			return m_Handle.promise();
		}

		void SetDependencies(std::unique_ptr<RequestBase> InRequest, std::shared_ptr<ITransport> InTransport,
			std::shared_ptr<MessageManager> InMessageManager)
		{
			m_Request = std::move(InRequest);
			m_Transport = std::move(InTransport);
			m_MessageManager = std::move(InMessageManager);
		}

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
			m_Handle{ std::exchange(Other.m_Handle, {}) },
			m_Transport{ std::move(Other.m_Transport) },
			m_MessageManager{ std::move(Other.m_MessageManager) },
			m_Request{ std::move(Other.m_Request) }
		{}
		ResponseTask& operator=(const ResponseTask&) = delete;
		ResponseTask& operator=(ResponseTask&& Other) noexcept
		{
			if (this != &Other)
			{
				if (m_Handle)
					m_Handle.destroy();
				m_Handle = std::exchange(Other.m_Handle, {});
				m_Request = std::move(Other.m_Request);
				m_Transport = std::move(Other.m_Transport);
				m_MessageManager = std::move(Other.m_MessageManager);
			}
			return *this;
		}

	private:
		// ====================================================================
		// RESPONSE TASK MEMBERS
		// ====================================================================
		std::coroutine_handle<promise_type> m_Handle;
		std::shared_ptr<ITransport> m_Transport;
		std::shared_ptr<MessageManager> m_MessageManager;
		std::unique_ptr<RequestBase> m_Request;

		explicit ResponseTask(const std::coroutine_handle<promise_type> Handle) : m_Handle{ Handle } {}

		friend class MCPProtocol;
	};

	template <ConcreteResponse T> [[nodiscard]] ResponseTask<T> SendRequest(const RequestBase& InRequest)
	{
		// Create promise & handle for coroutine
		auto InternalCoro = []() -> ResponseTask<T> { co_return; };

		// Create a task object with a valid handle
		ResponseTask<T> Task = InternalCoro();

		// Populate dependencies for coroutine to execute message sending
		Task.SetDependencies(std::make_unique<RequestBase>(InRequest), m_Transport, m_MessageManager);

		// Return a task which is fully prepped for co_await
		return Task;
	}

protected:
	MCPProtocolState m_State;
	std::shared_ptr<ITransport> m_Transport;
	std::shared_ptr<MessageManager> m_MessageManager;

	Implementation m_ServerInfo;
	Implementation m_ClientInfo;
	ServerCapabilities m_ServerCapabilities;
	ClientCapabilities m_ClientCapabilities;

private:
	void SetupTransportRouter() const;
};

MCP_NAMESPACE_END
