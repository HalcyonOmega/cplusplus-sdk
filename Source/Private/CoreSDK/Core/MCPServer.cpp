#include "CoreSDK/Core/MCPServer.h"

#include "CoreSDK/Common/Content.h"
#include "CoreSDK/Common/Progress.h"
#include "CoreSDK/Messages/MCPMessages.h"

MCP_NAMESPACE_BEGIN

MCPServer::MCPServer(const ETransportType InTransportType,
	const std::optional<std::unique_ptr<TransportOptions>>& InOptions,
	const Implementation& InServerInfo,
	const ServerCapabilities& InCapabilities)
	: MCPProtocol(TransportFactory::CreateTransport(InTransportType, ETransportSide::Server, InOptions), true)
{
	MCPServer::SetHandlers();
	SetServerInfo(InServerInfo);
	SetServerCapabilities(InCapabilities);
}

VoidTask MCPServer::Start()
{
	if (m_IsRunning)
	{
		HandleRuntimeError("Server already running");
		co_return;
	}

	try
	{
		// Start transport
		co_await m_Transport->Connect();
		m_IsRunning = true;
	}
	catch (const std::exception& Except)
	{
		HandleRuntimeError("Failed to start server: " + std::string(Except.what()));
		co_return;
	}

	co_return;
}

VoidTask MCPServer::Stop()
{
	if (!m_IsRunning)
	{
		HandleRuntimeError("Server already stopped");
		co_return;
	}

	try
	{
		co_await m_Transport->Disconnect();
		m_IsRunning = false;
	}
	catch (const std::exception& Except)
	{
		HandleRuntimeError("Failed to stop server: " + std::string(Except.what()));
		co_return;
	}

	co_return;
}

void MCPServer::OnRequest_Initialize(const InitializeRequest& InRequest)
{
	try
	{
		if (const auto Request = GetRequestParams<InitializeRequest::Params>(InRequest))
		{
			SendMessage(InitializeResponse{ InRequest.GetRequestID(),
				InitializeResponse::Result{ m_ServerInfo.ProtocolVersion, m_ServerInfo, m_ServerCapabilities } });
		}
	}
	catch (const std::exception& Except)
	{
		SendMessage(ErrorInternalError(InRequest.GetRequestID(), Except.what()));
	}
}

void MCPServer::Notify_Initialized() { SendMessage(InitializedNotification()); }

bool MCPServer::AddTool(const Tool& InTool, const ToolManager::ToolFunction& InFunction)
{
	if (!m_ToolManager->AddTool(InTool, InFunction))
	{
		HandleRuntimeError("Failed to add tool: " + InTool.Name);
		return false;
	}

	Notify_ToolListChanged();
	return true;
}

bool MCPServer::RemoveTool(const Tool& InTool)
{
	if (!m_ToolManager->RemoveTool(InTool))
	{
		HandleRuntimeError("Failed to remove tool: " + InTool.Name);
		return false;
	}

	Notify_ToolListChanged();
	return true;
}

void MCPServer::Notify_ToolListChanged() { SendMessage(ToolListChangedNotification()); }

void MCPServer::OnRequest_ListTools(const ListToolsRequest& InRequest)
{
	try
	{
		const auto RequestParams = GetRequestParams<PaginatedRequestParams>(InRequest);

		const ListToolsResponse::Result Result = m_ToolManager->ListTools(RequestParams.value());

		SendMessage(ListToolsResponse(InRequest.GetRequestID(), Result));
	}
	catch (const std::exception& Except)
	{
		SendMessage(ErrorInternalError(InRequest.GetRequestID(), Except.what()));
	}
}

void MCPServer::OnRequest_CallTool(const CallToolRequest& InRequest)
{
	try
	{
		const auto Request = GetRequestParams<CallToolRequest::Params>(InRequest).value();
		Tool Tool = m_ToolManager->FindTool(Request->Name).value();

		// Use ToolManager to call the tool
		const auto Result = m_ToolManager->CallTool(Request);

		const CallToolResponse::Result ResponseResult{ Result.Content, Result.IsError };
		SendMessage(CallToolResponse(InRequest.GetRequestID(), ResponseResult));
	}
	catch (const std::exception& Except)
	{
		SendMessage(ErrorInternalError(InRequest.GetRequestID(), Except.what()));
	}
}

