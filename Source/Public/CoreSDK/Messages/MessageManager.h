#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

#include "CoreSDK/Common/MCPContext.h"
#include "CoreSDK/Common/RuntimeError.h"
#include "CoreSDK/Messages/ErrorResponseBase.h"
#include "CoreSDK/Messages/NotificationBase.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "CoreSDK/Messages/ResponseBase.h"
#include "JSONProxy.h"
#include "Utilities/JSON/JSONMessages.h"

MCP_NAMESPACE_BEGIN

class MessageManager {
  public:
    using MessageHandler = std::function<void(const JSONValue&, std::optional<MCPContext*>)>;

    // Register handlers for specific concrete message types
    template <ConcreteRequest T, typename Function>
    bool RegisterRequestHandler(Function&& InHandler) {
        static_assert(std::is_invocable_v<Function, const T&, std::optional<MCPContext*>>,
                      "Handler must be callable with '(const ConcreteRequestType&, "
                      "std::optional<MCPContext*>)'");

        std::scoped_lock lock(m_RequestMutex);

        if (m_WarnOnDuplicateHandlers && m_RequestHandlers.count(T::Method)) {
            HandleRuntimeError("Request handler already exists for message: "
                               + std::string(T::Method));
            return false;
        }

        m_RequestHandlers[T::Method] =
            [Handler = std::forward<Function>(InHandler)](const JSONValue& InMessage,
                                                          std::optional<MCPContext*> InContext) {
                JSONValue JSONRequest = InMessage;
                T Request = JSONRequest.get<T>();
                Handler(Request, InContext);
            };

        return true;
    }

    template <ConcreteResponse T, typename Function>
    bool RegisterResponseHandler(Function&& InHandler) {
        static_assert(std::is_invocable_v<Function, const T&, std::optional<MCPContext*>>,
                      "Handler must be callable with '(const ConcreteResponseType&, "
                      "std::optional<MCPContext*>)'");

        std::scoped_lock lock(m_ResponseMutex);

        if (m_WarnOnDuplicateHandlers && m_ResponseHandlers.count(T::ID.ToString())) {
            HandleRuntimeError("Response handler already exists for message: "
                               + std::string(T::ID.ToString()));
            return false;
        }

        m_ResponseHandlers[T::ID.ToString()] =
            [Handler = std::forward<Function>(InHandler)](const JSONValue& InMessage,
                                                          std::optional<MCPContext*> InContext) {
                JSONValue JSONResponse = InMessage;
                T Response = JSONResponse.get<T>();
                Handler(Response, InContext);
            };

        return true;
    }

    template <ConcreteNotification T, typename Function>
    bool RegisterNotificationHandler(Function&& InHandler) {
        static_assert(std::is_invocable_v<Function, const T&, std::optional<MCPContext*>>,
                      "Handler must be callable with '(const ConcreteNotificationType&, "
                      "std::optional<MCPContext*>)'");

        std::scoped_lock lock(m_NotificationMutex);

        if (m_WarnOnDuplicateHandlers && m_NotificationHandlers.count(T::Method)) {
            HandleRuntimeError("Notification handler already exists for message: "
                               + std::string(T::Method));
            return false;
        }

        m_NotificationHandlers[T::Method] =
            [Handler = std::forward<Function>(InHandler)](const JSONValue& InMessage,
                                                          std::optional<MCPContext*> InContext) {
                JSONValue JSONNotification = InMessage;
                T Notification = JSONNotification.get<T>();
                Handler(Notification, InContext);
            };

        return true;
    }

    template <ConcreteErrorResponse T, typename Function>
    bool RegisterErrorHandler(Function&& InHandler) {
        static_assert(std::is_invocable_v<Function, const T&, std::optional<MCPContext*>>,
                      "Handler must be callable with '(const ConcreteErrorResponseType&, "
                      "std::optional<MCPContext*>)'");

        std::scoped_lock lock(m_ErrorMutex);

        if (m_WarnOnDuplicateHandlers && m_ErrorHandlers.count(T::ID.ToString())) {
            HandleRuntimeError("Error handler already exists for message: "
                               + std::string(T::ID.ToString()));
            return false;
        }

        m_ErrorHandlers[T::ID.ToString()] =
            [Handler = std::forward<Function>(InHandler)](const JSONValue& InMessage,
                                                          std::optional<MCPContext*> InContext) {
                JSONValue JSONError = InMessage;
                T Error = JSONError.get<T>();
                Handler(Error, InContext);
            };

        return true;
    }

