#include "CoreSDK/Core/MCPServer.h"

#include "CoreSDK/Common/Content.h"
#include "CoreSDK/Common/Progress.h"
#include "CoreSDK/Messages/MCPMessages.h"

MCP_NAMESPACE_BEGIN

MCPServer::MCPServer(const TransportType InTransportType, std::optional<std::unique_ptr<TransportOptions>> InOptions,
	const Implementation& InServerInfo, const ServerCapabilities& InCapabilities) :
	MCPProtocol(TransportFactory::CreateTransport(InTransportType, TransportSide::Server, std::move(InOptions)), true)
{
	SetupDefaultHandlers();
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

		// Server doesn't need to call Initialize - it responds to client
		// initialization
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
		const InitializeRequest::Params Request = GetRequestParams<InitializeRequest::Params>(InRequest).value();

		// CRITICAL: Validate protocol version first
		static const std::vector<std::string> SUPPORTED_PROTOCOL_VERSIONS = { "2024-11-05", "2025-03-26" };

		if (const auto Iter = std::ranges::find(SUPPORTED_PROTOCOL_VERSIONS, Request.ProtocolVersion);
			Iter == SUPPORTED_PROTOCOL_VERSIONS.end())
		{
			std::string SupportedVersions;
			for (size_t Index = 0; Index < SUPPORTED_PROTOCOL_VERSIONS.size(); ++Index)
			{
				if (Index > 0)
				{
					SupportedVersions += ", ";
				}
				SupportedVersions += SUPPORTED_PROTOCOL_VERSIONS[Index];
			}

			SendMessage(ErrorResponseBase(InRequest.GetRequestID(),
				MCPError(ErrorCodes::INVALID_REQUEST,
					"Unsupported protocol version: " + Request.ProtocolVersion
						+ ". Supported versions: " + SupportedVersions)));
			return;
		}

		InitializeResponse::Result Result;
		Result.ProtocolVersion = Request.ProtocolVersion; // Use negotiated version
		Result.ServerInfo = m_ServerInfo;
		Result.Capabilities = m_ServerCapabilities;

		SendMessage(InitializeResponse{ InRequest.GetRequestID(), Result });
	}
	catch (const std::exception& Except)
	{
		SendMessage(ErrorResponseBase(InRequest.GetRequestID(), MCPError(ErrorCodes::INTERNAL_ERROR, Except.what())));
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
		SendMessage(ErrorResponseBase(InRequest.GetRequestID(), MCPError(ErrorCodes::INTERNAL_ERROR, Except.what())));
	}
}

void MCPServer::OnRequest_CallTool(const CallToolRequest& InRequest)
{
	try
	{
		CallToolRequest::Params Request = GetRequestParams<CallToolRequest::Params>(InRequest).value();
		Tool Tool = m_ToolManager->FindTool(Request.Name).value();

		// Use ToolManager to call the tool
		auto Result = m_ToolManager->CallTool(Request);

		CallToolResponse::Result ResponseResult;
		ResponseResult.Content = Result.Content;
		ResponseResult.IsError = Result.IsError;

		SendMessage(CallToolResponse(InRequest.GetRequestID(), ResponseResult));
	}
	catch (const std::exception& Except)
	{
		SendMessage(ErrorResponseBase(InRequest.GetRequestID(), MCPError(ErrorCodes::INTERNAL_ERROR, Except.what())));
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
		SendMessage(ErrorResponseBase(InRequest.GetRequestID(), MCPError(ErrorCodes::INTERNAL_ERROR, Except.what())));
	}
}

