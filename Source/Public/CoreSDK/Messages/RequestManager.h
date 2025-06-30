#pragma once

#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Messages/RequestBase.h"

// Forward declarations
class MCPContext;

MCP_NAMESPACE_BEGIN

/**
 * Routes incoming MCP request messages to registered handlers.
 * Servers register handlers for specific request types (e.g., InitializeRequest) during
 * construction. When messages arrive from transport, they are routed to the appropriate handler.
 */
class RequestManager {
  public:
    using RequestHandlerFunction =
        std::function<void(const RequestBase& InRequest, std::optional<MCPContext*> InContext)>;

    /**
     * Constructor
     * @param InWarnOnDuplicateHandlers Whether to warn when duplicate handlers are registered
     */
    explicit RequestManager(bool InWarnOnDuplicateHandlers = true);

    /**
     * Register a request handler for a specific request method.
     * @param InMethod The request method name (e.g., "initialize", "tools/list")
     * @param InHandler The handler function to execute
     * @return True if successfully registered, false if method already exists
     */
    bool RegisterRequestHandler(std::string_view InMethod, RequestHandlerFunction InHandler);

    /**
     * Unregister a request handler.
     * @param InMethod The request method name to unregister
     * @return True if successfully unregistered, false if method didn't exist
     */
    bool UnregisterRequestHandler(std::string_view InMethod);

    /**
     * Route an incoming request to the appropriate handler.
     * @param InRequest The request to route
     * @param InContext Optional context for the request handling
     * @return True if routed successfully, false if no handler found
     */
    bool RouteRequest(const RequestBase& InRequest,
                      std::optional<MCPContext*> InContext = std::nullopt);

    /**
     * Check if a handler is registered for a request method.
     * @param InMethod The method name to check
     * @return True if handler exists, false otherwise
     */
    bool HasRequestHandler(std::string_view InMethod) const;

    /**
     * Get the handler for a specific request method.
     * @param InMethod The method name
     * @return The handler function if found, nullopt otherwise
     */
    std::optional<RequestHandlerFunction> GetRequestHandler(std::string_view InMethod) const;

    /**
     * List all registered methods.
     * @return Vector containing all registered method names
     */
    std::vector<std::string> ListRegisteredMethods() const;

    /**
     * Clear all registered handlers.
     */
    void ClearRegisteredHandlers();

    /**
     * Get the number of registered handlers.
     * @return Number of registered handlers
     */
    size_t GetHandlerCount() const;

  private:
    std::unordered_map<std::string, RequestHandlerFunction> m_RequestHandlers;
    bool m_WarnOnDuplicateHandlers;
    mutable std::mutex m_RequestHandlersMutex;

  public:
    template <ConcreteRequest T, typename Function> void RegisterHandler(Function&& InHandler) {
        static_assert(std::is_invocable_v<Function, const T&, std::optional<MCPContext*>>,
                      "Handler must be callable with '(const ConcreteMessageType&, MCPContext*)' ");

        std::scoped_lock lock(m_RequestHandlersMutex);

        m_RequestHandlers[T::Method()] = [Handler = std::forward<Function>(InHandler)](
                                             const MessageBase& Message, MCPContext* Context) {
            Handler(static_cast<const T&>(Message), Context);
        };
    }

    // Dispatch a received message to its registered handler
    template <ConcreteRequest T> void RouteRequest(const T& InRequest, MCPContext* InContext) {
        auto FoundHandler = m_RequestHandlers.find(T::Method());
        if (FoundHandler != m_RequestHandlers.end()) { FoundHandler->second(InRequest, InContext); }
    }
};

MCP_NAMESPACE_END
