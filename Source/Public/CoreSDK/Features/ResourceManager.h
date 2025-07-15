#pragma once

#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

#include "CoreSDK/Common/Content.h"
#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Messages/MCPMessages.h"
#include "ResourceBase.h"

MCP_NAMESPACE_BEGIN

/**
 * Manages MCP resources.
 * Provides functionality for registering, retrieving, and listing resources and templates.
 */
class ResourceManager
{
public:
	using ResourceFunction = std::function<std::variant<TextResourceContents, BlobResourceContents>(
		const std::unordered_map<std::string, std::string>&)>;

	/**
	 * Constructor
	 * @param InWarnOnDuplicateResources Whether to warn when duplicate resources are added
	 */
	explicit ResourceManager(bool InWarnOnDuplicateResources = true);

	/**
	 * Add a resource to the manager.
	 * @param InResource The resource to add
	 * @return The added resource. If a resource with the same URI already exists, returns the
	 * existing resource.
	 */
	bool AddResource(const Resource& InResource);

	/**
	 * Remove a resource from the manager.
	 * @param InResource The resource to remove
	 */
	bool RemoveResource(const Resource& InResource);

	/**
	 * Add a template from a function.
	 * @param InFunction The function to handle template-based resource creation
	 * @param InTemplate The resource template configuration
	 * @return The added resource template
	 */
	bool AddTemplate(const ResourceTemplate& InTemplate, const ResourceFunction& InFunction);

	/**
	 * Remove a template from the manager.
	 * @param InTemplate The template to remove
	 */
	bool RemoveTemplate(const ResourceTemplate& InTemplate);

	/**
	 * Get resource content by URI, checking concrete resources first, then templates.
	 * @param InURI The URI to search for
	 * @return The resource content if found, nullopt otherwise
	 */
	std::optional<std::variant<TextResourceContents, BlobResourceContents>> GetResource(const MCP::URI& InURI);

	/**
	 * List all registered resources.
	 * @return Vector containing all registered resources
	 */
	ListResourcesResponse::Result ListResources(const PaginatedRequestParams* InRequest);

	/**
	 * List all registered templates.
	 * @return Vector containing all registered templates
	 */
	ListResourceTemplatesResponse::Result ListTemplates(const PaginatedRequestParams* InRequest);

	/**
	 * Check if a resource with the given URI exists.
	 * @param InURI The URI to check
	 * @return True if the resource exists, false otherwise
	 */
	bool HasResource(const MCP::URI& InURI) const;

private:
	std::map<MCP::URI, Resource> m_Resources;
	std::unordered_map<std::string, std::pair<ResourceTemplate, ResourceFunction>> m_Templates;
	bool m_WarnOnDuplicateResources;
	mutable std::mutex m_Mutex;

	std::unordered_map<std::string /* Resource */, std::vector<std::string> /* Connections */> m_ResourceSubscriptions;
	mutable std::mutex m_ResourceSubscriptionsMutex;

	/**
	 * Check if a URI matches a template and extract parameters.
	 * @param InTemplate The template to match against
	 * @param InURI The URI to match
	 * @return Optional map of template parameters if matched
	 */
	static std::optional<std::unordered_map<std::string, std::string>> MatchTemplate(const ResourceTemplate& InTemplate,
		const MCP::URI& InURI);
};

MCP_NAMESPACE_END