bool MCPServer::AddPrompt(const Prompt& InPrompt, const PromptManager::PromptFunction& InFunction)
{
	if (!m_PromptManager->AddPrompt(InPrompt, InFunction))
	{
		HandleRuntimeError("Failed to add prompt: " + InPrompt.Name);
		return false;
	}

	Notify_PromptListChanged();
	return true;
}

bool MCPServer::RemovePrompt(const Prompt& InPrompt)
{
	if (!m_PromptManager->RemovePrompt(InPrompt))
	{
		HandleRuntimeError("Failed to remove prompt: " + InPrompt.Name);
		return false;
	}

	Notify_PromptListChanged();
	return true;
}

void MCPServer::Notify_PromptListChanged() { SendMessage(PromptListChangedNotification()); }

void MCPServer::OnRequest_ListPrompts(const ListPromptsRequest& InRequest)
{
	try
	{
		const auto RequestParams = GetRequestParams<PaginatedRequestParams>(InRequest);

		const ListPromptsResponse::Result Result = m_PromptManager->ListPrompts(RequestParams.value());

		SendMessage(ListPromptsResponse(InRequest.GetRequestID(), Result));
	}
	catch (const std::exception& Except)
	{
		SendMessage(ErrorInternalError(InRequest.GetRequestID(), Except.what()));
	}
}

void MCPServer::OnRequest_GetPrompt(const GetPromptRequest& InRequest)
{
	try
	{
		const auto Request = GetRequestParams<GetPromptRequest::Params>(InRequest).value();

		const GetPromptResponse::Result Result = m_PromptManager->GetPrompt(Request);

		SendMessage(GetPromptResponse(InRequest.GetRequestID(), Result));
	}
	catch (const std::exception& Except)
	{
		SendMessage(ErrorInternalError(InRequest.GetRequestID(), Except.what()));
	}
}

bool MCPServer::AddResource(const Resource& InResource)
{
	if (!m_ResourceManager->AddResource(InResource))
	{
		HandleRuntimeError("Failed to add resource: " + InResource.URI.toString());
		return false;
	}

	Notify_ResourceListChanged();
	return true;
}

bool MCPServer::AddResourceTemplate(const ResourceTemplate& InTemplate,
	const ResourceManager::ResourceFunction& InFunction)
{
	if (!m_ResourceManager->AddTemplate(InTemplate, InFunction))
	{
		HandleRuntimeError("Failed to add resource template: " + InTemplate.Name);
		return false;
	}

	Notify_ResourceListChanged();
	return true;
}

bool MCPServer::RemoveResource(const Resource& InResource)
{
	if (!m_ResourceManager->RemoveResource(InResource))
	{
		HandleRuntimeError("Failed to remove resource: " + InResource.URI.toString());
		return false;
	}

	Notify_ResourceListChanged();
	return true;
}

bool MCPServer::RemoveResourceTemplate(const ResourceTemplate& InTemplate)
{
	if (!m_ResourceManager->RemoveTemplate(InTemplate))
	{
		HandleRuntimeError("Failed to remove resource template: " + InTemplate.Name);
		return false;
	}

	Notify_ResourceListChanged();
	return true;
}

void MCPServer::Notify_ResourceListChanged() { SendMessage(ResourceListChangedNotification()); }

void MCPServer::Notify_ResourceUpdated(const ResourceUpdatedNotification::Params& InParams)
{
	SendMessage(ResourceUpdatedNotification(InParams));
}

OptTask<ListRootsResponse::Result> MCPServer::Request_ListRoots(const PaginatedRequestParams& InParams)
{
	if (auto Response = std::move(co_await SendRequest<ListRootsResponse>(ListRootsRequest{ InParams }));
		Response.Result())
	{
		co_return Response.Result().value();
	}
	co_return std::nullopt;
}

