#include "CoreSDK/Messages/NotificationManager.h"

#include <iostream>

MCP_NAMESPACE_BEGIN

NotificationManager::NotificationManager(bool InWarnOnDuplicateHandlers)
    : m_WarnOnDuplicateHandlers(InWarnOnDuplicateHandlers) {}

bool NotificationManager::RegisterNotificationHandler(const std::string& InMethod,
                                                      NotificationHandlerFunction InHandler) {
    std::lock_guard<std::mutex> lock(m_NotificationHandlersMutex);

    if (m_NotificationHandlers.find(InMethod) != m_NotificationHandlers.end()) {
        if (m_WarnOnDuplicateHandlers) {
            std::cerr << "Warning: Duplicate notification handler for method: " << InMethod
                      << std::endl;
        }
        return false;
    }

    m_NotificationHandlers[InMethod] = std::move(InHandler);
    return true;
}

bool NotificationManager::UnregisterNotificationHandler(const std::string& InMethod) {
    std::lock_guard<std::mutex> lock(m_NotificationHandlersMutex);
    return m_NotificationHandlers.erase(InMethod) > 0;
}

bool NotificationManager::RouteNotification(const NotificationBase& InNotification,
                                            std::optional<MCPContext*> InContext) {
    std::lock_guard<std::mutex> lock(m_NotificationHandlersMutex);

    auto it = m_NotificationHandlers.find(InNotification.Method);
    if (it != m_NotificationHandlers.end()) {
        it->second(InNotification, InContext);
        return true;
    }
    return false;
}

bool NotificationManager::HasNotificationHandler(const std::string& InMethod) const {
    std::lock_guard<std::mutex> lock(m_NotificationHandlersMutex);
    return m_NotificationHandlers.find(InMethod) != m_NotificationHandlers.end();
}

std::optional<NotificationManager::NotificationHandlerFunction>
NotificationManager::GetNotificationHandler(const std::string& InMethod) const {
    std::lock_guard<std::mutex> lock(m_NotificationHandlersMutex);

    auto it = m_NotificationHandlers.find(InMethod);
    if (it != m_NotificationHandlers.end()) { return it->second; }
    return std::nullopt;
}

std::vector<std::string> NotificationManager::ListRegisteredMethods() const {
    std::lock_guard<std::mutex> lock(m_NotificationHandlersMutex);

    std::vector<std::string> methods;
    methods.reserve(m_NotificationHandlers.size());

    for (const auto& pair : m_NotificationHandlers) { methods.push_back(pair.first); }

    return methods;
}

void NotificationManager::ClearRegisteredHandlers() {
    std::lock_guard<std::mutex> lock(m_NotificationHandlersMutex);
    m_NotificationHandlers.clear();
}

size_t NotificationManager::GetHandlerCount() const {
    std::lock_guard<std::mutex> lock(m_NotificationHandlersMutex);
    return m_NotificationHandlers.size();
}

MCP_NAMESPACE_END