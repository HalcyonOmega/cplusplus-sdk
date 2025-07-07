#pragma once

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Core/IMCP.h"

MCP_NAMESPACE_BEGIN

// Client protocol handler
class MCPClient final : public MCPProtocol
{
public:
	MCPClient(TransportType InTransportType, std::optional<std::unique_ptr<TransportOptions>> InOptions,
		const Implementation& InClientInfo, const ClientCapabilities& InCapabilities);

	MCPTask_Void Start() override;
	MCPTask_Void Stop() override;

	// Requests
	MCPTask<InitializeResponse::Result> Request_Initialize(const InitializeRequest::Params& InParams);
	void OnNotified_Initialized(const InitializedNotification& InNotification);

	// Tools
	MCPTask<ListToolsResponse::Result> Request_ListTools(const PaginatedRequestParams& InParams);
	MCPTask<CallToolResponse::Result> Request_CallTool(const CallToolRequest::Params& InParams);
	void OnNotified_ToolListChanged(const ToolListChangedNotification& InNotification);

	// Prompts
	MCPTask<ListPromptsResponse::Result> Request_ListPrompts(const PaginatedRequestParams& InParams);
	MCPTask<GetPromptResponse::Result> Request_GetPrompt(const GetPromptRequest::Params& InParams);
	void OnNotified_PromptListChanged(const PromptListChangedNotification& InNotification);

	// Resources
	MCPTask<ListResourcesResponse::Result> Request_ListResources(const PaginatedRequestParams& InParams);
	MCPTask<ReadResourceResponse::Result> Request_ReadResource(const ReadResourceRequest::Params& InParams);
	MCPTask_Void Request_Subscribe(const SubscribeRequest::Params& InParams);
	MCPTask_Void Request_Unsubscribe(const UnsubscribeRequest::Params& InParams);
	void OnNotified_ResourceListChanged(const ResourceListChangedNotification& InNotification);
	void OnNotified_ResourceUpdated(const ResourceUpdatedNotification& InNotification);

	// Roots
	MCPTask<ListRootsResponse::Result> Request_ListRoots(const PaginatedRequestParams& InParams);
	void OnNotified_RootsListChanged(const RootsListChangedNotification& InNotification);

	// Logging
	MCPTask_Void Request_SetLoggingLevel(const SetLevelRequest::Params& InParams);
	void OnNotified_LogMessage(const LoggingMessageNotification& InNotification);

	// Sampling
	void OnRequest_CreateMessage(const CreateMessageRequest& InParams);

	// Autocomplete
	MCPTask<CompleteResponse::Result> Request_Complete(const CompleteRequest::Params& InParams);

	// Progress
	MCPTask_Void Notify_Progress(const ProgressNotification::Params& InParams);
	MCPTask_Void Notify_CancelRequest(const CancelledNotification::Params& InParams);
	void OnNotified_Progress(const ProgressNotification& InNotification);
	void OnNotified_CancelRequest(const CancelledNotification& InNotification);
};

MCP_NAMESPACE_END