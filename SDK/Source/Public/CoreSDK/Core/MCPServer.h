#pragma once

#include <memory>
#include <unordered_map>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Core/IMCP.h"
#include "CoreSDK/Features/PromptManager.h"
#include "CoreSDK/Features/ResourceManager.h"
#include "CoreSDK/Features/SamplingManager.h"
#include "CoreSDK/Features/ToolManager.h"
#include "CoreSDK/Messages/MCPMessages.h"

MCP_NAMESPACE_BEGIN

// Server protocol handler
class MCPServer : public MCPProtocol
{
public:
	MCPServer(ETransportType InTransportType,
		const std::optional<std::unique_ptr<TransportOptions>>& InOptions,
		const Implementation& InServerInfo,
		const ServerCapabilities& InCapabilities);

	// Lifecycle methods
	void Start() override;
	void Stop() override;

	// Initialization
	void Notify_Initialized();

	// Tool management
	bool AddTool(const Tool& InTool, const ToolManager::ToolFunction& InFunction);
	bool RemoveTool(const Tool& InTool);
	void Notify_ToolListChanged();

	// Prompt management
	bool AddPrompt(const Prompt& InPrompt, const PromptManager::PromptFunction& InFunction);
	bool RemovePrompt(const Prompt& InPrompt);
	void Notify_PromptListChanged();

	// Resource management
	bool AddResource(const Resource& InResource);
	bool AddResourceTemplate(const ResourceTemplate& InTemplate, const ResourceManager::ResourceFunction& InFunction);
	bool RemoveResource(const Resource& InResource);
	bool RemoveResourceTemplate(const ResourceTemplate& InTemplate);
	void Notify_ResourceListChanged();
	void Notify_ResourceUpdated(const ResourceUpdatedNotification::Params& InParams);

	// Roots
	OptTask<ListRootsResponse::Result> Request_ListRoots(const PaginatedRequestParams& InParams);

	// Logging
	void Notify_LogMessage(const LoggingMessageNotification::Params& InParams);

	// Sampling
	OptTask<CreateMessageResponse::Result> Request_CreateMessage(const CreateMessageRequest::Params& InParams);

	// Progress reporting
	void Notify_Progress(const ProgressNotification::Params& InParams);
	void Notify_CancelRequest(const CancelledNotification::Params& InParams);

	// ===============================
	// Default Handlers
	// ===============================

	// Setup Default Handlers
	virtual void SetHandlers();

	// Initialization
	void OnRequest_Initialize(const InitializeRequest& InRequest);

	// Tool
	void OnRequest_ListTools(const ListToolsRequest& InRequest);
	void OnRequest_CallTool(const CallToolRequest& InRequest);

	// Prompt
	void OnRequest_ListPrompts(const ListPromptsRequest& InRequest);
	void OnRequest_GetPrompt(const GetPromptRequest& InRequest);

	// Resource
	void OnRequest_ListResources(const ListResourcesRequest& InRequest);
	void OnRequest_ReadResource(const ReadResourceRequest& InRequest);
	void OnRequest_SubscribeResource(const SubscribeRequest& InRequest);
	void OnRequest_UnsubscribeResource(const UnsubscribeRequest& InRequest);

	// Roots
	void OnNotified_RootsListChanged(const RootsListChangedNotification& InNotification);

	// Autocomplete
	void OnRequest_Complete(const CompleteRequest& InRequest);

	// Progress Reporting
	void OnNotified_Progress(const ProgressNotification& InNotification);
	void OnNotified_CancelRequest(const CancelledNotification& InNotification);

	// Feature Managers
	std::weak_ptr<ToolManager> GetToolManager() { return m_ToolManager; }
	std::weak_ptr<ResourceManager> GetResourceManager() { return m_ResourceManager; }
	std::weak_ptr<PromptManager> GetPromptManager() { return m_PromptManager; }
	std::weak_ptr<ITransport> GetTransport() { return m_Transport; }

private:
	// Server state
	bool m_IsRunning{ false };

	// Managers for handling MCP features
	std::shared_ptr<ToolManager> m_ToolManager;
	std::shared_ptr<PromptManager> m_PromptManager;
	std::shared_ptr<ResourceManager> m_ResourceManager;

	// Feature handlers
	// TODO: @HalcyonOmega Cleanup, fix, or remove these handlers
	std::function<void()> m_CompletionHandler;

	// Handler management
	mutable std::mutex m_HandlersMutex;

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