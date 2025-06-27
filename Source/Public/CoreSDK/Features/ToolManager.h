#pragma once

#include <any>
#include <functional>
#include <future>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"
#include "ToolBase.h"

MCP_NAMESPACE_BEGIN

// Forward declarations
class MCPContext;

/**
 * Exception thrown when tool operations fail.
 */
class ToolError : public std::runtime_error {
  public:
    explicit ToolError(const std::string& InMessage) : std::runtime_error(InMessage) {}
};

/**
 * Manages FastMCP tools.
 * Provides functionality for registering, retrieving, listing, and calling tools.
 */
class ToolManager {
  public:
    using ToolFunction = std::function<std::future<std::any>(const JSONValue&, MCPContext*)>;

    /**
     * Constructor
     * @param InWarnOnDuplicateTools Whether to warn when duplicate tools are added
     * @param InTools Optional initial list of tools to register
     */
    explicit ToolManager(bool InWarnOnDuplicateTools = true, const std::vector<Tool>& InTools = {});

    /**
     * Get tool by name.
     * @param InName The name of the tool to retrieve
     * @return The tool if found, nullopt otherwise
     */
    std::optional<Tool> GetTool(const std::string& InName) const;

    /**
     * List all registered tools.
     * @return Vector containing all registered tools
     */
    std::vector<Tool> ListTools() const;

    /**
     * Add a tool (tool must have Handler member set).
     * @param InTool The tool configuration with handler
     * @return The added tool. If a tool with the same name exists, returns the existing tool.
     */
    Tool AddTool(const Tool& InTool);

    /**
     * Add a tool with automatic schema generation.
     * @param InHandler The function to execute when the tool is called
     * @param InName The name of the tool
     * @param InDescription Optional description of the tool
     * @param InAnnotations Optional tool annotations
     * @return The added tool
     */
    Tool AddTool(ToolFunction InHandler, const std::string& InName,
                 const std::optional<std::string>& InDescription = std::nullopt,
                 const std::optional<ToolAnnotations>& InAnnotations = std::nullopt);

    /**
     * Call a tool by name with arguments.
     * @param InName The name of the tool to call
     * @param InArguments The arguments to pass to the tool
     * @param InContext Optional context for the tool execution
     * @param InConvertResult Whether to convert the result format
     * @return Future containing the result of the tool execution
     */
    std::future<std::any> CallTool(const std::string& InName, const JSONValue& InArguments,
                                   MCPContext* InContext = nullptr, bool InConvertResult = false);

    /**
     * Call a tool by name with arguments synchronously.
     * @param InName The name of the tool to call
     * @param InArguments The arguments to pass to the tool
     * @param InContext Optional context for the tool execution
     * @param InConvertResult Whether to convert the result format
     * @return The result of the tool execution
     */
    std::any CallToolSync(const std::string& InName, const JSONValue& InArguments,
                          MCPContext* InContext = nullptr, bool InConvertResult = false);

    /**
     * Check if a tool with the given name exists.
     * @param InName The name to check
     * @return True if the tool exists, false otherwise
     */
    bool HasTool(const std::string& InName) const;

  private:
    std::unordered_map<std::string, Tool> m_Tools;
    bool m_WarnOnDuplicateTools;
    mutable std::mutex m_ToolsMutex;

    /**
     * Create a basic JSON schema for a tool.
     * @param InName The tool name
     * @return A basic JSON schema
     */
    static JSONSchema CreateBasicSchema(const std::string& InName);
};

MCP_NAMESPACE_END