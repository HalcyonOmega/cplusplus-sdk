#include "CoreSDK/Core/MCPClient.h"

#include "CoreSDK/Transport/ITransport.h"

#define CO_RETURN_IF_CLIENT_NOT_CONNECTED           \
	if (!IsConnected())                             \
	{                                               \
		HandleRuntimeError("Client not connected"); \
		co_return;                                  \
	}

#define RETURN_IF_CLIENT_NOT_CONNECTED              \
	if (!IsConnected())                             \
	{                                               \
		HandleRuntimeError("Client not connected"); \
		return;                                     \
	}

#define SEND_REQUEST_RETURN_RESULT(ResponseType, Request)                                      \
	CO_RETURN_IF_CLIENT_NOT_CONNECTED                                                          \
	if (const auto& Response = co_await SendRequest<ResponseType>(Request); Response.Result()) \
	{                                                                                          \
		co_return Response.Result().value();                                                   \
	}                                                                                          \
	co_return std::nullopt;

MCP_NAMESPACE_BEGIN

MCPClient::MCPClient(const TransportType InTransportType, std::optional<std::unique_ptr<TransportOptions>> InOptions,
	const Implementation& InClientInfo, const ClientCapabilities& InCapabilities) :
	MCPProtocol(TransportFactory::CreateTransport(InTransportType, TransportSide::Client, std::move(InOptions)), true)
{
	SetClientInfo(InClientInfo);
	SetClientCapabilities(InCapabilities);
}

VoidTask MCPClient::Start()
{
	CO_RETURN_IF_CLIENT_NOT_CONNECTED

	try
	{
		std::optional<InitializeResponse::Result> Result = co_await Request_Initialize(InitializeRequest::Params{
			.ProtocolVersion = m_ClientInfo.Version,
			.Capabilities = m_ClientCapabilities,
			.ClientInfo = m_ClientInfo,
		});

		m_Transport->SetState(TransportState::Connected);
	}
	catch (const std::exception& e)
	{
		HandleRuntimeError("Failed to connect: " + std::string(e.what()));
		co_return;
	}

	co_return;
}

VoidTask MCPClient::Stop()
{
	CO_RETURN_IF_CLIENT_NOT_CONNECTED

	try
	{
		co_await m_Transport->Disconnect();
		m_Transport->SetState(TransportState::Disconnected);
	}
	catch (const std::exception& Except)
	{
		HandleRuntimeError("Failed to disconnect: " + std::string(Except.what()));
		co_return;
	}

	co_return;
}

OptTask<InitializeResponse::Result> MCPClient::Request_Initialize(const InitializeRequest::Params& InParams)
{
	if (IsInitialized())
	{
		HandleRuntimeError("Protocol already initialized");
		co_return std::nullopt;
	}

	try
	{
		SetState(MCPProtocolState::Initializing);
		// Start transport
		co_await m_Transport->Connect();

		const auto& Response = co_await SendRequest<InitializeResponse>(InitializeRequest{ InParams });
		if (Response.Result())
		{
			const auto& Result = Response.Result().value();
			// Store negotiated capabilities
			SetServerCapabilities(Result.Capabilities);
			SetServerInfo(Result.ServerInfo);
			SetState(MCPProtocolState::Initialized);
			co_return Result;
		}
		if (Response.Raw())
		{
			const auto& UnexpectedType = Response.Raw().value();
			HandleRuntimeError(UnexpectedType.dump());
		};
	}
	catch (const std::exception& e)
	{
		HandleRuntimeError("Failed to initialize protocol: " + std::string(e.what()));
	}
	co_return std::nullopt;
}

void MCPClient::OnNotified_Initialized(const InitializedNotification& InNotification) {}

OptTask<ListToolsResponse::Result> MCPClient::Request_ListTools(const PaginatedRequestParams& InParams){
	SEND_REQUEST_RETURN_RESULT(ListToolsResponse, ListToolsRequest{ InParams })
}

OptTask<CallToolResponse::Result> MCPClient::Request_CallTool(const CallToolRequest::Params& InParams)
{
	SEND_REQUEST_RETURN_RESULT(CallToolResponse, CallToolRequest{ InParams })
}

void MCPClient::OnNotified_ToolListChanged(const ToolListChangedNotification& InNotification) {}

