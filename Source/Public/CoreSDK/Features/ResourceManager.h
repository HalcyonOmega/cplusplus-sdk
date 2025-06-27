#pragma once

#include <any>
#include <functional>
#include <future>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "CoreSDK/Common/Macros.h"
#include "ResourceBase.h"

MCP_NAMESPACE_BEGIN

/**
 * Manages FastMCP resources.
 * Provides functionality for registering, retrieving, and listing resources and templates.
 */
class ResourceManager {
  public:
    using ResourceFunction =
        std::function<std::future<std::any>(const std::unordered_map<std::string, std::string>&)>;

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
    Resource AddResource(const Resource& InResource);

    /**
     * Add a template from a function.
     * @param InFunction The function to handle template-based resource creation
     * @param InURITemplate The URI template string
     * @param InTemplate The resource template configuration
     * @return The added resource template
     */
    ResourceTemplate AddTemplate(ResourceFunction InFunction, const std::string& InURITemplate,
                                 const ResourceTemplate& InTemplate);

    /**
     * Get resource by URI, checking concrete resources first, then templates.
     * @param InURI The URI to search for
     * @return Future containing the resource if found, nullopt otherwise
     */
    std::future<std::optional<Resource>> GetResource(const std::string& InURI);

    /**
     * Get resource by URI synchronously, checking concrete resources first, then templates.
     * @param InURI The URI to search for
     * @return The resource if found, nullopt otherwise
     */
    std::optional<Resource> GetResourceSync(const std::string& InURI);

    /**
     * List all registered resources.
     * @return Vector containing all registered resources
     */
    std::vector<Resource> ListResources() const;

    /**
     * List all registered templates.
     * @return Vector containing all registered templates
     */
    std::vector<ResourceTemplate> ListTemplates() const;

    /**
     * Check if a resource with the given URI exists.
     * @param InURI The URI to check
     * @return True if the resource exists, false otherwise
     */
    bool HasResource(const std::string& InURI) const;

  private:
    std::unordered_map<std::string, Resource> m_Resources;
    std::unordered_map<std::string, std::pair<ResourceTemplate, ResourceFunction>> m_Templates;
    bool m_WarnOnDuplicateResources;
    mutable std::mutex m_ResourcesMutex;

    /**
     * Check if a URI matches a template and extract parameters.
     * @param InTemplate The template to match against
     * @param InURI The URI to match
     * @return Optional map of template parameters if matched
     */
    std::optional<std::unordered_map<std::string, std::string>>
    MatchTemplate(const ResourceTemplate& InTemplate, const std::string& InURI) const;
};

MCP_NAMESPACE_END