void MCPServer::OnRequest_GetPrompt(const GetPromptRequest& InRequest)
{
	try
	{
		const GetPromptRequest::Params Request = GetRequestParams<GetPromptRequest::Params>(InRequest).value();

		const GetPromptResponse::Result Result = m_PromptManager->GetPrompt(Request);

		SendMessage(GetPromptResponse(InRequest.GetRequestID(), Result));
	}
	catch (const std::exception& Except)
	{
		SendMessage(ErrorResponseBase(InRequest.GetRequestID(), MCPError(ErrorCodes::INTERNAL_ERROR, Except.what())));
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

bool MCPServer::AddResourceTemplate(
	const ResourceTemplate& InTemplate, const ResourceManager::ResourceFunction& InFunction)
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
		SendMessage(ErrorResponseBase(InRequest.GetRequestID(), MCPError(ErrorCodes::INTERNAL_ERROR, Except.what())));
	}
}

void MCPServer::OnRequest_ReadResource(const ReadResourceRequest& InRequest)
{
	try
	{
		const ReadResourceRequest::Params Request = GetRequestParams<ReadResourceRequest::Params>(InRequest).value();

		// TODO: @HalcyonOmega - Relook logic here
		// Use ResourceManager to read the resource content
		std::variant<TextResourceContents, BlobResourceContents> ResourceContent
			= m_ResourceManager->GetResource(Request.URI).value();

		ReadResourceResponse::Result ResponseResult;
		ResponseResult.Contents.emplace_back(ResourceContent);

		SendMessage(ReadResourceResponse(InRequest.GetRequestID(), ResponseResult));
	}
	catch (const std::exception& Except)
	{
		SendMessage(ErrorResponseBase(InRequest.GetRequestID(), MCPError(ErrorCodes::INTERNAL_ERROR, Except.what())));
	}
}

void MCPServer::OnRequest_SubscribeResource(const SubscribeRequest& InRequest)
{
	try
	{
		const SubscribeRequest::Params Request = GetRequestParams<SubscribeRequest::Params>(InRequest).value();

		std::string_view ClientID = GetCurrentClientID();

		// Validate resource exists
		if (!m_ResourceManager->HasResource(Request.URI))
		{
			SendMessage(ErrorResponseBase(InRequest.GetRequestID(),
				MCPError(ErrorCodes::INVALID_REQUEST, "Resource not found",
					JSONData::object({ { "uri", Request.URI.toString() } }))));
			return;
		}

		// Add a subscription with proper client tracking
		{
			std::lock_guard<std::mutex> Lock(m_ResourceSubscriptionsMutex);
			m_ResourceSubscriptions[Request.URI.toString()].emplace_back(ClientID);
		}

		// Send an empty response to indicate success
		SendMessage(EmptyResponse{ InRequest.GetRequestID() });
	}
	catch (const std::exception& Except)
	{
		SendMessage(ErrorResponseBase(InRequest.GetRequestID(), MCPError(ErrorCodes::INTERNAL_ERROR, Except.what())));
	}
}

void MCPServer::OnRequest_UnsubscribeResource(const UnsubscribeRequest& InRequest)
{
	try
	{
		const UnsubscribeRequest::Params Request = GetRequestParams<UnsubscribeRequest::Params>(InRequest).value();

		const std::string_view ClientID = GetCurrentClientID();

		// Remove from subscriptions
		{
			std::lock_guard<std::mutex> Lock(m_ResourceSubscriptionsMutex);
			if (const auto Iter = m_ResourceSubscriptions.find(Request.URI.toString());
				Iter != m_ResourceSubscriptions.end())
			{
				auto& Subscribers = Iter->second;
				std::erase(Subscribers, std::string(ClientID));
				// Clean up empty subscription sets
				if (Subscribers.empty())
				{
					m_ResourceSubscriptions.erase(Iter);
				}
			}
		}

		// Send empty response
		SendMessage(EmptyResponse{ InRequest.GetRequestID() });
	}
	catch (const std::exception& Except)
	{
		SendMessage(ErrorResponseBase(InRequest.GetRequestID(), MCPError(ErrorCodes::INTERNAL_ERROR, Except.what())));
	}
}

bool MCPServer::AddRoot(const Root& InRoot)
{
	if (!m_RootManager->AddRoot(InRoot))
	{
		HandleRuntimeError("Failed to add root: " + InRoot.Name.value());
		return false;
	}

	Notify_RootsListChanged();
	return true;
}

