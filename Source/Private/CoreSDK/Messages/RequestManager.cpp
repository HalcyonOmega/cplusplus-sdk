#include "CoreSDK/Messages/RequestManager.h"

#include <iostream>

MCP_NAMESPACE_BEGIN

RequestManager::RequestManager(bool InWarnOnDuplicateHandlers)
    : m_WarnOnDuplicateHandlers(InWarnOnDuplicateHandlers) {}

bool RequestManager::RegisterRequestHandler(const std::string& InMethod,
                                            RequestHandlerFunction InHandler) {
    std::lock_guard<std::mutex> lock(m_RequestHandlersMutex);

    if (m_RequestHandlers.find(InMethod) != m_RequestHandlers.end()) {
        if (m_WarnOnDuplicateHandlers) {
            std::cerr << "Warning: Duplicate request handler for method: " << InMethod << std::endl;
        }
        return false;
    }

    m_RequestHandlers[InMethod] = std::move(InHandler);
    return true;
}

bool RequestManager::UnregisterRequestHandler(const std::string& InMethod) {
    std::lock_guard<std::mutex> lock(m_RequestHandlersMutex);
    return m_RequestHandlers.erase(InMethod) > 0;
}

bool RequestManager::RouteRequest(const RequestBase& InRequest,
                                  std::optional<MCPContext*> InContext) {
    std::lock_guard<std::mutex> lock(m_RequestHandlersMutex);

    auto it = m_RequestHandlers.find(InRequest.Method);
    if (it != m_RequestHandlers.end()) { it->second(InRequest, InContext); }
}

bool RequestManager::HasRequestHandler(const std::string& InMethod) const {
    std::lock_guard<std::mutex> lock(m_RequestHandlersMutex);
    return m_RequestHandlers.find(InMethod) != m_RequestHandlers.end();
}

std::optional<RequestManager::RequestHandlerFunction>
RequestManager::GetRequestHandler(const std::string& InMethod) const {
    std::lock_guard<std::mutex> lock(m_RequestHandlersMutex);

    auto it = m_RequestHandlers.find(InMethod);
    if (it != m_RequestHandlers.end()) { return it->second; }
    return std::nullopt;
}

std::vector<std::string> RequestManager::ListRegisteredMethods() const {
    std::lock_guard<std::mutex> lock(m_RequestHandlersMutex);

    std::vector<std::string> methods;
    methods.reserve(m_RequestHandlers.size());

    for (const auto& pair : m_RequestHandlers) { methods.push_back(pair.first); }

    return methods;
}

void RequestManager::ClearRegisteredHandlers() {
    std::lock_guard<std::mutex> lock(m_RequestHandlersMutex);
    m_RequestHandlers.clear();
}

size_t RequestManager::GetHandlerCount() const {
    std::lock_guard<std::mutex> lock(m_RequestHandlersMutex);
    return m_RequestHandlers.size();
}

MCP_NAMESPACE_END