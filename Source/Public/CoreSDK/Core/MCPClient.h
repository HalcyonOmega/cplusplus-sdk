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

	VoidTask Start() override;
	VoidTask Stop() override;

	// Requests
	OptTask<InitializeResponse::Result> Request_Initialize(const InitializeRequest::Params& InParams);
	void OnNotified_Initialized(const InitializedNotification& InNotification);

	// Tools
	OptTask<ListToolsResponse::Result> Request_ListTools(const PaginatedRequestParams& InParams);
	OptTask<CallToolResponse::Result> Request_CallTool(const CallToolRequest::Params& InParams);
	void OnNotified_ToolListChanged(const ToolListChangedNotification& InNotification);

	// Prompts
	OptTask<ListPromptsResponse::Result> Request_ListPrompts(const PaginatedRequestParams& InParams);
	OptTask<GetPromptResponse::Result> Request_GetPrompt(const GetPromptRequest::Params& InParams);
	void OnNotified_PromptListChanged(const PromptListChangedNotification& InNotification);

	// Resources
	OptTask<ListResourcesResponse::Result> Request_ListResources(const PaginatedRequestParams& InParams);
	OptTask<ReadResourceResponse::Result> Request_ReadResource(const ReadResourceRequest::Params& InParams);
	VoidTask Request_Subscribe(const SubscribeRequest::Params& InParams);
	VoidTask Request_Unsubscribe(const UnsubscribeRequest::Params& InParams);
	void OnNotified_ResourceListChanged(const ResourceListChangedNotification& InNotification);
	void OnNotified_ResourceUpdated(const ResourceUpdatedNotification& InNotification);

	// Roots
	OptTask<ListRootsResponse::Result> Request_ListRoots(const PaginatedRequestParams& InParams);
	void OnNotified_RootsListChanged(const RootsListChangedNotification& InNotification);

	// Logging
	VoidTask Request_SetLoggingLevel(const SetLevelRequest::Params& InParams);
	void OnNotified_LogMessage(const LoggingMessageNotification& InNotification);

	// Sampling
	void OnRequest_CreateMessage(const CreateMessageRequest& InRequest);

	// Autocomplete
	OptTask<CompleteResponse::Result> Request_Complete(const CompleteRequest::Params& InParams);

	// Progress
	VoidTask Notify_Progress(const ProgressNotification::Params& InParams);
	VoidTask Notify_CancelRequest(const CancelledNotification::Params& InParams);
	void OnNotified_Progress(const ProgressNotification& InNotification);
	void OnNotified_CancelRequest(const CancelledNotification& InNotification);
};

MCP_NAMESPACE_END