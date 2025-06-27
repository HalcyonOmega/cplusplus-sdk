#include "CoreSDK/Messages/ErrorManager.h"

#include <iostream>

MCP_NAMESPACE_BEGIN

ErrorManager::ErrorManager(bool InWarnOnDuplicateHandlers)
    : m_WarnOnDuplicateHandlers(InWarnOnDuplicateHandlers) {}

bool ErrorManager::RegisterRequestErrorHandler(const RequestID& InRequestID,
                                               ErrorResponseHandlerFunction InHandler) {
    std::lock_guard<std::mutex> lock(m_ErrorHandlersMutex);

    std::string requestIDStr = InRequestID.ToString();
    if (m_RequestErrorHandlers.find(requestIDStr) != m_RequestErrorHandlers.end()) {
        if (m_WarnOnDuplicateHandlers) {
            std::cerr << "Warning: Duplicate error handler for request ID: " << requestIDStr
                      << std::endl;
        }
        return false;
    }

    m_RequestErrorHandlers[requestIDStr] = std::move(InHandler);
    return true;
}

bool ErrorManager::RegisterErrorCodeHandler(int64_t InErrorCode,
                                            ErrorResponseHandlerFunction InHandler) {
    std::lock_guard<std::mutex> lock(m_ErrorHandlersMutex);

    if (m_ErrorCodeHandlers.find(InErrorCode) != m_ErrorCodeHandlers.end()) {
        if (m_WarnOnDuplicateHandlers) {
            std::cerr << "Warning: Duplicate error handler for error code: " << InErrorCode
                      << std::endl;
        }
        return false;
    }

    m_ErrorCodeHandlers[InErrorCode] = std::move(InHandler);
    return true;
}

void ErrorManager::RegisterDefaultErrorHandler(ErrorResponseHandlerFunction InHandler) {
    std::lock_guard<std::mutex> lock(m_ErrorHandlersMutex);
    m_DefaultErrorHandler = std::move(InHandler);
}

bool ErrorManager::UnregisterRequestErrorHandler(const RequestID& InRequestID) {
    std::lock_guard<std::mutex> lock(m_ErrorHandlersMutex);
    return m_RequestErrorHandlers.erase(InRequestID.ToString()) > 0;
}

bool ErrorManager::UnregisterErrorCodeHandler(int64_t InErrorCode) {
    std::lock_guard<std::mutex> lock(m_ErrorHandlersMutex);
    return m_ErrorCodeHandlers.erase(InErrorCode) > 0;
}

void ErrorManager::ClearDefaultErrorHandler() {
    std::lock_guard<std::mutex> lock(m_ErrorHandlersMutex);
    m_DefaultErrorHandler.reset();
}

bool ErrorManager::RouteError(const ErrorResponseBase& InError,
                              std::optional<MCPContext*> InContext) {
    std::lock_guard<std::mutex> lock(m_ErrorHandlersMutex);

    // 1. Try request-specific error handler first
    std::string requestIDStr = InError.ID.ToString();
    auto requestIter = m_RequestErrorHandlers.find(requestIDStr);
    if (requestIter != m_RequestErrorHandlers.end()) {
        requestIter->second(InError, InContext);
        // Remove the handler after calling it (one-time use)
        m_RequestErrorHandlers.erase(requestIter);
        return;
    }

    // 2. Try error code handler
    auto codeIter = m_ErrorCodeHandlers.find(InError.Error.Code);
    if (codeIter != m_ErrorCodeHandlers.end()) {
        codeIter->second(InError, InContext);
        return;
    }

    // 3. Try default error handler
    if (m_DefaultErrorHandler.has_value()) { m_DefaultErrorHandler.value()(InError, InContext); }
}

bool ErrorManager::HasRequestErrorHandler(const RequestID& InRequestID) const {
    std::lock_guard<std::mutex> lock(m_ErrorHandlersMutex);
    return m_RequestErrorHandlers.find(InRequestID.ToString()) != m_RequestErrorHandlers.end();
}

bool ErrorManager::HasErrorCodeHandler(int64_t InErrorCode) const {
    std::lock_guard<std::mutex> lock(m_ErrorHandlersMutex);
    return m_ErrorCodeHandlers.find(InErrorCode) != m_ErrorCodeHandlers.end();
}

bool ErrorManager::HasDefaultErrorHandler() const {
    std::lock_guard<std::mutex> lock(m_ErrorHandlersMutex);
    return m_DefaultErrorHandler.has_value();
}

std::optional<ErrorManager::ErrorResponseHandlerFunction>
ErrorManager::GetRequestErrorHandler(const RequestID& InRequestID) const {
    std::lock_guard<std::mutex> lock(m_ErrorHandlersMutex);

    auto it = m_RequestErrorHandlers.find(InRequestID.ToString());
    if (it != m_RequestErrorHandlers.end()) { return it->second; }
    return std::nullopt;
}

std::optional<ErrorManager::ErrorResponseHandlerFunction>
ErrorManager::GetErrorCodeHandler(int64_t InErrorCode) const {
    std::lock_guard<std::mutex> lock(m_ErrorHandlersMutex);

    auto it = m_ErrorCodeHandlers.find(InErrorCode);
    if (it != m_ErrorCodeHandlers.end()) { return it->second; }
    return std::nullopt;
}

std::optional<ErrorManager::ErrorResponseHandlerFunction>
ErrorManager::GetDefaultErrorHandler() const {
    std::lock_guard<std::mutex> lock(m_ErrorHandlersMutex);
    return m_DefaultErrorHandler;
}

std::vector<RequestID> ErrorManager::ListRequestsWithErrorHandlers() const {
    std::lock_guard<std::mutex> lock(m_ErrorHandlersMutex);

    std::vector<RequestID> requestIDs;
    requestIDs.reserve(m_RequestErrorHandlers.size());

    for (const auto& pair : m_RequestErrorHandlers) { requestIDs.emplace_back(pair.first); }

    return requestIDs;
}

std::vector<int64_t> ErrorManager::ListRegisteredErrorCodes() const {
    std::lock_guard<std::mutex> lock(m_ErrorHandlersMutex);

    std::vector<int64_t> errorCodes;
    errorCodes.reserve(m_ErrorCodeHandlers.size());

    for (const auto& pair : m_ErrorCodeHandlers) { errorCodes.push_back(pair.first); }

    return errorCodes;
}

void ErrorManager::ClearRegisteredHandlers() {
    std::lock_guard<std::mutex> lock(m_ErrorHandlersMutex);
    m_RequestErrorHandlers.clear();
    m_ErrorCodeHandlers.clear();
    m_DefaultErrorHandler.reset();
}

size_t ErrorManager::GetRequestErrorHandlerCount() const {
    std::lock_guard<std::mutex> lock(m_ErrorHandlersMutex);
    return m_RequestErrorHandlers.size();
}

size_t ErrorManager::GetErrorCodeHandlerCount() const {
    std::lock_guard<std::mutex> lock(m_ErrorHandlersMutex);
    return m_ErrorCodeHandlers.size();
}

MCP_NAMESPACE_END