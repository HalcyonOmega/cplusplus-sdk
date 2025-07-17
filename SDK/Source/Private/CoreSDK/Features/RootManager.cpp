#include "CoreSDK/Features/RootManager.h"

#include <algorithm>

#include "CoreSDK/Common/Logging.h"
#include "CoreSDK/Common/RuntimeError.h"

MCP_NAMESPACE_BEGIN

RootManager::RootManager(const bool InWarnOnDuplicateRoots) : m_WarnOnDuplicateRoots(InWarnOnDuplicateRoots) {}

bool RootManager::AddRoot(const Root& InRoot)
{
	std::lock_guard Lock(m_RootsMutex);

	const std::string Key = GetRootKey(InRoot.URI);
	if (const auto ExistingIt = m_Roots.find(Key); ExistingIt != m_Roots.end())
	{
		if (m_WarnOnDuplicateRoots)
		{
			HandleRuntimeError("Root already exists: " + InRoot.URI.toString());
		}
		ExistingIt->second;
		return false;
	}

	m_Roots[Key] = InRoot;
	return true;
}

bool RootManager::AddRoot(const MCP::URIFile& InURI, const std::optional<std::string>& InName)
{
	if (!IsValidRootURI(InURI))
	{
		throw RootError("Invalid root URI: " + InURI.toString() + " (must start with file://)");
	}

	return AddRoot(CreateRoot(InURI, InName));
}

bool RootManager::RemoveRoot(const Root& InRoot) { return RemoveRoot(InRoot.URI); }

bool RootManager::RemoveRoot(const MCP::URIFile& InURI)
{
	std::lock_guard Lock(m_RootsMutex);

	const std::string Key = GetRootKey(InURI);
	if (const auto Iter = m_Roots.find(Key); Iter != m_Roots.end())
	{
		m_Roots.erase(Iter);
		return true;
	}
	return false;
}

std::optional<Root> RootManager::GetRoot(const MCP::URIFile& InURI) const
{
	std::lock_guard Lock(m_RootsMutex);

	const std::string Key = GetRootKey(InURI);
	if (const auto Iter = m_Roots.find(Key); Iter != m_Roots.end())
	{
		return Iter->second;
	}
	return std::nullopt;
}

std::optional<Root> RootManager::GetRootByName(const std::string& InName) const
{
	std::lock_guard Lock(m_RootsMutex);

	const auto Iter = std::ranges::find_if(m_Roots,
		[&InName](const auto& Pair) { return Pair.second.Name.has_value() && Pair.second.Name.value() == InName; });

	if (Iter != m_Roots.end())
	{
		return Iter->second;
	}
	return std::nullopt;
}

ListRootsResponse::Result RootManager::ListRoots() const
{
	std::lock_guard Lock(m_RootsMutex);

	std::vector<Root> Roots;
	Roots.reserve(m_Roots.size());

	for (const auto& RootData : m_Roots | std::views::values)
	{
		Roots.push_back(RootData);
	}

	return ListRootsResponse::Result{ Roots };
}

bool RootManager::HasRoot(const MCP::URIFile& InURI) const
{
	std::lock_guard Lock(m_RootsMutex);

	const std::string Key = GetRootKey(InURI);
	return m_Roots.contains(Key);
}

bool RootManager::HasRootWithName(const std::string& InName) const
{
	std::lock_guard Lock(m_RootsMutex);

	return std::ranges::any_of(m_Roots,
		[&InName](const auto& Pair) { return Pair.second.Name.has_value() && Pair.second.Name.value() == InName; });
}

void RootManager::ClearRoots()
{
	std::lock_guard Lock(m_RootsMutex);
	m_Roots.clear();
}

size_t RootManager::GetRootCount() const
{
	std::lock_guard Lock(m_RootsMutex);
	return m_Roots.size();
}

bool RootManager::IsValidRootURI(const MCP::URIFile& InURI)
{
	const std::string URIString = InURI.toString();
	return URIString.starts_with("file://");
}

Root RootManager::CreateRoot(const MCP::URIFile& InURI, const std::optional<std::string>& InName)
{
	if (!IsValidRootURI(InURI))
	{
		throw RootError("Invalid root URI: " + InURI.toString() + " (must start with file://)");
	}

	Root NewRoot;
	NewRoot.URI = InURI;
	NewRoot.Name = InName;
	return NewRoot;
}

std::string RootManager::GetRootKey(const MCP::URIFile& InURI) { return InURI.toString(); }

MCP_NAMESPACE_END
