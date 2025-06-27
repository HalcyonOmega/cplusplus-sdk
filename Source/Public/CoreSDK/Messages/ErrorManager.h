#pragma once

#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Messages/ErrorResponseBase.h"
#include "CoreSDK/Messages/RequestBase.h"

// Forward declarations
class MCPContext;

MCP_NAMESPACE_BEGIN

/**
 * Routes incoming MCP error response messages to appropriate handlers.
 * Supports request-specific error handlers, global error code handlers, and default error handling.
 * Used for handling error responses from outgoing requests and general error management.
 */
class ErrorManager {
  public:
    using ErrorResponseHandlerFunction =
        std::function<void(const ErrorResponseBase& InError, MCPContext* InContext)>;

    /**
     * Constructor
     * @param InWarnOnDuplicateHandlers Whether to warn when duplicate handlers are registered
     */
    explicit ErrorManager(bool InWarnOnDuplicateHandlers = true);

    /**
     * Register an error handler for a specific request ID.
     * @param InRequestID The request ID to track for errors
     * @param InHandler The handler function to execute when error response is received
     * @return True if successfully registered, false if ID already exists
     */
    bool RegisterRequestErrorHandler(const RequestID& InRequestID,
                                     ErrorResponseHandlerFunction InHandler);

    /**
     * Register a global error handler for a specific error code.
     * @param InErrorCode The error code to handle
     * @param InHandler The handler function to execute when error response is received
     * @return True if successfully registered, false if code already exists
     */
    bool RegisterErrorCodeHandler(int64_t InErrorCode, ErrorResponseHandlerFunction InHandler);

    /**
     * Register a default error handler (catches all unhandled errors).
     * @param InHandler The handler function to execute for unhandled errors
     */
    void RegisterDefaultErrorHandler(ErrorResponseHandlerFunction InHandler);

    /**
     * Unregister a request-specific error handler.
     * @param InRequestID The request ID to unregister
     * @return True if successfully unregistered, false if ID didn't exist
     */
    bool UnregisterRequestErrorHandler(const RequestID& InRequestID);

    /**
     * Unregister an error code handler.
     * @param InErrorCode The error code to unregister
     * @return True if successfully unregistered, false if code didn't exist
     */
    bool UnregisterErrorCodeHandler(int64_t InErrorCode);

    /**
     * Clear the default error handler.
     */
    void ClearDefaultErrorHandler();

    /**
     * Route an incoming error response to the appropriate handler.
     * @param InError The error response to route
     * @param InContext Optional context for the error handling
     * @return True if routed successfully, false if no handler found
     */
    bool RouteError(const ErrorResponseBase& InError,
                    std::optional<MCPContext*> InContext = std::nullopt);

    /**
     * Check if a request-specific error handler is registered.
     * @param InRequestID The request ID to check
     * @return True if handler exists, false otherwise
     */
    bool HasRequestErrorHandler(const RequestID& InRequestID) const;

    /**
     * Check if an error code handler is registered.
     * @param InErrorCode The error code to check
     * @return True if handler exists, false otherwise
     */
    bool HasErrorCodeHandler(int64_t InErrorCode) const;

    /**
     * Check if a default error handler is registered.
     * @return True if default handler exists, false otherwise
     */
    bool HasDefaultErrorHandler() const;

    /**
     * Get the request-specific error handler.
     * @param InRequestID The request ID
     * @return The handler function if found, nullopt otherwise
     */
    std::optional<ErrorResponseHandlerFunction>
    GetRequestErrorHandler(const RequestID& InRequestID) const;

    /**
     * Get the error code handler for a specific error code.
     * @param InErrorCode The error code
     * @return The handler function if found, nullopt otherwise
     */
    std::optional<ErrorResponseHandlerFunction> GetErrorCodeHandler(int64_t InErrorCode) const;

    /**
     * Get the default error handler.
     * @return The default handler function if set, nullopt otherwise
     */
    std::optional<ErrorResponseHandlerFunction> GetDefaultErrorHandler() const;

    /**
     * List all request IDs with error handlers.
     * @return Vector containing all registered request IDs
     */
    std::vector<RequestID> ListRequestsWithErrorHandlers() const;

    /**
     * List all registered error codes.
     * @return Vector containing all registered error codes
     */
    std::vector<int64_t> ListRegisteredErrorCodes() const;

    /**
     * Clear all registered handlers.
     */
    void ClearRegisteredHandlers();

    /**
     * Get the number of request-specific error handlers.
     * @return Number of request error handlers
     */
    size_t GetRequestErrorHandlerCount() const;

    /**
     * Get the number of registered error code handlers.
     * @return Number of error code handlers
     */
    size_t GetErrorCodeHandlerCount() const;

  private:
    std::unordered_map<std::string, ErrorResponseHandlerFunction>
        m_RequestErrorHandlers; // Request ID -> Handler
    std::unordered_map<int64_t, ErrorResponseHandlerFunction>
        m_ErrorCodeHandlers; // Error Code -> Handler
    std::optional<ErrorResponseHandlerFunction>
        m_DefaultErrorHandler; // Default handler for unhandled errors
    bool m_WarnOnDuplicateHandlers;
    mutable std::mutex m_ErrorHandlersMutex;
};

MCP_NAMESPACE_END