OptTask<ListPromptsResponse::Result> MCPClient::Request_ListPrompts(const PaginatedRequestParams& InParams){
	SEND_REQUEST_RETURN_RESULT(ListPromptsResponse, ListPromptsRequest{ InParams })
}

OptTask<GetPromptResponse::Result> MCPClient::Request_GetPrompt(const GetPromptRequest::Params& InParams)
{
	SEND_REQUEST_RETURN_RESULT(GetPromptResponse, GetPromptRequest{ InParams })
}

void MCPClient::OnNotified_PromptListChanged(const PromptListChangedNotification& InNotification) {}

OptTask<ListResourcesResponse::Result> MCPClient::Request_ListResources(const PaginatedRequestParams& InParams){
	SEND_REQUEST_RETURN_RESULT(ListResourcesResponse, ListResourcesRequest{ InParams })
}

OptTask<ReadResourceResponse::Result> MCPClient::Request_ReadResource(const ReadResourceRequest::Params& InParams){
	SEND_REQUEST_RETURN_RESULT(ReadResourceResponse, ReadResourceRequest{ InParams })
}

VoidTask MCPClient::Request_Subscribe(const SubscribeRequest::Params& InParams)
{
	CO_RETURN_IF_CLIENT_NOT_CONNECTED

	if (const auto& Response = co_await SendRequest<EmptyResponse>(SubscribeRequest{ InParams }); Response.Get())
	{
		Logger::Notice("Subscribe Successful");
		co_return;
	}

	Logger::Emergency("Subscribe Failed");
	co_return;
}

VoidTask MCPClient::Request_Unsubscribe(const UnsubscribeRequest::Params& InParams)
{
	CO_RETURN_IF_CLIENT_NOT_CONNECTED

	if (const auto& Response = co_await SendRequest<EmptyResponse>(UnsubscribeRequest{ InParams }); Response.Get())
	{
		Logger::Notice("Unsubscribe Successful");
		co_return;
	}

	Logger::Emergency("Unsubscribe Failed");
	co_return;
}

void MCPClient::OnNotified_ResourceListChanged(const ResourceListChangedNotification& InNotification) {}

void MCPClient::OnNotified_ResourceUpdated(const ResourceUpdatedNotification& InNotification) {}

OptTask<ListRootsResponse::Result> MCPClient::Request_ListRoots(const PaginatedRequestParams& InParams)
{
	SEND_REQUEST_RETURN_RESULT(ListRootsResponse, ListRootsRequest{ InParams })
}

void MCPClient::OnNotified_RootsListChanged(const RootsListChangedNotification& InNotification) {}

VoidTask MCPClient::Request_SetLoggingLevel(const SetLevelRequest::Params& InParams)
{
	CO_RETURN_IF_CLIENT_NOT_CONNECTED

	if (const auto& Response = co_await SendRequest<EmptyResponse>(SetLevelRequest{ InParams }); Response.Get())
	{
		Logger::Notice("Set Logging Level Successful");
		co_return;
	}

	Logger::Error("Set Logging Level Failed");
	co_return;
}

void MCPClient::OnNotified_LogMessage(const LoggingMessageNotification& InNotification) {}

void MCPClient::OnRequest_CreateMessage(const CreateMessageRequest& InRequest)
{
	RETURN_IF_CLIENT_NOT_CONNECTED

	SendMessage(CreateMessageResponse{ InRequest.GetRequestID() });
}

OptTask<CompleteResponse::Result> MCPClient::Request_Complete(const CompleteRequest::Params& InParams)
{
	SEND_REQUEST_RETURN_RESULT(CompleteResponse, CompleteRequest{ InParams })
}

void MCPClient::Notify_Progress(const ProgressNotification::Params& InParams)
{
	return SendMessage(ProgressNotification{ InParams });
}

void MCPClient::Notify_CancelRequest(const CancelledNotification::Params& InParams)
{
	return SendMessage(CancelledNotification{ InParams });
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
	if (const auto& Data = GetNotificationParams<CancelledNotification::Params>(InNotification))
	{
		if (const bool Success = m_MessageManager->UnregisterResponseHandler(Data.value().CancelRequestID); !Success)
		{
			Logger::Error("Failed to cancel request: " + Data->CancelRequestID.ToString());
			return;
		}
		Logger::Notice("Cancelled request: " + Data->CancelRequestID.ToString());
	}

	Logger::Error("Invalid cancel request");
}

MCP_NAMESPACE_END