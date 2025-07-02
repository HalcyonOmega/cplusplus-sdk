#include "CoreSDK/Features/ToolManager.h"

#include "CoreSDK/Common/Logging.h"
#include "CoreSDK/Messages/MCPMessages.h"

MCP_NAMESPACE_BEGIN

ToolManager::ToolManager(bool InWarnOnDuplicateTools, const std::map<Tool, ToolFunction>& InTools)
    : m_WarnOnDuplicateTools(InWarnOnDuplicateTools) {
    std::lock_guard<std::mutex> Lock(m_Mutex);

    // Register initial tools if provided
    for (const auto& [ToolItem, Function] : InTools) { AddTool(ToolItem, Function); }
}

std::optional<Tool> ToolManager::GetTool(const std::string& InName) const {
    std::lock_guard<std::mutex> Lock(m_Mutex);

    for (const auto& [ToolItem, Function] : m_Tools) {
        if (ToolItem.Name == InName) { return ToolItem; }
    }
    return std::nullopt;
}

std::vector<Tool> ToolManager::ListTools() const {
    std::lock_guard<std::mutex> Lock(m_Mutex);

    std::vector<Tool> Result;
    Result.reserve(m_Tools.size());

    for (const auto& [ToolItem, Function] : m_Tools) { Result.emplace_back(ToolItem); }

    return Result;
}

bool ToolManager::AddTool(const Tool& InTool, const ToolFunction& InFunction) {
    Logger::Debug("Adding tool: " + InTool.Name);
    std::lock_guard<std::mutex> Lock(m_Mutex);

    const auto ExistingIt = m_Tools.find(InTool);
    if (ExistingIt != m_Tools.end()) {
        if (m_WarnOnDuplicateTools) { Logger::Warning("Tool already exists: " + InTool.Name); }
        return false;
    }

    m_Tools.emplace(InTool, InFunction);
    return true;
}

MCPTask<CallToolResponse::Result> ToolManager::CallTool(const Tool& InTool,
                                                        const JSONData& InArguments,
                                                        MCPContext* InContext,
                                                        bool /*InConvertResult*/) {
    std::lock_guard<std::mutex> Lock(m_Mutex);

    const auto Iter = m_Tools.find(InTool);
    if (Iter == m_Tools.end()) { throw ToolError("Unknown tool: " + InTool.Name); }

    return Iter->second(InArguments, InContext);
}

bool ToolManager::HasTool(const Tool& InTool) const {
    std::lock_guard<std::mutex> Lock(m_Mutex);

    return m_Tools.find(InTool) != m_Tools.end();
}

JSONSchema ToolManager::CreateBasicSchema(const std::string& InName) {
    JSONSchema Schema;
    Schema.Type = "object";
    Schema.Properties = std::unordered_map<std::string, JSONData>();
    Schema.Required = std::vector<std::string>();

    // Add a basic property for demonstration
    (*Schema.Properties)["input"] =
        JSONData{{"type", "string"}, {"description", "Input parameter for " + InName}};

    return Schema;
}

MCP_NAMESPACE_END