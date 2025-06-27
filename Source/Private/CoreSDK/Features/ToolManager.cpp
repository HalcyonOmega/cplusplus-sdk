#include "CoreSDK/Features/ToolManager.h"

#include <stdexcept>

#include "CoreSDK/Common/Logging.h"

MCP_NAMESPACE_BEGIN

ToolManager::ToolManager(bool InWarnOnDuplicateTools, const std::vector<Tool>& InTools)
    : m_WarnOnDuplicateTools(InWarnOnDuplicateTools) {
    // Register initial tools if provided
    for (const auto& ToolItem : InTools) {
        const auto ExistingIt = m_Tools.find(ToolItem.Name);
        if (ExistingIt != m_Tools.end()) {
            if (m_WarnOnDuplicateTools) {
                MCP::Logger::Warning("Tool already exists: " + ToolItem.Name);
            }
        } else {
            // Create a dummy function for initial tools
            ToolFunction DummyFunction =
                [ToolItem](const JSONValue& InArgs,
                           MCPContext* InContext) -> std::future<std::any> {
                return std::async(std::launch::async, []() -> std::any {
                    throw ToolError("Tool function not implemented for: " + ToolItem.Name);
                });
            };
            m_Tools[ToolItem.Name] = std::make_pair(ToolItem, DummyFunction);
        }
    }
}

std::optional<Tool> ToolManager::GetTool(const std::string& InName) const {
    const auto It = m_Tools.find(InName);
    if (It != m_Tools.end()) { return It->second.first; }
    return std::nullopt;
}

std::vector<Tool> ToolManager::ListTools() const {
    std::vector<Tool> Result;
    Result.reserve(m_Tools.size());

    for (const auto& [Name, ToolPair] : m_Tools) { Result.push_back(ToolPair.first); }

    return Result;
}

Tool ToolManager::AddTool(ToolFunction InFunction, const Tool& InTool) {
    MCP::Logger::Debug("Adding tool: " + InTool.Name);

    const auto ExistingIt = m_Tools.find(InTool.Name);
    if (ExistingIt != m_Tools.end()) {
        if (m_WarnOnDuplicateTools) { MCP::Logger::Warning("Tool already exists: " + InTool.Name); }
        return ExistingIt->second.first;
    }

    m_Tools[InTool.Name] = std::make_pair(InTool, InFunction);
    return InTool;
}

Tool ToolManager::AddTool(ToolFunction InFunction, const std::string& InName,
                          const std::optional<std::string>& InDescription,
                          const std::optional<ToolAnnotations>& InAnnotations) {
    // Create a tool with basic configuration
    Tool NewTool;
    NewTool.Name = InName;
    NewTool.Description = InDescription;
    NewTool.InputSchema = CreateBasicSchema(InName);
    NewTool.Annotations = InAnnotations;

    return AddTool(InFunction, NewTool);
}

std::future<std::any> ToolManager::CallTool(const std::string& InName, const JSONValue& InArguments,
                                            MCPContext* InContext, bool InConvertResult) {
    const auto It = m_Tools.find(InName);
    if (It == m_Tools.end()) {
        return std::async(std::launch::async,
                          [InName]() -> std::any { throw ToolError("Unknown tool: " + InName); });
    }

    const auto& [ToolConfig, Function] = It->second;

    return std::async(std::launch::async,
                      [Function, InArguments, InContext, InConvertResult]() -> std::any {
                          auto Future = Function(InArguments, InContext);
                          return Future.get();
                      });
}

std::any ToolManager::CallToolSync(const std::string& InName, const JSONValue& InArguments,
                                   MCPContext* InContext, bool InConvertResult) {
    const auto It = m_Tools.find(InName);
    if (It == m_Tools.end()) { throw ToolError("Unknown tool: " + InName); }

    const auto& [ToolConfig, Function] = It->second;

    auto Future = Function(InArguments, InContext);
    return Future.get();
}

bool ToolManager::HasTool(const std::string& InName) const {
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