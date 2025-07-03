#pragma once

#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Messages/MCPMessages.h"
#include "JSONProxy.h"
#include "ToolBase.h"
#include "Utilities/Async/MCPTask.h"

// Forward declarations
class MCPContext;

MCP_NAMESPACE_BEGIN

/**
 * Exception thrown when tool operations fail.
 */
class ToolError : public std::runtime_error {
  public:
    explicit ToolError(const std::string& InMessage) : std::runtime_error(InMessage) {}
};

/**
 * Manages MCP tools.
 * Provides functionality for registering, retrieving, listing, and calling tools.
 */
class ToolManager {
  public:
    using ToolFunction =
        std::function<MCPTask<CallToolResponse::Result>(const JSONData&, MCPContext*)>;

    /**
     * Constructor
     * @param InWarnOnDuplicateTools Whether to warn when duplicate tools are added
     * @param InTools Optional initial list of tools to register
     */
    explicit ToolManager(bool InWarnOnDuplicateTools = true,
                         const std::map<Tool, ToolFunction>& InTools = {});

    /**
     * Add a tool (tool must have Handler member set).
     * @param InTool The tool configuration with handler
     * @return True if the tool was added, false if a tool with the same name already exists
     */
    bool AddTool(const Tool& InTool, const ToolFunction& InFunction);

    /**
     * Remove a tool.
     * @param InTool The tool to remove
     * @return True if the tool was removed, false if the tool does not exist
     */
    bool RemoveTool(const Tool& InTool);

    /**
     * Get tool by name.
     * @param InName The name of the tool to retrieve
     * @return The tool if found, nullopt otherwise
     */
    std::optional<Tool> GetTool(const std::string& InName) const;

    /**
     * Call a tool by name with arguments.
     * @param InName The name of the tool to call
     * @param InArguments The arguments to pass to the tool
     * @param InContext Optional context for the tool execution
     * @param InConvertResult Whether to convert the result format
     * @return Future containing the result of the tool execution
     */
    MCPTask<CallToolResponse::Result> CallTool(const Tool& InTool, const JSONData& InArguments,
                                               MCPContext* InContext = nullptr,
                                               bool InConvertResult = false);

    /**
     * List all registered tools.
     * @return Vector containing all registered tools
     */
    std::vector<Tool> ListTools() const;

    /**
     * Check if a tool with the given name exists.
     * @param InName The name to check
     * @return True if the tool exists, false otherwise
     */
    bool HasTool(const Tool& InTool) const;

  private:
    std::map<Tool, ToolFunction> m_Tools;
    bool m_WarnOnDuplicateTools;
    mutable std::mutex m_Mutex;

    /**
     * Create a basic JSON schema for a tool.
     * @param InName The tool name
     * @return A basic JSON schema
     */
    static JSONSchema CreateBasicSchema(const std::string& InName);
};

MCP_NAMESPACE_END