void MCPServer::OnRequest_ListResources(const ListResourcesRequest& InRequest)
{
	try
	{
		const auto Request = GetRequestParams<PaginatedRequestParams>(InRequest);

		const ListResourcesResponse::Result Result = m_ResourceManager->ListResources(Request.value());

		SendMessage(ListResourcesResponse(InRequest.GetRequestID(), Result));
	}
	catch (const std::exception& Except)
	{
		SendMessage(ErrorInternalError(InRequest.GetRequestID(), Except.what()));
	}
}

void MCPServer::OnRequest_ReadResource(const ReadResourceRequest& InRequest)
{
	try
	{
		const auto Request = GetRequestParams<ReadResourceRequest::Params>(InRequest).value();

		// TODO: @HalcyonOmega - Relook logic here
		// Use ResourceManager to read the resource content
		std::variant<TextResourceContents, BlobResourceContents> ResourceContent
			= m_ResourceManager->GetResource(Request->URI).value();

		ReadResourceResponse::Result ResponseResult;
		ResponseResult.Contents.emplace_back(ResourceContent);

		SendMessage(ReadResourceResponse(InRequest.GetRequestID(), ResponseResult));
	}
	catch (const std::exception& Except)
	{
		SendMessage(ErrorInternalError(InRequest.GetRequestID(), Except.what()));
	}
}

void MCPServer::OnRequest_SubscribeResource(const SubscribeRequest& InRequest)
{
	try
	{
		const auto Request = GetRequestParams<SubscribeRequest::Params>(InRequest).value();

		std::string_view ClientID = GetCurrentClientID();

		// Validate resource exists
		if (!m_ResourceManager->HasResource(Request->URI))
		{
			SendMessage(ErrorInvalidRequest(InRequest.GetRequestID(),
				"Resource not found",
				JSONData::object({ { "uri", Request->URI.toString() } })));
			return;
		}

		// Add a subscription with proper client tracking
		m_ResourceManager->AddResourceSubscription(Request, std::string(ClientID));

		// Send an empty response to indicate success
		SendMessage(EmptyResponse{ InRequest.GetRequestID() });
	}
	catch (const std::exception& Except)
	{
		SendMessage(ErrorInternalError(InRequest.GetRequestID(), Except.what()));
	}
}

void MCPServer::OnRequest_UnsubscribeResource(const UnsubscribeRequest& InRequest)
{
	try
	{
		const auto Request = GetRequestParams<UnsubscribeRequest::Params>(InRequest).value();
		const std::string_view ClientID = GetCurrentClientID();

		m_ResourceManager->RemoveResourceSubscription(Request, std::string(ClientID));
		SendMessage(EmptyResponse{ InRequest.GetRequestID() });
	}
	catch (const std::exception& Except)
	{
		SendMessage(ErrorInternalError(InRequest.GetRequestID(), Except.what()));
	}
}

void MCPServer::OnNotified_RootsListChanged(const RootsListChangedNotification& InNotification)
{
	// TODO: @HalcyonOmega
	(void)InNotification;
}

void MCPServer::Notify_LogMessage(const LoggingMessageNotification::Params& InParams)
{
	SendMessage(LoggingMessageNotification(InParams));
}

OptTask<CreateMessageResponse::Result> MCPServer::Request_CreateMessage(const CreateMessageRequest::Params& InParams)
{
	if (auto Response = std::move(co_await SendRequest<CreateMessageResponse>(CreateMessageRequest{ InParams }));
		Response.Result())
	{
		co_return Response.Result().value();
	}
	co_return std::nullopt;
}

void MCPServer::OnRequest_Complete(const CompleteRequest& InRequest)
{
	try
	{
		const CompleteRequest::Params* Request = GetRequestParams<CompleteRequest::Params>(InRequest).value();

		if (!m_CompletionHandler)
		{
			SendMessage(ErrorMethodNotFound(InRequest.GetRequestID(), "Completion not supported"));
		}

		(void)Request;
		// Call handler
		// auto response = m_CompletionHandler(request);
		// SendMessage(InRequestID, JSONData(response));

		// TODO: Implement actual completion handler call
	}
	catch (const std::exception& Except)
	{
		SendMessage(ErrorInternalError(InRequest.GetRequestID(), Except.what()));
	}
}

