#pragma once

#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Messages/NotificationBase.h"

// Forward declarations
class MCPContext;

MCP_NAMESPACE_BEGIN

/**
 * Routes incoming MCP notification messages to registered handlers.
 * Servers register handlers for specific notification types during construction.
 * When notifications arrive from transport, they are routed to the appropriate handler.
 */
class NotificationManager {
  public:
    using NotificationHandlerFunction =
        std::function<void(const NotificationBase& InNotification, MCPContext* InContext)>;

    /**
     * Constructor
     * @param InWarnOnDuplicateHandlers Whether to warn when duplicate handlers are registered
     */
    explicit NotificationManager(bool InWarnOnDuplicateHandlers = true);

    /**
     * Register a notification handler for a specific notification method.
     * @param InMethod The notification method name (e.g., "notifications/initialized",
     * "notifications/progress")
     * @param InHandler The handler function to execute
     * @return True if successfully registered, false if method already exists
     */
    bool RegisterNotificationHandler(const std::string& InMethod,
                                     NotificationHandlerFunction InHandler);

    /**
     * Unregister a notification handler.
     * @param InMethod The notification method name to unregister
     * @return True if successfully unregistered, false if method didn't exist
     */
    bool UnregisterNotificationHandler(const std::string& InMethod);

    /**
     * Route an incoming notification to the appropriate handler.
     * @param InNotification The notification to route
     * @param InContext Optional context for the notification handling
     * @return True if routed successfully, false if no handler found
     */
    bool RouteNotification(const NotificationBase& InNotification,
                           std::optional<MCPContext*> InContext = std::nullopt);

    /**
     * Check if a handler is registered for a notification method.
     * @param InMethod The method name to check
     * @return True if handler exists, false otherwise
     */
    bool HasNotificationHandler(const std::string& InMethod) const;

    /**
     * Get the handler for a specific notification method.
     * @param InMethod The method name
     * @return The handler function if found, nullopt otherwise
     */
    std::optional<NotificationHandlerFunction>
    GetNotificationHandler(const std::string& InMethod) const;

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
    std::unordered_map<std::string, NotificationHandlerFunction> m_NotificationHandlers;
    bool m_WarnOnDuplicateHandlers;
    mutable std::mutex m_NotificationHandlersMutex;
};

MCP_NAMESPACE_END
