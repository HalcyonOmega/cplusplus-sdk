#include "CoreSDK/Messages/ResponseManager.h"

#include <iostream>

MCP_NAMESPACE_BEGIN

ResponseManager::ResponseManager(bool InWarnOnDuplicateHandlers)
    : m_WarnOnDuplicateHandlers(InWarnOnDuplicateHandlers) {}

bool ResponseManager::RegisterPendingRequest(const RequestID& InRequestID,
                                             ResponseHandlerFunction InHandler) {
    std::lock_guard<std::mutex> lock(m_PendingRequestsMutex);

    std::string requestIDStr = InRequestID.ToString();
    if (m_PendingRequestHandlers.find(requestIDStr) != m_PendingRequestHandlers.end()) {
        if (m_WarnOnDuplicateHandlers) {
            std::cerr << "Warning: Duplicate response handler for request ID: " << requestIDStr
                      << std::endl;
        }
        return false;
    }

    m_PendingRequestHandlers[requestIDStr] = std::move(InHandler);
    return true;
}

bool ResponseManager::UnregisterPendingRequest(const RequestID& InRequestID) {
    std::lock_guard<std::mutex> lock(m_PendingRequestsMutex);
    return m_PendingRequestHandlers.erase(InRequestID.ToString()) > 0;
}

bool ResponseManager::RouteResponse(const ResponseBase& InResponse,
                                    std::optional<MCPContext*> InContext) {
    std::lock_guard<std::mutex> lock(m_PendingRequestsMutex);

    std::string requestIDStr = InResponse.ID.ToString();
    auto it = m_PendingRequestHandlers.find(requestIDStr);
    if (it != m_PendingRequestHandlers.end()) {
        it->second(InResponse, InContext);
        // Remove the handler after calling it (one-time use)
        m_PendingRequestHandlers.erase(it);
    }
}

bool ResponseManager::HasPendingRequest(const RequestID& InRequestID) const {
    std::lock_guard<std::mutex> lock(m_PendingRequestsMutex);
    return m_PendingRequestHandlers.find(InRequestID.ToString()) != m_PendingRequestHandlers.end();
}

std::optional<ResponseManager::ResponseHandlerFunction>
ResponseManager::GetPendingRequestHandler(const RequestID& InRequestID) const {
    std::lock_guard<std::mutex> lock(m_PendingRequestsMutex);

    auto it = m_PendingRequestHandlers.find(InRequestID.ToString());
    if (it != m_PendingRequestHandlers.end()) { return it->second; }
    return std::nullopt;
}

std::vector<RequestID> ResponseManager::ListPendingRequests() const {
    std::lock_guard<std::mutex> lock(m_PendingRequestsMutex);

    std::vector<RequestID> requestIDs;
    requestIDs.reserve(m_PendingRequestHandlers.size());

    for (const auto& pair : m_PendingRequestHandlers) { requestIDs.emplace_back(pair.first); }

    return requestIDs;
}

void ResponseManager::ClearRegisteredHandlers() {
    std::lock_guard<std::mutex> lock(m_PendingRequestsMutex);
    m_PendingRequestHandlers.clear();
}

size_t ResponseManager::GetHandlerCount() const {
    std::lock_guard<std::mutex> lock(m_PendingRequestsMutex);
    return m_PendingRequestHandlers.size();
}

MCP_NAMESPACE_END