void MCPServer::Notify_Progress(const ProgressNotification::Params& InParams)
{
	SendMessage(ProgressNotification(InParams));
}

void MCPServer::Notify_CancelRequest(const CancelledNotification::Params& InParams)
{
	SendMessage(CancelledNotification(InParams));
}

void MCPServer::OnNotified_Progress(const ProgressNotification& InNotification)
{
	// Ensure this method requires instance access
	if (!IsInitialized())
	{
		return;
	}

	const auto Params = GetNotificationParams<ProgressNotification::Params>(InNotification).value();

	// TODO: Implement progress notification handling
	(void)Params;
}

void MCPServer::OnNotified_CancelRequest(const CancelledNotification& InNotification)
{
	const auto Params = GetNotificationParams<CancelledNotification::Params>(InNotification).value();
	m_MessageManager->UnregisterResponseHandler(Params->CancelRequestID);
}

void MCPServer::SetHandlers()
{
	// Register request handlers using the RegisterRequestHandler method
	m_MessageManager->RegisterRequestHandler<InitializeRequest>(
		[this](const auto& Request) { OnRequest_Initialize(Request); });
	m_MessageManager->RegisterRequestHandler<ListToolsRequest>(
		[this](const auto& Request) { OnRequest_ListTools(Request); });
	m_MessageManager->RegisterRequestHandler<CallToolRequest>(
		[this](const auto& Request) { OnRequest_CallTool(Request); });
	m_MessageManager->RegisterRequestHandler<ListPromptsRequest>(
		[this](const auto& Request) { OnRequest_ListPrompts(Request); });
	m_MessageManager->RegisterRequestHandler<GetPromptRequest>(
		[this](const auto& Request) { OnRequest_GetPrompt(Request); });
	m_MessageManager->RegisterRequestHandler<ListResourcesRequest>(
		[this](const auto& Request) { OnRequest_ListResources(Request); });
	m_MessageManager->RegisterRequestHandler<ReadResourceRequest>(
		[this](const auto& Request) { OnRequest_ReadResource(Request); });
	m_MessageManager->RegisterRequestHandler<SubscribeRequest>(
		[this](const auto& Request) { OnRequest_SubscribeResource(Request); });
	m_MessageManager->RegisterRequestHandler<UnsubscribeRequest>(
		[this](const auto& Request) { OnRequest_UnsubscribeResource(Request); });
	m_MessageManager->RegisterRequestHandler<CompleteRequest>(
		[this](const auto& Request) { OnRequest_Complete(Request); });
}

// Enhanced resource change notification
void MCPServer::Notify_ResourceSubscribers(const ResourceUpdatedNotification::Params& InParams)
{
	// TODO: @HalcyonOmega - Get Actual Subscribers

	if (const auto Subscribers = m_ResourceManager->GetSubscribers(InParams.URI))
	{
		// Send notification to all subscribers
		SendMessage(ResourceUpdatedNotification{ InParams }, Subscribers.value());
	}
}

// Client identification helper (simplified - in production would use transport session data)
// TODO: @HalcyonOmega - Implement this
std::string_view MCPServer::GetCurrentClientID() const
{
	// For now, return a default client ID
	// In a real implementation this would extract the client ID from the current
	// transport session
	return "default_client";
}

VoidTask MCPServer::UpdateProgress(const double InProgress, std::optional<int64_t> InTotal)
{
	try
	{
		ProgressNotification::Params Params;
		Params.ProgressToken = ProgressToken(std::string("current_request"));
		Params.Progress = InProgress;
		if (InTotal.has_value())
		{
			Params.Total = *InTotal;
		}

		SendMessage(ProgressNotification(Params));
	}
	catch (const std::exception&)
	{
		// Ignore progress reporting errors to not break the main operation
	}

	co_return;
}

VoidTask MCPServer::CompleteProgress()
{
	co_await UpdateProgress(1.0);
	co_return;
}

MCP_NAMESPACE_END