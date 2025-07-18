#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Messages/MCPMessages.h"
#include "RootBase.h"

MCP_NAMESPACE_BEGIN

/**
 * Exception thrown when root operations fail.
 */
class RootError final : public std::runtime_error
{
public:
	explicit RootError(const std::string& InMessage) : std::runtime_error(InMessage) {}
};

/**
 * Manages MCP roots.
 * Provides functionality for registering, retrieving, and listing root directories and files.
 */
class RootManager
{
public:
	/**
	 * Constructor
	 * @param InWarnOnDuplicateRoots Whether to warn when duplicate roots are added
	 */
	explicit RootManager(bool InWarnOnDuplicateRoots = true);

	/**
	 * Add a root to the manager.
	 * @param InRoot The root to add
	 * @return The added root. If a root with the same URI already exists, returns the existing
	 * root.
	 */
	bool AddRoot(const Root& InRoot);

	/**
	 * Add a root with URI and optional name.
	 * @param InURI The URI for the root (must start with file://)
	 * @param InName Optional human-readable name for the root
	 * @return The added root
	 */
	bool AddRoot(const MCP::URIFile& InURI, const std::optional<std::string>& InName = std::nullopt);

	/**
	 * Remove a root from the manager.
	 * @param InRoot The root to remove
	 */
	bool RemoveRoot(const Root& InRoot);

	/**
	 * Remove a root by URI.
	 * @param InURI The URI of the root to remove
	 */
	bool RemoveRoot(const MCP::URIFile& InURI);

	/**
	 * Get root by URI.
	 * @param InURI The URI of the root to retrieve
	 * @return The root if found, nullopt otherwise
	 */
	std::optional<Root> GetRoot(const MCP::URIFile& InURI) const;

	/**
	 * Get root by name.
	 * @param InName The name of the root to retrieve
	 * @return The root if found, nullopt otherwise
	 */
	std::optional<Root> GetRootByName(const std::string& InName) const;

	/**
	 * List all registered roots.
	 * @return Vector containing all registered roots
	 */
	ListRootsResponse::Result ListRoots() const;

	/**
	 * Check if a root with the given URI exists.
	 * @param InURI The URI to check
	 * @return True if the root exists, false otherwise
	 */
	bool HasRoot(const MCP::URIFile& InURI) const;

	/**
	 * Check if a root with the given name exists.
	 * @param InName The name to check
	 * @return True if a root with this name exists, false otherwise
	 */
	bool HasRootWithName(const std::string& InName) const;

	/**
	 * Clear all roots from the manager.
	 */
	void ClearRoots();

	/**
	 * Get the number of registered roots.
	 * @return The count of registered roots
	 */
	size_t GetRootCount() const;

	/**
	 * Validate that a URI is a valid file URI for root usage.
	 * @param InURI The URI to validate
	 * @return True if the URI is valid for use as a root, false otherwise
	 */
	static bool IsValidRootURI(const MCP::URIFile& InURI);

	/**
	 * Create a root with the given URI and optional name.
	 * @param InURI The URI for the root
	 * @param InName Optional name for the root
	 * @return The created root
	 */
	static Root CreateRoot(const MCP::URIFile& InURI, const std::optional<std::string>& InName = std::nullopt);

private:
	std::unordered_map<std::string, Root> m_Roots; // Keyed by URI string
	bool m_WarnOnDuplicateRoots;
	mutable std::mutex m_RootsMutex;

	/**
	 * Get the string key for a root URI.
	 * @param InURI The URI to get the key for
	 * @return The string key for indexing
	 */
	static std::string GetRootKey(const MCP::URIFile& InURI);
};

MCP_NAMESPACE_END
