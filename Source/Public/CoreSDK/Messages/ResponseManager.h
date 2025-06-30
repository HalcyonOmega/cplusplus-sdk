#pragma once

#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "CoreSDK/Messages/ResponseBase.h"

// Forward declarations
class MCPContext;

MCP_NAMESPACE_BEGIN

/**
 * Routes incoming MCP response messages to pending request handlers.
 * Tracks pending requests by request ID and routes responses when they arrive.
 * Used for handling responses to outgoing requests.
 */
class ResponseManager {
  public:
    using ResponseHandlerFunction =
        std::function<void(const ResponseBase& InResponse, MCPContext* InContext)>;

    /**
     * Constructor
     * @param InWarnOnDuplicateHandlers Whether to warn when duplicate handlers are registered
     */
    explicit ResponseManager(bool InWarnOnDuplicateHandlers = true);

    /**
     * Register a pending request handler for a specific request ID.
     * @param InRequestID The request ID to track
     * @param InHandler The handler function to execute when response is received
     * @return True if successfully registered, false if ID already exists
     */
    bool RegisterPendingRequest(const RequestID& InRequestID, ResponseHandlerFunction InHandler);

    /**
     * Unregister a pending request handler.
     * @param InRequestID The request ID to unregister
     * @return True if successfully unregistered, false if ID didn't exist
     */
    bool UnregisterPendingRequest(const RequestID& InRequestID);

    /**
     * Route an incoming response to the appropriate pending request handler.
     * @param InResponse The response to route
     * @param InContext Optional context for the response handling
     * @return True if routed successfully, false if no pending request found
     */
    bool RouteResponse(const ResponseBase& InResponse,
                       std::optional<MCPContext*> InContext = std::nullopt);

    /**
     * Check if a pending request handler is registered.
     * @param InRequestID The request ID to check
     * @return True if pending request exists, false otherwise
     */
    bool HasPendingRequest(const RequestID& InRequestID) const;

    /**
     * Get the handler for a specific pending request.
     * @param InRequestID The request ID
     * @return The handler function if found, nullopt otherwise
     */
    std::optional<ResponseHandlerFunction>
    GetPendingRequestHandler(const RequestID& InRequestID) const;

    /**
     * List all pending request IDs.
     * @return Vector containing all pending request IDs
     */
    std::vector<RequestID> ListPendingRequests() const;

    /**
     * Clear all pending request handlers.
     */
    void ClearRegisteredHandlers();

    /**
     * Get the number of pending request handlers.
     * @return Number of pending request handlers
     */
    size_t GetHandlerCount() const;

  private:
    std::unordered_map<std::string, ResponseHandlerFunction>
        m_PendingRequestHandlers; // Request ID -> Handler
    bool m_WarnOnDuplicateHandlers;
    mutable std::mutex m_PendingRequestsMutex;

  public:
    template <ConcreteResponse T, typename Function> void RegisterHandler(Function&& InHandler) {
        static_assert(std::is_invocable_v<Function, const T&, std::optional<MCPContext*>>,
                      "Handler must be callable with '(const ConcreteMessageType&, MCPContext*)' ");

        std::scoped_lock lock(m_PendingRequestsMutex);
    }
};

MCP_NAMESPACE_END
