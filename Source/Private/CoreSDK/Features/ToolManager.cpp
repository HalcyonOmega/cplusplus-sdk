#include "CoreSDK/Features/ToolManager.h"

#include <stdexcept>

#include "CoreSDK/Common/Logging.h"

MCP_NAMESPACE_BEGIN

ToolManager::ToolManager(bool InWarnOnDuplicateTools, const std::vector<Tool>& InTools)
    : m_WarnOnDuplicateTools(InWarnOnDuplicateTools) {
    std::lock_guard<std::mutex> Lock(m_ToolsMutex);

    // Register initial tools if provided
    for (const auto& ToolItem : InTools) { AddTool(ToolItem); }
}

std::optional<Tool> ToolManager::GetTool(const std::string& InName) const {
    std::lock_guard<std::mutex> Lock(m_ToolsMutex);

    const auto It = m_Tools.find(InName);
    if (It != m_Tools.end()) { return It->second; }
    return std::nullopt;
}

std::vector<Tool> ToolManager::ListTools() const {
    std::lock_guard<std::mutex> Lock(m_ToolsMutex);

    std::vector<Tool> Result;
    Result.reserve(m_Tools.size());

    for (const auto& [Name, ToolItem] : m_Tools) { Result.push_back(ToolItem); }

    return Result;
}

Tool ToolManager::AddTool(const Tool& InTool) {
    MCP::Logger::Debug("Adding tool: " + InTool.Name);
    std::lock_guard<std::mutex> Lock(m_ToolsMutex);

    const auto ExistingIt = m_Tools.find(InTool.Name);
    if (ExistingIt != m_Tools.end()) {
        if (m_WarnOnDuplicateTools) { MCP::Logger::Warning("Tool already exists: " + InTool.Name); }
        return ExistingIt->second;
    }

    m_Tools[InTool.Name] = InTool;
    return InTool;
}

std::future<std::any> ToolManager::CallTool(const std::string& InName, const JSONValue& InArguments,
                                            MCPContext* InContext, bool /*InConvertResult*/) {
    std::lock_guard<std::mutex> Lock(m_ToolsMutex);

    const auto It = m_Tools.find(InName);
    if (It == m_Tools.end()) {
        return std::async(std::launch::async,
                          [InName]() -> std::any { throw ToolError("Unknown tool: " + InName); });
    }

    const Tool& ToolItem = It->second;

    // Note: This assumes Tool has a Handler member
    // If this doesn't compile, the Tool class needs a Handler member added
    return std::async(std::launch::async, [ToolItem, InArguments, InContext]() -> std::any {
        // Tool execution would go here using ToolItem.Handler
        return MCP::Logger::Debug("Tool executed: " + ToolItem.Name);
    });
}

std::any ToolManager::CallToolSync(const std::string& InName, const JSONValue& InArguments,
                                   MCPContext* InContext, bool /*InConvertResult*/) {
    auto Future = CallTool(InName, InArguments, InContext, false);
    return Future.get();
}

bool ToolManager::HasTool(const std::string& InName) const {
    std::lock_guard<std::mutex> Lock(m_ToolsMutex);

    return m_Tools.find(InName) != m_Tools.end();
}

JSONSchema ToolManager::CreateBasicSchema(const std::string& InName) const {
    JSONSchema Schema;
    Schema.Type = "object";
    Schema.Properties = std::unordered_map<std::string, JSONValue>();
    Schema.Required = std::vector<std::string>();

    // Add a basic property for demonstration
    (*Schema.Properties)["input"] =
        JSONValue{{"type", "string"}, {"description", "Input parameter for " + InName}};

    return Schema;
}

MCP_NAMESPACE_END