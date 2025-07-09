#include "CoreSDK/Core/MCPClient.h"

#include "CoreSDK/Transport/ITransport.h"

#define RETURN_IF_CLIENT_NOT_CONNECTED              \
	if (!IsConnected())                             \
	{                                               \
		HandleRuntimeError("Client not connected"); \
		co_return;                                  \
	}

MCP_NAMESPACE_BEGIN

MCPClient::MCPClient(const TransportType InTransportType, std::optional<std::unique_ptr<TransportOptions>> InOptions,
	const Implementation& InClientInfo, const ClientCapabilities& InCapabilities) :
	MCPProtocol(TransportFactory::CreateTransport(InTransportType, TransportSide::Client, std::move(InOptions)), true)
{
	SetClientInfo(InClientInfo);
	SetClientCapabilities(InCapabilities);
}

MCPTask_Void MCPClient::Start()
{
	RETURN_IF_CLIENT_NOT_CONNECTED

	try
	{
		InitializeResponse::Result Result = co_await Request_Initialize();
		IsConnected() = true;
		m_ClientInfo = InClientInfo;
	}
	catch (const std::exception& e)
	{
		HandleRuntimeError("Failed to connect: " + std::string(e.what()));
		co_return;
	}

	co_return;
}

MCPTask_Void MCPClient::Stop()
{
	if (!IsConnected())
	{
		co_return;
	}

	try
	{
		co_await m_Transport->Disconnect();
		IsConnected() = false;
	}
	catch (const std::exception& Except)
	{
		HandleRuntimeError("Failed to disconnect: " + std::string(Except.what()));
		co_return;
	}

	co_return;
}

MCPTask<InitializeResponse::Result> MCPClient::Request_Initialize(const InitializeRequest::Params& InParams)
{
	if (IsInitialized())
	{
		HandleRuntimeError("Protocol already initialized");
		co_return;
	}

	try
	{
		SetState(MCPProtocolState::Initializing);

		// Start transport
		co_await m_Transport->Connect();

		// Send initialize request
		const InitializeRequest Request{ InitializeRequest::Params{
			.ProtocolVersion = m_ClientInfo.Version,
			.Capabilities = m_ClientCapabilities,
			.ClientInfo = m_ClientInfo,
		} };

		const auto& Response = co_await SendRequest<InitializeResponse>(Request);

		Response.HandleResponse(
			[this](const auto& Expected)
			{
				const auto& Result = GetResponseResult<InitializeResponse::Result>(Expected);

				// Store negotiated capabilities
				SetServerCapabilities(Result.Capabilities);
				SetServerInfo(Result.ServerInfo);

				SetState(MCPProtocolState::Initialized);
			},
			[]() {});
		if (const auto& Success = Response.Expected())
		{
		}
		else if (const auto& Error = Response.Unexpected())
		{
			HandleRuntimeError(Error.value().dump());
		}
	}
	catch (const std::exception& e)
	{
		HandleRuntimeError("Failed to initialize protocol: " + std::string(e.what()));
	}

	co_return;
}

void MCPClient::OnNotified_Initialized(const InitializedNotification& InNotification) {}

MCPTask<ListToolsResponse::Result> MCPClient::Request_ListTools(const PaginatedRequestParams& InParams)
{
	RETURN_IF_CLIENT_NOT_CONNECTED

	SendRequest(ListToolsRequest{ InParams });
	co_return const ListToolsResponse::Result Result = GetResponseResult<ListToolsResponse>();
}

MCPTask<CallToolResponse::Result> MCPClient::Request_CallTool(const CallToolRequest::Params& InParams)
{
	RETURN_IF_CLIENT_NOT_CONNECTED

	SendRequest(CallToolRequest{ InParams });
	co_return response.get<ToolCallResponse>();
}

void MCPClient::OnNotified_ToolListChanged(const ToolListChangedNotification& InNotification) {}

MCPTask<ListPromptsResponse::Result> MCPClient::Request_ListPrompts(const PaginatedRequestParams& InParams)
{
	RETURN_IF_CLIENT_NOT_CONNECTED

	SendRequest(ListPromptsRequest{ InParams });
	co_return response.get<ListPromptsResponse>();
}

MCPTask<GetPromptResponse::Result> MCPClient::Request_GetPrompt(const GetPromptRequest::Params& InParams)
{
	RETURN_IF_CLIENT_NOT_CONNECTED

	co_await SendRequest(GetPromptRequest{ InParams });
	co_return response.get<PromptGetResponse>();
}

