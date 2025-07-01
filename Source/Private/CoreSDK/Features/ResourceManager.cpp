#include "CoreSDK/Features/ResourceManager.h"

#include <regex>
#include <stdexcept>

#include "CoreSDK/Common/Logging.h"

MCP_NAMESPACE_BEGIN

ResourceManager::ResourceManager(bool InWarnOnDuplicateResources)
    : m_WarnOnDuplicateResources(InWarnOnDuplicateResources) {}

Resource ResourceManager::AddResource(const Resource& InResource) {
    const MCP::URI URIString = InResource.URI;
    // Log the addition attempt
    MCP::Logger::Debug("Adding resource - URI: " + URIString.toString()
                       + ", Name: " + InResource.Name);
    std::lock_guard<std::mutex> Lock(m_ResourcesMutex);

    const auto ExistingIt = m_Resources.find(URIString.toString());
    if (ExistingIt != m_Resources.end()) {
        if (m_WarnOnDuplicateResources) {
            MCP::Logger::Warning("Resource already exists: " + URIString.toString());
        }
        return ExistingIt->second;
    }

    m_Resources[URIString.toString()] = InResource;
    return InResource;
}

ResourceTemplate ResourceManager::AddTemplate(const ResourceTemplate& InTemplate) {
    // Validate URI template
    if (InTemplate.URITemplate.ToString().empty()) {
        throw std::invalid_argument("URI template cannot be empty");
    }

    // Check for existing template
    const auto ExistingIt = m_Templates.find(InTemplate.URITemplate.ToString());
    if (ExistingIt != m_Templates.end()) {
        if (m_WarnOnDuplicateResources) {
            MCP::Logger::Warning("Resource template already exists: "
                                 + InTemplate.URITemplate.ToString());
        }
        return ExistingIt->second.first;
    }

    // Create a copy of the template with the correct URI template
    ResourceTemplate Template = InTemplate;
    Template.URITemplate = MCP::URITemplate(InTemplate.URITemplate.ToString());

    m_Templates[InTemplate.URITemplate.ToString()] = std::make_pair(Template, InTemplate.Function);

    MCP::Logger::Debug("Added resource template: " + InTemplate.URITemplate.ToString());
    return Template;
}

std::optional<std::variant<TextResourceContents, BlobResourceContents>>
ResourceManager::GetResource(const MCP::URI& InURI) {
    MCP::Logger::Debug("Getting resource: " + InURI.toString());

    // First check concrete resources
    const auto ResourceIt = m_Resources.find(InURI.toString());
    if (ResourceIt != m_Resources.end()) { return ResourceIt->second; }

    // Then check templates
    for (const auto& [TemplateURI, TemplatePair] : m_Templates) {
        const auto& [Template, Function] = TemplatePair;

        if (auto Parameters = MatchTemplate(Template, InURI.toString())) {
            try {
                // For sync version, we'll create a basic resource
                // In a full implementation, this would involve calling the function
                Resource TemplateResource;
                TemplateResource.URI = MCP::URI(InURI);
                TemplateResource.Name = Template.Name;
                TemplateResource.Description = Template.Description;
                TemplateResource.MIMEType = Template.MIMEType;
                TemplateResource.Annotations = Template.Annotations;

                return TemplateResource;
            } catch (const std::exception& Exception) {
                throw std::runtime_error("Error creating resource from template: "
                                         + std::string(Exception.what()));
            }
        }
    }

    return std::nullopt;
}

std::vector<Resource> ResourceManager::ListResources() const {
    MCP::Logger::Debug("Listing resources - Count: " + std::to_string(m_Resources.size()));

    std::vector<Resource> Result;
    Result.reserve(m_Resources.size());

    for (const auto& [URI, ResourceData] : m_Resources) { Result.push_back(ResourceData); }

    return Result;
}

std::vector<ResourceTemplate> ResourceManager::ListTemplates() const {
    MCP::Logger::Debug("Listing templates - Count: " + std::to_string(m_Templates.size()));

    std::vector<ResourceTemplate> Result;
    Result.reserve(m_Templates.size());

    for (const auto& [URI, TemplatePair] : m_Templates) { Result.push_back(TemplatePair.first); }

    return Result;
}

bool ResourceManager::HasResource(const MCP::URI& InURI) const {
    std::lock_guard<std::mutex> Lock(m_ResourcesMutex);

    return m_Resources.find(InURI.toString()) != m_Resources.end();
}

std::optional<std::unordered_map<std::string, std::string>>
ResourceManager::MatchTemplate(const ResourceTemplate& InTemplate, const MCP::URI& InURI) const {
    // Basic URI template matching - this is a simplified implementation
    // A full implementation would use RFC 6570 URI template matching

    const std::string TemplateString = InTemplate.URITemplate.ToString();

    // Convert URI template to regex pattern
    std::string Pattern = TemplateString;
    std::regex VariableRegex(R"(\{([^}]+)\})");
    std::smatch Match;
    std::unordered_map<std::string, std::string> Parameters;

    // Replace template variables with capturing groups
    std::string::const_iterator SearchStart(Pattern.cbegin());
    std::string ResultPattern;

    while (std::regex_search(SearchStart, Pattern.cend(), Match, VariableRegex)) {
        ResultPattern += std::string(SearchStart, Match[0].first);
        ResultPattern += "([^/]+)"; // Capture group for the variable
        SearchStart = Match[0].second;
    }
    ResultPattern += std::string(SearchStart, Pattern.cend());

    // Try to match the URI against the pattern
    try {
        std::regex URIRegex(ResultPattern);
        std::smatch URIMatch;

        if (std::regex_match(InURI.toString(), URIMatch, URIRegex)) {
            // Extract variable names and values
            std::string::const_iterator VarSearchStart(TemplateString.cbegin());
            std::smatch VarMatch;
            int GroupIndex = 1;

            while (
                std::regex_search(VarSearchStart, TemplateString.cend(), VarMatch, VariableRegex)) {
                if (GroupIndex < static_cast<int>(URIMatch.size())) {
                    Parameters[VarMatch[1].str()] = URIMatch[GroupIndex].str();
                    GroupIndex++;
                }
                VarSearchStart = VarMatch[0].second;
            }

            return Parameters;
        }
    } catch (const std::exception&) {
        // Regex compilation or matching failed
        return std::nullopt;
    }

    return std::nullopt;
}

MCP_NAMESPACE_END