    // Main routing function - receives JSON string and routes to appropriate handler
    bool RouteMessage(const std::string& InMessage,
                      std::optional<MCPContext*> InContext = std::nullopt) {
        try {
            JSONValue Message = JSONValue::parse(InMessage);

            std::optional<MessageType> MessageType = GetValidMessageType(Message);

            if (MessageType) {
                switch (MessageType.value()) {
                    case MessageType::Request: return RouteRequest(Message, InContext);
                    case MessageType::Response: return RouteResponse(Message, InContext);
                    case MessageType::Error: return RouteError(Message, InContext);
                    case MessageType::Notification: return RouteNotification(Message, InContext);
                    default:
                        HandleRuntimeError("Invalid message type: "
                                           + std::to_string(static_cast<int>(MessageType.value())));
                        return false;
                }
            }

            HandleRuntimeError("Invalid JSON-RPC message received");
            return false;

        } catch (const std::exception& e) {
            HandleRuntimeError("Error parsing message: " + std::string(e.what()));
            return false;
        }
    }

  private:
    // Message type routing functions
    bool RouteRequest(const JSONValue& InMessage, std::optional<MCPContext*> InContext) {
        try {
            std::string method = ExtractMethod(InMessage);

            std::scoped_lock lock(m_RequestMutex);
            auto Iter = m_RequestHandlers.find(method);
            if (Iter != m_RequestHandlers.end()) {
                Iter->second(InMessage, InContext);
                return true;
            }
            HandleRuntimeError("No handler registered for request method: " + method);
            return false;

        } catch (const std::exception& e) {
            HandleRuntimeError("Error routing request: " + std::string(e.what()));
            return false;
        }
    }

    bool RouteResponse(const JSONValue& InMessage, std::optional<MCPContext*> InContext) {
        try {
            std::optional<RequestID> RequestID = ExtractRequestID(InMessage);

            if (!RequestID) {
                HandleRuntimeError("No request ID found in response");
                return false;
            }

            std::scoped_lock lock(m_ResponseMutex);
            auto Iter = m_ResponseHandlers.find(RequestID.value().ToString());
            if (Iter != m_ResponseHandlers.end()) {
                Iter->second(InMessage, InContext);
                return true;
            }
            HandleRuntimeError("No handler registered for response ID: "
                               + RequestID.value().ToString());
            return false;

        } catch (const std::exception& e) {
            HandleRuntimeError("Error routing response: " + std::string(e.what()));
            return false;
        }
    }

    bool RouteNotification(const JSONValue& InMessage, std::optional<MCPContext*> InContext) {
        try {
            std::string method = ExtractMethod(InMessage);

            std::scoped_lock lock(m_NotificationMutex);
            auto Iter = m_NotificationHandlers.find(method);
            if (Iter != m_NotificationHandlers.end()) {
                Iter->second(InMessage, InContext);
                return true;
            }
            HandleRuntimeError("No handler registered for notification method: " + method);
            return false;

        } catch (const std::exception& e) {
            HandleRuntimeError("Error routing notification: " + std::string(e.what()));
            return false;
        }
    }

    bool RouteError(const JSONValue& InMessage, std::optional<MCPContext*> InContext) {
        try {
            std::optional<RequestID> errorID = ExtractRequestID(InMessage);

            if (!errorID) {
                HandleRuntimeError("No request ID found in error");
                return false;
            }

            std::scoped_lock lock(m_ErrorMutex);
            auto Iter = m_ErrorHandlers.find(errorID.value().ToString());
            if (Iter != m_ErrorHandlers.end()) {
                Iter->second(InMessage, InContext);
                return true;
            }
            HandleRuntimeError("No handler registered for error ID: " + errorID.value().ToString());
            return false;

        } catch (const std::exception& e) {
            HandleRuntimeError("Error routing error: " + std::string(e.what()));
            return false;
        }
    }

    std::unordered_map<std::string, MessageHandler> m_RequestHandlers;
    std::unordered_map<std::string, MessageHandler> m_ResponseHandlers;
    std::unordered_map<std::string, MessageHandler> m_NotificationHandlers;
    std::unordered_map<std::string, MessageHandler> m_ErrorHandlers;

    // Mutexes for thread safety
    std::mutex m_RequestMutex;
    std::mutex m_ResponseMutex;
    std::mutex m_NotificationMutex;
    std::mutex m_ErrorMutex;

    bool m_WarnOnDuplicateHandlers = true;
};

MCP_NAMESPACE_END