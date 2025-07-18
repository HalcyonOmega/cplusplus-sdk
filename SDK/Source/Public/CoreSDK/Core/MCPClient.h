#pragma once

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Core/IMCP.h"
#include "CoreSDK/Features/RootManager.h"
#include "CoreSDK/Features/SamplingManager.h"

MCP_NAMESPACE_BEGIN

// Client protocol handler
class MCPClient : public MCPProtocol
{
public:
	MCPClient(ETransportType InTransportType,
		const std::optional<std::unique_ptr<TransportOptions>>& InOptions,
		const Implementation& InClientInfo,
		const ClientCapabilities& InCapabilities);

	VoidTask Start() override;
	VoidTask Stop() override;

	// Initialization
	OptTask<InitializeResponse::Result> Request_Initialize(const InitializeRequest::Params& InParams);
	// Tools
	OptTask<ListToolsResponse::Result> Request_ListTools(const PaginatedRequestParams& InParams);
	OptTask<CallToolResponse::Result> Request_CallTool(const CallToolRequest::Params& InParams);

	// Prompts
	OptTask<ListPromptsResponse::Result> Request_ListPrompts(const PaginatedRequestParams& InParams);
	OptTask<GetPromptResponse::Result> Request_GetPrompt(const GetPromptRequest::Params& InParams);

	// Resources
	OptTask<ListResourcesResponse::Result> Request_ListResources(const PaginatedRequestParams& InParams);
	OptTask<ReadResourceResponse::Result> Request_ReadResource(const ReadResourceRequest::Params& InParams);
	VoidTask Request_Subscribe(const SubscribeRequest::Params& InParams);
	VoidTask Request_Unsubscribe(const UnsubscribeRequest::Params& InParams);

	// Root management
	bool AddRoot(const Root& InRoot);
	bool RemoveRoot(const Root& InRoot);
	void Notify_RootsListChanged();

	// Logging
	VoidTask Request_SetLoggingLevel(const SetLevelRequest::Params& InParams);

	// Autocomplete
	OptTask<CompleteResponse::Result> Request_Complete(const CompleteRequest::Params& InParams);

	// Progress
	void Notify_Progress(const ProgressNotification::Params& InParams);
	void Notify_CancelRequest(const CancelledNotification::Params& InParams);

	// ===============================
	// Default Handlers
	// ===============================

	// Setup Default Handlers
	virtual void SetHandlers();

	// Initialization
	void OnNotified_Initialized(const InitializedNotification& InNotification);

	// Tools
	void OnNotified_ToolListChanged(const ToolListChangedNotification& InNotification);

	// Prompts
	void OnNotified_PromptListChanged(const PromptListChangedNotification& InNotification);

	// Resources
	void OnNotified_ResourceListChanged(const ResourceListChangedNotification& InNotification);
	void OnNotified_ResourceUpdated(const ResourceUpdatedNotification& InNotification);

	// Logging
	void OnNotified_LogMessage(const LoggingMessageNotification& InNotification);

	// Roots
	void OnRequest_ListRoots(const ListRootsRequest& InRequest);

	// Sampling
	void OnRequest_CreateMessage(const CreateMessageRequest& InRequest);

	// Progress
	void OnNotified_Progress(const ProgressNotification& InNotification);
	void OnNotified_CancelRequest(const CancelledNotification& InNotification);

	// Managers
	std::weak_ptr<RootManager> GetRootManager() { return m_RootManager; }
	std::weak_ptr<SamplingManager> GetSamplingManager() { return m_SamplingManager; }

private:
	std::shared_ptr<RootManager> m_RootManager;
	std::shared_ptr<SamplingManager> m_SamplingManager;
};

MCP_NAMESPACE_END