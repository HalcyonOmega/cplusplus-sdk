#include "CoreSDK/Features/RootManager.h"

#include <algorithm>

#include "CoreSDK/Common/Logging.h"

MCP_NAMESPACE_BEGIN

RootManager::RootManager(bool InWarnOnDuplicateRoots)
    : m_WarnOnDuplicateRoots(InWarnOnDuplicateRoots) {}

Root RootManager::AddRoot(const Root& InRoot) {
    MCP::Logger::Debug("Adding root - URI: " + InRoot.URI.toString()
                       + ", Name: " + InRoot.Name.value_or("(unnamed)"));
    std::lock_guard<std::mutex> Lock(m_RootsMutex);

    const std::string Key = GetRootKey(InRoot.URI);
    const auto ExistingIt = m_Roots.find(Key);
    if (ExistingIt != m_Roots.end()) {
        if (m_WarnOnDuplicateRoots) {
            MCP::Logger::Warning("Root already exists: " + InRoot.URI.toString());
        }
        return ExistingIt->second;
    }

    m_Roots[Key] = InRoot;
    return InRoot;
}

Root RootManager::AddRoot(const MCP::URIFile& InURI, const std::optional<std::string>& InName) {
    if (!IsValidRootURI(InURI)) {
        throw RootError("Invalid root URI: " + InURI.toString() + " (must start with file://)");
    }

    return AddRoot(CreateRoot(InURI, InName));
}

void RootManager::RemoveRoot(const Root& InRoot) {
    RemoveRoot(InRoot.URI);
}

void RootManager::RemoveRoot(const MCP::URIFile& InURI) {
    MCP::Logger::Debug("Removing root: " + InURI.toString());
    std::lock_guard<std::mutex> Lock(m_RootsMutex);

    const std::string Key = GetRootKey(InURI);
    const auto Iter = m_Roots.find(Key);
    if (Iter != m_Roots.end()) { m_Roots.erase(Iter); }
}

std::optional<Root> RootManager::GetRoot(const MCP::URIFile& InURI) const {
    std::lock_guard<std::mutex> Lock(m_RootsMutex);

    const std::string Key = GetRootKey(InURI);
    const auto Iter = m_Roots.find(Key);
    if (Iter != m_Roots.end()) { return Iter->second; }
    return std::nullopt;
}

std::optional<Root> RootManager::GetRootByName(const std::string& InName) const {
    std::lock_guard<std::mutex> Lock(m_RootsMutex);

    const auto Iter = std::find_if(m_Roots.begin(), m_Roots.end(), [&InName](const auto& Pair) {
        return Pair.second.Name.has_value() && Pair.second.Name.value() == InName;
    });

    if (Iter != m_Roots.end()) { return Iter->second; }
    return std::nullopt;
}

std::vector<Root> RootManager::ListRoots() const {
    std::lock_guard<std::mutex> Lock(m_RootsMutex);

    MCP::Logger::Debug("Listing roots - Count: " + std::to_string(m_Roots.size()));

    std::vector<Root> Result;
    Result.reserve(m_Roots.size());

    for (const auto& [Key, RootData] : m_Roots) { Result.push_back(RootData); }

    return Result;
}

bool RootManager::HasRoot(const MCP::URIFile& InURI) const {
    std::lock_guard<std::mutex> Lock(m_RootsMutex);

    const std::string Key = GetRootKey(InURI);
    return m_Roots.find(Key) != m_Roots.end();
}

bool RootManager::HasRootWithName(const std::string& InName) const {
    std::lock_guard<std::mutex> Lock(m_RootsMutex);

    return std::any_of(m_Roots.begin(), m_Roots.end(), [&InName](const auto& Pair) {
        return Pair.second.Name.has_value() && Pair.second.Name.value() == InName;
    });
}

void RootManager::ClearRoots() {
    std::lock_guard<std::mutex> Lock(m_RootsMutex);

    MCP::Logger::Debug("Clearing all roots - Count: " + std::to_string(m_Roots.size()));
    m_Roots.clear();
}

size_t RootManager::GetRootCount() const {
    std::lock_guard<std::mutex> Lock(m_RootsMutex);

    return m_Roots.size();
}

bool RootManager::IsValidRootURI(const MCP::URIFile& InURI) {
    const std::string URIString = InURI.toString();
    return URIString.starts_with("file://");
}

Root RootManager::CreateRoot(const MCP::URIFile& InURI, const std::optional<std::string>& InName) {
    if (!IsValidRootURI(InURI)) {
        throw RootError("Invalid root URI: " + InURI.toString() + " (must start with file://)");
    }

    Root NewRoot;
    NewRoot.URI = InURI;
    NewRoot.Name = InName;
    return NewRoot;
}

std::string RootManager::GetRootKey(const MCP::URIFile& InURI) {
    return InURI.toString();
}

MCP_NAMESPACE_END
