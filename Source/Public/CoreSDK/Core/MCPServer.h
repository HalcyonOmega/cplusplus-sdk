#pragma once

#include <memory>
#include <unordered_map>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Core/IMCP.h"
#include "CoreSDK/Features/PromptManager.h"
#include "CoreSDK/Features/ResourceManager.h"
#include "CoreSDK/Features/RootManager.h"
#include "CoreSDK/Features/SamplingManager.h"
#include "CoreSDK/Features/ToolManager.h"
#include "CoreSDK/Messages/MCPMessages.h"

MCP_NAMESPACE_BEGIN

// Server protocol handler
class MCPServer final : public MCPProtocol
{
public:
	MCPServer(TransportType InTransportType,
		std::optional<std::unique_ptr<TransportOptions>> InOptions,
		const Implementation& InServerInfo,
		const ServerCapabilities& InCapabilities);

	// Lifecycle methods
	VoidTask Start() override;
	VoidTask Stop() override;

	// Initialization
	void OnRequest_Initialize(const InitializeRequest& InRequest);
	void Notify_Initialized();

	// Tool management
	bool AddTool(const Tool& InTool, const ToolManager::ToolFunction& InFunction);
	bool RemoveTool(const Tool& InTool);
	void Notify_ToolListChanged();
	void OnRequest_ListTools(const ListToolsRequest& InRequest);
	void OnRequest_CallTool(const CallToolRequest& InRequest);

	// Prompt management
	bool AddPrompt(const Prompt& InPrompt, const PromptManager::PromptFunction& InFunction);
	bool RemovePrompt(const Prompt& InPrompt);
	void Notify_PromptListChanged();
	void OnRequest_ListPrompts(const ListPromptsRequest& InRequest);
	void OnRequest_GetPrompt(const GetPromptRequest& InRequest);

	// Resource management
	bool AddResource(const Resource& InResource);
	bool AddResourceTemplate(const ResourceTemplate& InTemplate, const ResourceManager::ResourceFunction& InFunction);
	bool RemoveResource(const Resource& InResource);
	bool RemoveResourceTemplate(const ResourceTemplate& InTemplate);
	void Notify_ResourceListChanged();
	void Notify_ResourceUpdated(const ResourceUpdatedNotification::Params& InParams);
	void OnRequest_ListResources(const ListResourcesRequest& InRequest);
	void OnRequest_ReadResource(const ReadResourceRequest& InRequest);
	void OnRequest_SubscribeResource(const SubscribeRequest& InRequest);
	void OnRequest_UnsubscribeResource(const UnsubscribeRequest& InRequest);

	// Root management
	bool AddRoot(const Root& InRoot);
	bool RemoveRoot(const Root& InRoot);
	void Notify_RootsListChanged();

	// Logging
	void Notify_LogMessage(const LoggingMessageNotification::Params& InParams);

	// Sampling requests (servers can request sampling from clients)
	Task<CreateMessageResponse::Result> Request_CreateMessage(const CreateMessageRequest::Params& InParams);

	// Autocomplete
	void OnRequest_Complete(const CompleteRequest& InRequest);

	// Progress reporting
	void Notify_Progress(const ProgressNotification::Params& InParams);
	void Notify_CancelRequest(const CancelledNotification::Params& InParams);
	void OnNotified_Progress(const ProgressNotification& InNotification);
	void OnNotified_CancelRequest(const CancelledNotification& InNotification);

	// Access to managers for MCPContext
	std::weak_ptr<ToolManager> GetToolManager() { return m_ToolManager; }
	std::weak_ptr<ResourceManager> GetResourceManager() { return m_ResourceManager; }
	std::weak_ptr<PromptManager> GetPromptManager() { return m_PromptManager; }
	std::weak_ptr<RootManager> GetRootManager() { return m_RootManager; }

private:
	// Server state
	bool m_IsRunning{ false };

	// Managers for handling MCP features
	std::shared_ptr<ToolManager> m_ToolManager;
	std::shared_ptr<PromptManager> m_PromptManager;
	std::shared_ptr<ResourceManager> m_ResourceManager;
	std::shared_ptr<RootManager> m_RootManager;

	// Feature handlers
	// TODO: @HalcyonOmega Cleanup, fix, or remove these handlers
	std::function<void()> m_CompletionHandler;

	// Handler management
	mutable std::mutex m_HandlersMutex;

	// Setup methods
	void SetupDefaultHandlers();

	// Progress tracking and tool execution
	Task<CallToolResponse> ExecuteToolWithProgress(const Tool& InTool,
		const std::optional<std::unordered_map<std::string, JSONData>>& InArguments,
		const RequestID& InRequestID);

	// Resource subscription management
	void Notify_ResourceSubscribers(const ResourceUpdatedNotification::Params& InParams);
	std::string_view GetCurrentClientID() const;

	VoidTask UpdateProgress(double InProgress, std::optional<int64_t> InTotal = {});
	VoidTask CompleteProgress();
};

MCP_NAMESPACE_END