void MCPClient::OnNotified_PromptListChanged(const PromptListChangedNotification& InNotification) {}

MCPTask<ListResourcesResponse::Result> MCPClient::Request_ListResources(const PaginatedRequestParams& InParams)
{
	RETURN_IF_CLIENT_NOT_CONNECTED

	SendRequest(ListResourcesRequest{ InParams });
	co_return response.get<ResourceListResponse>();
}

MCPTask<ReadResourceResponse::Result> MCPClient::Request_ReadResource(const ReadResourceRequest::Params& InParams)
{
	RETURN_IF_CLIENT_NOT_CONNECTED

	co_return co_await SendRequest(ReadResourceRequest{ InParams });
}

MCPTask_Void MCPClient::Request_Subscribe(const SubscribeRequest::Params& InParams)
{
	RETURN_IF_CLIENT_NOT_CONNECTED

	co_return co_await SendRequest(SubscribeRequest{ InParams });
}

MCPTask_Void MCPClient::Request_Unsubscribe(const UnsubscribeRequest::Params& InParams)
{
	RETURN_IF_CLIENT_NOT_CONNECTED

	co_return co_await SendRequest(UnsubscribeRequest{ InParams });
}

void MCPClient::OnNotified_ResourceListChanged(const ResourceListChangedNotification& InNotification) {}

void MCPClient::OnNotified_ResourceUpdated(const ResourceUpdatedNotification& InNotification) {}

MCPTask<ListRootsResponse::Result> MCPClient::Request_ListRoots(const PaginatedRequestParams& InParams)
{
	RETURN_IF_CLIENT_NOT_CONNECTED

	co_return co_await SendRequest(ListRootsRequest{ InParams });
}

void MCPClient::OnNotified_RootsListChanged(const RootsListChangedNotification& InNotification) {}

MCPTask_Void MCPClient::Request_SetLoggingLevel(const SetLevelRequest::Params& InParams)
{
	RETURN_IF_CLIENT_NOT_CONNECTED

	co_return co_await SendRequest(SetLevelRequest{ InParams });
}

void MCPClient::OnNotified_LogMessage(const LoggingMessageNotification& InNotification) {}

void MCPClient::OnRequest_CreateMessage(const CreateMessageRequest& InRequest)
{
	RETURN_IF_CLIENT_NOT_CONNECTED

	SendResponse(CreateMessageResponse{ InRequest.GetRequestID() });
	return;
}

MCPTask<CompleteResponse::Result> MCPClient::Request_Complete(const CompleteRequest::Params& InParams)
{
	RETURN_IF_CLIENT_NOT_CONNECTED

	auto response = co_await SendRequest(CompleteRequest{ InParams });
	co_return response.get<CompleteResponse>();
}

MCPTask_Void MCPClient::Notify_Progress(const ProgressNotification::Params& InParams)
{
	return SendNotification(ProgressNotification{ InParams });
}

MCPTask_Void MCPClient::Notify_CancelRequest(const CancelledNotification::Params& InParams)
{
	return SendNotification(CancelledNotification{ InParams });
}

void MCPClient::OnNotified_Progress(const ProgressNotification& InNotification)
{
	const std::optional<ProgressNotification::Params> Data
		= GetNotificationParams<ProgressNotification::Params>(InNotification);

	if (Data)
	{
		Logger::Notice(std::string("Progress Notification: ") + "\n	Progress Token: "
			+ Data.value().ProgressToken.ToString() + "\n	Progress: " + std::to_string(Data.value().Progress)
			+ "\n	Message: " + Data.value().Message.value());
	}

	Logger::Error("Invalid progress notification");
}

void MCPClient::OnNotified_CancelRequest(const CancelledNotification& InNotification)
{
	const std::optional<CancelledNotification::Params> Data
		= GetNotificationParams<CancelledNotification::Params>(InNotification).value();
	if (Data)
	{
		if (const bool Success = m_MessageManager->CancelRequest(Data->CancelRequestID); !Success)
		{
			Logger::Error("Failed to cancel request: " + Data->CancelRequestID.ToString());
			return;
		}
		Logger::Notice("Cancelled request: " + Data->CancelRequestID.ToString());
	}

	Logger::Error("Invalid cancel request");
}

MCP_NAMESPACE_END