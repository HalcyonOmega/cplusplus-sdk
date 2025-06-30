#pragma once

#include <any>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

#include "CoreSDK/Common/MCPContext.h"
#include "CoreSDK/Common/RuntimeError.h"
#include "CoreSDK/Messages/ErrorResponseBase.h"
#include "CoreSDK/Messages/MessageBase.h"
#include "CoreSDK/Messages/NotificationBase.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "CoreSDK/Messages/ResponseBase.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

class MessageManager {
  public:
    // Handler function signatures for different message types
    template <typename T>
    using RequestHandlerFunction = std::function<void(const T&, std::optional<MCPContext*>)>;

    template <typename T>
    using ResponseHandlerFunction = std::function<void(const T&, std::optional<MCPContext*>)>;

    template <typename T>
    using NotificationHandlerFunction = std::function<void(const T&, std::optional<MCPContext*>)>;

    template <typename T>
    using ErrorHandlerFunction = std::function<void(const T&, std::optional<MCPContext*>)>;

    // Register handlers for specific concrete message types
    template <ConcreteRequest T, typename Function>
    bool RegisterRequestHandler(Function&& InHandler) {
        static_assert(std::is_invocable_v<Function, const T&, std::optional<MCPContext*>>,
                      "Handler must be callable with '(const ConcreteRequestType&, "
                      "std::optional<MCPContext*>)'");

        std::scoped_lock lock(m_RequestMutex);

        if (m_WarnOnDuplicateHandlers && m_RequestHandlers.count(T::MessageName)) {
            HandleRuntimeError("Request handler already exists for message: "
                               + std::string(T::MessageName));
            return false;
        }

        m_RequestHandlers[T::MessageName] =
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

        if (m_WarnOnDuplicateHandlers && m_ResponseHandlers.count(T::MessageName)) {
            HandleRuntimeError("Response handler already exists for message: "
                               + std::string(T::MessageName));
            return false;
        }

        m_ResponseHandlers[T::MessageName] =
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

        if (m_WarnOnDuplicateHandlers && m_NotificationHandlers.count(T::MessageName)) {
            HandleRuntimeError("Notification handler already exists for message: "
                               + std::string(T::MessageName));
            return false;
        }

        m_NotificationHandlers[T::MessageName] =
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

        if (m_WarnOnDuplicateHandlers && m_ErrorHandlers.count(T::MessageName)) {
            HandleRuntimeError("Error handler already exists for message: "
                               + std::string(T::MessageName));
            return false;
        }

        m_ErrorHandlers[T::MessageName] =
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

            if (!IsValidJSONRPC(Message)) {
                HandleRuntimeError("Invalid JSON-RPC message received");
                return false;
            }

            // Determine message type and route accordingly
            if (Message.contains("method")) {
                if (Message.contains("id")) {
                    // Request message - has both method and id
                    return RouteRequest(Message, InContext);
                } else {
                    // Notification message - has method but no id
                    return RouteNotification(Message, InContext);
                }
            } else if (Message.contains("id")) {
                if (Message.contains("result")) {
                    // Response message - has id and result
                    return RouteResponse(Message, InContext);
                } else if (Message.contains("error")) {
                    // Error response - has id and error
                    return RouteError(Message, InContext);
                } else {
                    HandleRuntimeError("Invalid message: has id but neither result nor error");
                    return false;
                }
            } else {
                HandleRuntimeError("Invalid message: missing required fields");
                return false;
            }

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
            auto it = m_RequestHandlers.find(method);
            if (it != m_RequestHandlers.end()) {
                it->second(InMessage, InContext);
                return true;
            } else {
                HandleRuntimeError("No handler registered for request method: " + method);
                return false;
            }
        } catch (const std::exception& e) {
            HandleRuntimeError("Error routing request: " + std::string(e.what()));
            return false;
        }
    }

    bool RouteResponse(const JSONValue& InMessage, std::optional<MCPContext*> InContext) {
        try {
            // For responses, we need to determine the response type
            // This could be based on the original request or other metadata
            // For now, we'll use a generic approach
            std::string responseType = DetermineResponseType(InMessage);

            std::scoped_lock lock(m_ResponseMutex);
            auto it = m_ResponseHandlers.find(responseType);
            if (it != m_ResponseHandlers.end()) {
                it->second(InMessage, InContext);
                return true;
            } else {
                HandleRuntimeError("No handler registered for response type: " + responseType);
                return false;
            }
        } catch (const std::exception& e) {
            HandleRuntimeError("Error routing response: " + std::string(e.what()));
            return false;
        }
    }

    bool RouteNotification(const JSONValue& InMessage, std::optional<MCPContext*> InContext) {
        try {
            std::string method = ExtractMethod(InMessage);

            std::scoped_lock lock(m_NotificationMutex);
            auto it = m_NotificationHandlers.find(method);
            if (it != m_NotificationHandlers.end()) {
                it->second(InMessage, InContext);
                return true;
            } else {
                HandleRuntimeError("No handler registered for notification method: " + method);
                return false;
            }
        } catch (const std::exception& e) {
            HandleRuntimeError("Error routing notification: " + std::string(e.what()));
            return false;
        }
    }

    bool RouteError(const JSONValue& InMessage, std::optional<MCPContext*> InContext) {
        try {
            std::string errorType = DetermineErrorType(InMessage);

            std::scoped_lock lock(m_ErrorMutex);
            auto it = m_ErrorHandlers.find(errorType);
            if (it != m_ErrorHandlers.end()) {
                it->second(InMessage, InContext);
                return true;
            } else {
                HandleRuntimeError("No handler registered for error type: " + errorType);
                return false;
            }
        } catch (const std::exception& e) {
            HandleRuntimeError("Error routing error: " + std::string(e.what()));
            return false;
        }
    }

    // Helper functions for JSON parsing and validation
    bool IsValidJSONRPC(const JSONValue& InMessage) const {
        return InMessage.is_object() && InMessage.contains("jsonrpc")
               && InMessage["jsonrpc"].get<std::string>() == "2.0";
    }

    std::string ExtractMethod(const JSONValue& InMessage) const {
        if (InMessage.contains("method") && InMessage["method"].is_string()) {
            return InMessage["method"].get<std::string>();
        }
        throw std::runtime_error("Message does not contain valid method field");
    }

    std::string ExtractRequestID(const JSONValue& InMessage) const {
        if (InMessage.contains("id")) {
            if (InMessage["id"].is_string()) {
                return InMessage["id"].get<std::string>();
            } else if (InMessage["id"].is_number()) {
                return std::to_string(InMessage["id"].get<int64_t>());
            }
        }
        throw std::runtime_error("Message does not contain valid id field");
    }

    JSONValue ExtractParams(const JSONValue& InMessage) const {
        if (InMessage.contains("params")) { return InMessage["params"]; }
        return JSONValue::object(); // Return empty object if no params
    }

    std::string DetermineResponseType(const JSONValue& InMessage) const {
        (void)InMessage; // Mark as unused for now
        // This is a simplified approach - in a real implementation, you might
        // track pending requests to determine the response type
        // For now, return a generic response type
        return "DefaultResponse";
    }

    std::string DetermineErrorType(const JSONValue& InMessage) const {
        (void)InMessage; // Mark as unused for now
        // This is a simplified approach - in a real implementation, you might
        // track pending requests to determine the error type
        // For now, return a generic error type
        return "DefaultErrorResponse";
    }

    // Handler storage with type erasure
    using RequestHandler = std::function<void(const JSONValue&, std::optional<MCPContext*>)>;
    using ResponseHandler = std::function<void(const JSONValue&, std::optional<MCPContext*>)>;
    using NotificationHandler = std::function<void(const JSONValue&, std::optional<MCPContext*>)>;
    using ErrorHandler = std::function<void(const JSONValue&, std::optional<MCPContext*>)>;

    std::unordered_map<std::string, RequestHandler> m_RequestHandlers;
    std::unordered_map<std::string, ResponseHandler> m_ResponseHandlers;
    std::unordered_map<std::string, NotificationHandler> m_NotificationHandlers;
    std::unordered_map<std::string, ErrorHandler> m_ErrorHandlers;

    // Mutexes for thread safety
    std::mutex m_RequestMutex;
    std::mutex m_ResponseMutex;
    std::mutex m_NotificationMutex;
    std::mutex m_ErrorMutex;

    bool m_WarnOnDuplicateHandlers = true;
};

MCP_NAMESPACE_END