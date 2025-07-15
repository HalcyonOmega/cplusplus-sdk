#include "CoreSDK/Features/ToolManager.h"

#include "CoreSDK/Common/Logging.h"
#include "CoreSDK/Messages/MCPMessages.h"

MCP_NAMESPACE_BEGIN

ToolManager::ToolManager(const bool InWarnOnDuplicateTools, const std::map<Tool, ToolFunction>& InTools)
	: m_WarnOnDuplicateTools(InWarnOnDuplicateTools)
{
	std::lock_guard Lock(m_Mutex);

	// Register initial tools if provided
	for (const auto& [ToolItem, Function] : InTools)
	{
		AddTool(ToolItem, Function);
	}
}

ListToolsResponse::Result ToolManager::ListTools(const PaginatedRequestParams* InRequest) const
{
	std::lock_guard Lock(m_Mutex);
	(void)InRequest; // TODO: @HalcyonOmega - Implement pagination

	std::vector<Tool> Result;
	Result.reserve(m_Tools.size());

	for (const auto& ToolItem : m_Tools | std::views::keys)
	{
		Result.emplace_back(ToolItem);
	}

	return ListToolsResponse::Result{ Result };
}

bool ToolManager::AddTool(const Tool& InTool, const ToolFunction& InFunction)
{
	Logger::Debug("Adding tool: " + InTool.Name);
	std::lock_guard Lock(m_Mutex);

	if (const auto ExistingIt = m_Tools.find(InTool); ExistingIt != m_Tools.end())
	{
		if (m_WarnOnDuplicateTools)
		{
			Logger::Warning("Tool already exists: " + InTool.Name);
		}
		return false;
	}

	m_Tools.emplace(InTool, InFunction);
	return true;
}

bool ToolManager::RemoveTool(const Tool& InTool)
{
	std::lock_guard Lock(m_Mutex);

	const auto ExistingIt = m_Tools.find(InTool);
	if (ExistingIt == m_Tools.end())
	{
		Logger::Warning("Tool does not exist: " + InTool.Name);
		return false;
	}

	m_Tools.erase(ExistingIt);
	return true;
}

CallToolResponse::Result ToolManager::CallTool(const CallToolRequest::Params& InRequest, MCPContext* InContext)
{
	std::lock_guard Lock(m_Mutex);

	const std::optional<Tool> FoundTool = FindTool(InRequest.Name);
	if (!FoundTool)
	{
		throw ToolError("Unknown tool: " + InRequest.Name);
	}

	return m_Tools.find(FoundTool.value())->second(InRequest.Arguments, InContext);
}

std::optional<Tool> ToolManager::FindTool(const std::string& InName) const
{
	std::lock_guard Lock(m_Mutex);

	if (const auto Iterator
		= std::ranges::find_if(m_Tools, [&](const auto& Pair) { return Pair.first.Name == InName; });
		Iterator != m_Tools.end())
	{
		return Iterator->first;
	}

	return std::nullopt;
}

JSONSchema ToolManager::CreateBasicSchema(const std::string& InName)
{
	JSONSchema Schema;
	Schema.Type = "object";
	Schema.Properties = std::unordered_map<std::string, JSONData>();
	Schema.Required = std::vector<std::string>();

	// Add a basic property for demonstration
	(*Schema.Properties)["input"]
		= JSONData{ { "type", "string" }, { "description", "Input parameter for " + InName } };

	return Schema;
}

MCP_NAMESPACE_END