#include "CoreSDK/Features/ResourceManager.h"

#include <map>
#include <regex>
#include <stdexcept>

#include "CoreSDK/Common/Content.h"
#include "CoreSDK/Common/Logging.h"

MCP_NAMESPACE_BEGIN

ResourceManager::ResourceManager(const bool InWarnOnDuplicateResources)
	: m_WarnOnDuplicateResources(InWarnOnDuplicateResources)
{}

bool ResourceManager::AddResource(const Resource& InResource)
{
	const MCP::URI& URI = InResource.URI;
	// Log the addition attempt
	Logger::Debug("Adding resource - URI: " + URI.toString() + ", Name: " + InResource.Name);
	std::lock_guard Lock(m_Mutex);

	if (const auto ExistingIt = m_Resources.find(URI); ExistingIt != m_Resources.end())
	{
		if (m_WarnOnDuplicateResources)
		{
			Logger::Warning("Resource already exists: " + URI.toString());
		}
		return false;
	}

	m_Resources[URI] = InResource;
	return true;
}

bool ResourceManager::RemoveResource(const Resource& InResource)
{
	std::lock_guard Lock(m_Mutex);

	const auto ExistingIt = m_Resources.find(InResource.URI);
	if (ExistingIt == m_Resources.end())
	{
		Logger::Warning("Resource does not exist: " + InResource.URI.toString());
		return false;
	}

	m_Resources.erase(ExistingIt);
	return true;
}

bool ResourceManager::AddTemplate(const ResourceTemplate& InTemplate, const ResourceFunction& InFunction)
{
	// Validate URI template
	if (InTemplate.URITemplate.ToString().empty())
	{
		throw std::invalid_argument("URI template cannot be empty");
	}

	std::lock_guard Lock(m_Mutex);

	// Check for existing template
	if (const auto ExistingIt = m_Templates.find(InTemplate.URITemplate.ToString()); ExistingIt != m_Templates.end())
	{
		if (m_WarnOnDuplicateResources)
		{
			Logger::Warning("Resource template already exists: " + InTemplate.URITemplate.ToString());
		}
		return false;
	}

	// Create a copy of the template with the correct URI template
	ResourceTemplate Template = InTemplate;
	Template.URITemplate = MCP::URITemplate(InTemplate.URITemplate.ToString());

	m_Templates[InTemplate.URITemplate.ToString()] = std::make_pair(Template, InFunction);

	Logger::Debug("Added resource template: " + InTemplate.URITemplate.ToString());
	return true;
}

bool ResourceManager::RemoveTemplate(const ResourceTemplate& InTemplate)
{
	const auto ExistingIt = m_Templates.find(InTemplate.URITemplate.ToString());
	if (ExistingIt == m_Templates.end())
	{
		Logger::Warning("Resource template does not exist: " + InTemplate.URITemplate.ToString());
		return false;
	}

	m_Templates.erase(ExistingIt);
	return true;
}

std::optional<std::variant<TextResourceContents, BlobResourceContents>> ResourceManager::GetResource(
	const MCP::URI& InURI)
{
	Logger::Debug("Getting resource: " + InURI.toString());
	std::lock_guard Lock(m_Mutex);

	// First check concrete resources
	if (const auto ResourceIt = m_Resources.find(InURI); ResourceIt != m_Resources.end())
	{
		// Note: The current design stores Resource metadata, not content.
		// Returning nullopt because we can't provide content for concrete resources yet.
		return std::nullopt;
	}

	// Then check templates
	for (const auto& TemplatePair : m_Templates | std::views::values)
	{
		const auto& [Template, Function] = TemplatePair;

		if (const auto Parameters = MatchTemplate(Template, InURI))
		{
			try
			{
				if (Function)
				{
					return Function(*Parameters);
				}
				throw std::runtime_error("No function provided for template");
			}
			catch (const std::exception& Exception)
			{
				throw std::runtime_error("Error creating resource from template: " + std::string(Exception.what()));
			}
		}
	}

	return std::nullopt;
}

ListResourcesResponse::Result ResourceManager::ListResources(const PaginatedRequestParams& InRequest) const
{
	Logger::Debug("Listing resources - Count: " + std::to_string(m_Resources.size()));

	std::vector<Resource> Result;
	std::lock_guard Lock(m_Mutex);
	Result.reserve(m_Resources.size());

	for (const auto& ResourceData : m_Resources | std::views::values)
	{
		Result.push_back(ResourceData);
	}

	return ListResourcesResponse::Result{
		.Resources = Result,
		.NextCursor = InRequest.NextCursor,
	};
}

ListResourceTemplatesResponse::Result ResourceManager::ListTemplates(const PaginatedRequestParams& InRequest) const
{
	Logger::Debug("Listing templates - Count: " + std::to_string(m_Templates.size()));

	std::vector<ResourceTemplate> Result;
	std::lock_guard Lock(m_Mutex);
	Result.reserve(m_Templates.size());

	for (const auto& [Template, Function] : m_Templates | std::views::values)
	{
		Result.push_back(Template);
	}

	return ListResourceTemplatesResponse::Result{
		.ResourceTemplates = Result,
		= InRequest.NextCursor,
	};
}

bool ResourceManager::HasResource(const MCP::URI& InURI) const
{
	std::lock_guard Lock(m_Mutex);

	return std::ranges::any_of(m_Resources, [InURI](const auto& Pair) { return Pair.first == InURI; });
}

std::optional<std::unordered_map<std::string, std::string>>
ResourceManager::MatchTemplate(const ResourceTemplate& InTemplate, const MCP::URI& InURI)
{
	// Basic URI template matching - this is a simplified implementation
	// A full implementation would use RFC 6570 URI template matching

	const std::string TemplateString = InTemplate.URITemplate.ToString();
	const std::string URIString = InURI.toString();

	// Convert URI template to a regex pattern
	const std::string& Pattern = TemplateString;
	const std::regex VariableRegex(R"(\{([^}]+)\})");
	std::smatch Match;

	// Replace template variables with capturing groups
	auto SearchStart(Pattern.cbegin());
	std::string ResultPattern;

	while (std::regex_search(SearchStart, Pattern.cend(), Match, VariableRegex))
	{
		ResultPattern += std::string(SearchStart, Match[0].first);
		ResultPattern += "([^/]+)"; // Capture group for the variable
		SearchStart = Match[0].second;
	}
	ResultPattern += std::string(SearchStart, Pattern.cend());

	// Try to match the URI against the pattern
	try
	{
		const std::regex URIRegex(ResultPattern);

		if (std::smatch URIMatch; std::regex_match(URIString, URIMatch, URIRegex))
		{
			std::unordered_map<std::string, std::string> Parameters;
			// Extract variable names and values
			auto VarSearchStart(TemplateString.cbegin());
			std::smatch VarMatch;
			int GroupIndex = 1;

			while (std::regex_search(VarSearchStart, TemplateString.cend(), VarMatch, VariableRegex))
			{
				if (GroupIndex < static_cast<int>(URIMatch.size()))
				{
					Parameters[VarMatch[1].str()] = URIMatch[GroupIndex].str();
					GroupIndex++;
				}
				VarSearchStart = VarMatch[0].second;
			}

			return Parameters;
		}
	}
	catch (const std::regex_error& Except)
	{
		// Regex compilation failed, log the error
		Logger::Error("Regex error in MatchTemplate: " + std::string(Except.what()));
		return std::nullopt;
	}

	return std::nullopt;
}

MCP_NAMESPACE_END