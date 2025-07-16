#pragma once

#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Messages/MCPMessages.h"
#include "JSONProxy.h"
#include "ToolBase.h"

// Forward declarations
class MCPContext;

MCP_NAMESPACE_BEGIN

/**
 * Exception thrown when tool operations fail.
 */
class ToolError : public std::runtime_error
{
public:
	explicit ToolError(const std::string& InMessage) : std::runtime_error(InMessage) {}
};

/**
 * Manages MCP tools.
 * Provides functionality for registering, retrieving, listing, and calling
 * tools.
 */
class ToolManager
{
public:
	using ToolFunction = std::function<CallToolResponse::Result(const JSONData&, MCPContext*)>;

	/**
	 * Constructor
	 * @param InWarnOnDuplicateTools Whether to warn when duplicate tools are
	 * added
	 * @param InTools Optional initial list of tools to register
	 */
	explicit ToolManager(bool InWarnOnDuplicateTools = true, const std::map<Tool, ToolFunction>& InTools = {});

	/**
	 * Add a tool (tool must have Handler member set).
	 * @param InTool The tool configuration with handler
	 * @param InFunction
	 * @return True if the tool was added, false if a tool with the same name
	 * already exists
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
	std::optional<Tool> FindTool(const std::string& InName) const;

	/**
	 * Call a tool by name with arguments.
	 * @param InRequest
	 * @param InContext Optional context for the tool execution
	 * @return The result of the tool execution
	 */
	CallToolResponse::Result CallTool(const CallToolRequest::Params* InRequest, MCPContext* InContext = nullptr);

	/**
	 * List all registered tools.
	 * @return Vector containing all registered tools
	 */
	ListToolsResponse::Result ListTools(const PaginatedRequestParams* InRequest) const;

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