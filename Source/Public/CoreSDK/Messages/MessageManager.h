#pragma once

#include <coroutine>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

#include "CoreSDK/Common/RuntimeError.h"
#include "CoreSDK/Messages/ErrorResponseBase.h"
#include "CoreSDK/Messages/NotificationBase.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "CoreSDK/Messages/ResponseBase.h"
#include "JSONProxy.h"
#include "Utilities/JSON/JSONMessages.h"

MCP_NAMESPACE_BEGIN

class MessageManager
{
public:
	using MessageHandler = std::function<void(const JSONData&)>;

	// Register handlers for specific concrete message types
	template <ConcreteRequest T, typename Function> bool RegisterRequestHandler(Function&& InHandler)
	{
		static_assert(std::is_invocable_v<Function, const T&>,
			"Handler must be callable with '(const ConcreteRequestType&)'");

		T TempInstance{};
		const auto MethodName = std::string(TempInstance.GetRequestMethod());

		std::scoped_lock lock(m_RequestMutex);

		if (m_WarnOnDuplicateHandlers && m_RequestHandlers.contains(MethodName))
		{
			HandleRuntimeError("Request handler already exists for message: " + MethodName);
			return false;
		}

		m_RequestHandlers[MethodName] = [Handler = std::forward<Function>(InHandler)](const JSONData& InMessage)
		{
			JSONData JSONRequest = InMessage;
			T Request = JSONRequest.get<T>();
			Handler(Request);
		};

		return true;
	}

	template <ConcreteResponse T, typename Function>
	bool RegisterResponseHandler(const RequestID& InRequestID, Function&& InHandler)
	{
		static_assert(std::is_invocable_v<Function, const T&>,
			"Handler must be callable with '(const ConcreteResponseType&)'");

		const std::string& ID = InRequestID.ToString();
		std::scoped_lock lock(m_ResponseMutex);

		if (m_WarnOnDuplicateHandlers && m_ResponseHandlers.contains(ID))
		{
			HandleRuntimeError("Response handler already exists for message: " + ID);
			return false;
		}

		m_ResponseHandlers[ID] = InHandler;

		return true;
	}

	template <ConcreteNotification T, typename Function> bool RegisterNotificationHandler(Function&& InHandler)
	{
		static_assert(std::is_invocable_v<Function, const T&>,
			"Handler must be callable with '(const ConcreteNotificationType&)'");

		T TempInstance{};
		const auto MethodName = std::string(TempInstance.GetNotificationMethod());

		std::scoped_lock lock(m_NotificationMutex);

		if (m_WarnOnDuplicateHandlers && m_NotificationHandlers.contains(MethodName))
		{
			HandleRuntimeError("Notification handler already exists for message: " + MethodName);
			return false;
		}

		m_NotificationHandlers[MethodName] = [Handler = std::forward<Function>(InHandler)](const JSONData& InMessage)
		{
			JSONData JSONNotification = InMessage;
			T Notification = JSONNotification.get<T>();
			Handler(Notification);
		};

		return true;
	}

	// Main routing function - receives JSON string and routes to the appropriate handler
	bool RouteMessage(const std::string& InMessage)
	{
		try
		{
			const JSONData Message = JSONData::parse(InMessage);

			if (std::optional<MessageType> MessageType = GetValidMessageType(Message))
			{
				switch (MessageType.value())
				{
					case MessageType::Request:
						return RouteRequest(Message);
					case MessageType::Response:
						return RouteResponse(Message);
					case MessageType::Notification:
						return RouteNotification(Message);
					case MessageType::Error:
						return RouteResponse(Message);
					default:
						HandleRuntimeError(
							"Invalid message type: " + std::to_string(static_cast<int>(MessageType.value())));
						return false;
				}
			}

			HandleRuntimeError("Invalid JSON-RPC message received");
			return false;
		}
		catch (const std::exception& e)
		{
			HandleRuntimeError("Error parsing message: " + std::string(e.what()));
			return false;
		}
	}

	bool UnregisterRequestHandler(const std::string& InMethod)
	{

		std::scoped_lock lock(m_RequestMutex);
		return m_ResponseHandlers.erase(InMethod) > 0;
	}

	bool UnregisterNotificationHandler(const std::string& InMethod)
	{
		std::scoped_lock lock(m_NotificationMutex);
		return m_ResponseHandlers.erase(InMethod) > 0;
	}

	bool UnregisterResponseHandler(const RequestID& InRequestID)
	{
		std::scoped_lock lock(m_ResponseMutex);
		return m_ResponseHandlers.erase(InRequestID.ToString()) > 0;
	}

private:
	// Message type routing functions
	bool RouteRequest(const JSONData& InMessage)
	{
		try
		{
			const std::string Method = ExtractMethod(InMessage);

			std::scoped_lock Lock(m_RequestMutex);
			if (const auto Iter = m_RequestHandlers.find(Method); Iter != m_RequestHandlers.end())
			{
				Iter->second(InMessage);
				return true;
			}
			HandleRuntimeError("No handler registered for request method: " + Method);
			return false;
		}
		catch (const std::exception& e)
		{
			HandleRuntimeError("Error routing request: " + std::string(e.what()));
			return false;
		}
	}

	bool RouteResponse(const JSONData& InMessage)
	{
		try
		{
			const std::optional<RequestID> RequestID = ExtractRequestID(InMessage);

			if (!RequestID)
			{
				HandleRuntimeError("No request ID found in response");
				return false;
			}

			std::scoped_lock Lock(m_ResponseMutex);
			if (const auto Iter = m_ResponseHandlers.find(RequestID.value().ToString());
				Iter != m_ResponseHandlers.end())
			{
				Iter->second(InMessage);
				return true;
			}
			HandleRuntimeError("No handler registered for response ID: " + RequestID.value().ToString());
			return false;
		}
		catch (const std::exception& e)
		{
			HandleRuntimeError("Error routing response: " + std::string(e.what()));
			return false;
		}
	}

	bool RouteNotification(const JSONData& InMessage)
	{
		try
		{
			const std::string Method = ExtractMethod(InMessage);

			std::scoped_lock lock(m_NotificationMutex);
			if (const auto Iter = m_NotificationHandlers.find(Method); Iter != m_NotificationHandlers.end())
			{
				Iter->second(InMessage);
				return true;
			}
			HandleRuntimeError("No handler registered for notification method: " + Method);
			return false;
		}
		catch (const std::exception& e)
		{
			HandleRuntimeError("Error routing notification: " + std::string(e.what()));
			return false;
		}
	}

	std::unordered_map<std::string, MessageHandler> m_RequestHandlers;
	std::unordered_map<std::string, MessageHandler> m_ResponseHandlers;
	std::unordered_map<std::string, MessageHandler> m_NotificationHandlers;

	// Mutexes for thread safety
	std::mutex m_RequestMutex;
	std::mutex m_ResponseMutex;
	std::mutex m_NotificationMutex;

	bool m_WarnOnDuplicateHandlers{ true };

	explicit MessageManager(const bool InWarnOnDuplicateHandlers) : m_WarnOnDuplicateHandlers(InWarnOnDuplicateHandlers)
	{}
};

MCP_NAMESPACE_END