bool MCPServer::RemoveRoot(const Root& InRoot)
{
	if (!m_RootManager->RemoveRoot(InRoot))
	{
		HandleRuntimeError("Failed to remove root: " + InRoot.Name.value());
		return false;
	}

	Notify_RootsListChanged();
	return true;
}

void MCPServer::Notify_RootsListChanged() { SendMessage(RootsListChangedNotification()); }

void MCPServer::Notify_LogMessage(const LoggingMessageNotification::Params& InParams)
{
	SendMessage(LoggingMessageNotification(InParams));
}

Task<CreateMessageResponse::Result> MCPServer::Request_CreateMessage(const CreateMessageRequest::Params& InParams)
{
	const auto& Response = SendRequest<CreateMessageResponse>(CreateMessageRequest(InParams));
	co_return GetResponseResult<CreateMessageResponse::Result>(Response.Get());
}

void MCPServer::OnRequest_Complete(const CompleteRequest& InRequest)
{
	try
	{
		CompleteRequest::Params Request = GetRequestParams<CompleteRequest::Params>(InRequest).value();

		if (!m_CompletionHandler)
		{
			SendMessage(ErrorResponseBase(
				InRequest.GetRequestID(), MCPError(ErrorCodes::METHOD_NOT_FOUND, "Completion not supported")));
			return;
		}

		// Call handler
		// auto response = m_CompletionHandler(request);
		// SendMessage(InRequestID, JSONData(response));

		// TODO: Implement actual completion handler call
	}
	catch (const std::exception& Except)
	{
		SendMessage(ErrorResponseBase(InRequest.GetRequestID(), MCPError(ErrorCodes::INTERNAL_ERROR, Except.what())));
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

	const ProgressNotification::Params Params
		= GetNotificationParams<ProgressNotification::Params>(InNotification).value();

	// TODO: Implement progress notification handling
	(void)Params;
}

void MCPServer::OnNotified_CancelRequest(const CancelledNotification& InNotification)
{
	// Ensure this method requires instance access
	if (!IsInitialized())
	{
		return;
	}

	(void)InNotification;
	// TODO: Implement cancel request handling
}

void MCPServer::SetupDefaultHandlers()
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
	std::vector<std::string> Subscribers;

	std::lock_guard<std::mutex> Lock(m_ResourceSubscriptionsMutex);

	if (const auto Iter = m_ResourceSubscriptions.find(InParams.URI.toString()); Iter != m_ResourceSubscriptions.end())
	{
		Subscribers = Iter->second;
	}

	if (!Subscribers.empty())
	{
		// Send notification to all subscribers
		SendMessage(ResourceUpdatedNotification{ InParams }, Subscribers);
	}
}

// Client identification helper (simplified - in production would use transport
// session data)
// TODO: @HalcyonOmega - Implement this
std::string_view MCPServer::GetCurrentClientID() const
{
	// For now, return a default client ID
	// In a real implementation this would extract the client ID from the current
	// transport session
	return "default_client";
}

// Send notification to specific client (simplified implementation)
// TODO: @HalcyonOmega - Implement this
void MCPServer::SendMessageToClient(
	const std::string_view InClientID, const ResourceUpdatedNotification& InNotification)
{
	(void)InClientID;
	// For now, send to all clients via the protocol
	// In a real implementation this would route to the specific client
	SendMessage(InNotification);
}

// Enhanced tool execution with progress reporting
Task<CallToolResponse> MCPServer::ExecuteToolWithProgress(const Tool& InTool,
	const std::optional<std::unordered_map<std::string, JSONData>>& InArguments, const RequestID& InRequestID)
{
	// Update progress at 0%
	co_await UpdateProgress(0.0);

	// Use ToolManager to execute a tool
	const auto Result = m_ToolManager->CallTool(InTool, InArguments);

	// Complete progress
	co_await UpdateProgress(1.0);

	CallToolResponse Response(InRequestID, Result);
	co_return Response;
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