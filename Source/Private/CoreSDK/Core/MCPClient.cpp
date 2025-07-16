#include "CoreSDK/Core/MCPClient.h"

#include "CoreSDK/Transport/ITransport.h"

#define CO_RETURN_VOID_IF_CLIENT_NOT_CONNECTED      \
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

#define SEND_REQUEST_RETURN_RESULT(ResponseType, Request)                                          \
	if (!IsConnected())                                                                            \
	{                                                                                              \
		HandleRuntimeError("Client not connected");                                                \
		co_return std::nullopt;                                                                    \
	}                                                                                              \
	if (auto Response = std::move(co_await SendRequest<ResponseType>(Request)); Response.Result()) \
	{                                                                                              \
		co_return Response.Result().value();                                                       \
	}                                                                                              \
	co_return std::nullopt;

MCP_NAMESPACE_BEGIN

MCPClient::MCPClient(const ETransportType InTransportType,
	const std::optional<std::unique_ptr<TransportOptions>>& InOptions,
	const Implementation& InClientInfo,
	const ClientCapabilities& InCapabilities)
	: MCPProtocol(TransportFactory::CreateTransport(InTransportType, ETransportSide::Client, InOptions), true)
{
	SetClientInfo(InClientInfo);
	SetClientCapabilities(InCapabilities);
}

VoidTask MCPClient::Start()
{
	CO_RETURN_VOID_IF_CLIENT_NOT_CONNECTED

	try
	{
		const InitializeRequest::Params InitParams{
			m_ClientInfo.ProtocolVersion,
			m_ClientCapabilities,
			m_ClientInfo,
		};

		const auto Result = co_await Request_Initialize(InitParams);

		m_Transport->SetState(ETransportState::Connected);
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
	CO_RETURN_VOID_IF_CLIENT_NOT_CONNECTED

	try
	{
		co_await m_Transport->Disconnect();
		m_Transport->SetState(ETransportState::Disconnected);
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
		SetState(EProtocolState::Initializing);
		// Start transport
		co_await m_Transport->Connect();

		auto Response = std::move(co_await SendRequest<InitializeResponse>(InitializeRequest{ InParams }));
		if (Response.Result())
		{
			const auto Result = Response.Result().value();
			// Store negotiated capabilities
			SetServerCapabilities(Result.Capabilities);
			SetServerInfo(Result.ServerInfo);
			SetState(EProtocolState::Initialized);
			co_return Result;
		}
		if (Response.Raw())
		{
			const auto UnexpectedType = Response.Raw().value();
			HandleRuntimeError(UnexpectedType.dump());
		}
	}
	catch (const std::exception& e)
	{
		HandleRuntimeError("Failed to initialize protocol: " + std::string(e.what()));
	}
	co_return std::nullopt;
}

void MCPClient::OnNotified_Initialized(const InitializedNotification& InNotification)
{
	// TODO: @HalcyonOmega
	(void)InNotification;
}

OptTask<ListToolsResponse::Result> MCPClient::Request_ListTools(const PaginatedRequestParams& InParams){
	SEND_REQUEST_RETURN_RESULT(ListToolsResponse, ListToolsRequest{ InParams })
}

OptTask<CallToolResponse::Result> MCPClient::Request_CallTool(const CallToolRequest::Params& InParams)
{
	SEND_REQUEST_RETURN_RESULT(CallToolResponse, CallToolRequest{ InParams })
}

void MCPClient::OnNotified_ToolListChanged(const ToolListChangedNotification& InNotification)
{
	// TODO: @HalcyonOmega
	(void)InNotification;
}

OptTask<ListPromptsResponse::Result> MCPClient::Request_ListPrompts(const PaginatedRequestParams& InParams){
	SEND_REQUEST_RETURN_RESULT(ListPromptsResponse, ListPromptsRequest{ InParams })
}

OptTask<GetPromptResponse::Result> MCPClient::Request_GetPrompt(const GetPromptRequest::Params& InParams)
{
	SEND_REQUEST_RETURN_RESULT(GetPromptResponse, GetPromptRequest{ InParams })
}

void MCPClient::OnNotified_PromptListChanged(const PromptListChangedNotification& InNotification)
{
	// TODO: @HalcyonOmega
	(void)InNotification;
}

OptTask<ListResourcesResponse::Result> MCPClient::Request_ListResources(const PaginatedRequestParams& InParams){
	SEND_REQUEST_RETURN_RESULT(ListResourcesResponse, ListResourcesRequest{ InParams })
}

OptTask<ReadResourceResponse::Result> MCPClient::Request_ReadResource(const ReadResourceRequest::Params& InParams){
	SEND_REQUEST_RETURN_RESULT(ReadResourceResponse, ReadResourceRequest{ InParams })
}

VoidTask MCPClient::Request_Subscribe(const SubscribeRequest::Params& InParams)
{
	CO_RETURN_VOID_IF_CLIENT_NOT_CONNECTED

	if (auto Response = std::move(co_await SendRequest<EmptyResponse>(SubscribeRequest{ InParams })); Response.Get())
	{
		Logger::Notice("Subscribe Successful");
		co_return;
	}

	Logger::Emergency("Subscribe Failed");
	co_return;
}

VoidTask MCPClient::Request_Unsubscribe(const UnsubscribeRequest::Params& InParams)
{
	CO_RETURN_VOID_IF_CLIENT_NOT_CONNECTED

	if (auto Response = std::move(co_await SendRequest<EmptyResponse>(UnsubscribeRequest{ InParams })); Response.Get())
	{
		Logger::Notice("Unsubscribe Successful");
		co_return;
	}

	Logger::Emergency("Unsubscribe Failed");
	co_return;
}

void MCPClient::OnNotified_ResourceListChanged(const ResourceListChangedNotification& InNotification)
{
	// TODO: @HalcyonOmega
	(void)InNotification;
}

void MCPClient::OnNotified_ResourceUpdated(const ResourceUpdatedNotification& InNotification)
{
	// TODO: @HalcyonOmega
	(void)InNotification;
}

bool MCPClient::AddRoot(const Root& InRoot)
{
	if (!m_RootManager->AddRoot(InRoot))
	{
		HandleRuntimeError("Failed to add root: " + InRoot.Name.value());
		return false;
	}

	Notify_RootsListChanged();
	return true;
}

bool MCPClient::RemoveRoot(const Root& InRoot)
{
	if (!m_RootManager->RemoveRoot(InRoot))
	{
		HandleRuntimeError("Failed to remove root: " + InRoot.Name.value());
		return false;
	}

	Notify_RootsListChanged();
	return true;
}

void MCPClient::Notify_RootsListChanged() { SendMessage(RootsListChangedNotification()); }

VoidTask MCPClient::Request_SetLoggingLevel(const SetLevelRequest::Params& InParams)
{
	CO_RETURN_VOID_IF_CLIENT_NOT_CONNECTED

	if (auto Response = std::move(co_await SendRequest<EmptyResponse>(SetLevelRequest{ InParams })); Response.Get())
	{
		Logger::Notice("Set Logging Level Successful");
		co_return;
	}

	Logger::Error("Set Logging Level Failed");
	co_return;
}

void MCPClient::OnNotified_LogMessage(const LoggingMessageNotification& InNotification)
{
	// TODO: @HalcyonOmega
	(void)InNotification;
}

void MCPClient::OnRequest_ListRoots(const ListRootsRequest& InRequest)
{
	if (m_RootManager)
	{
		SendMessage(ListRootsResponse(InRequest.GetRequestID(), m_RootManager->ListRoots()));
	}
}

void MCPClient::OnRequest_CreateMessage(const CreateMessageRequest& InRequest)
{
	RETURN_IF_CLIENT_NOT_CONNECTED

	if (const auto RequestParams = GetRequestParams<CreateMessageRequest::Params>(InRequest))
	{
		const CreateMessageResponse::Result Result = m_SamplingManager->CreateMessage(RequestParams.value());
		SendMessage(CreateMessageResponse{ InRequest.GetRequestID(), Result });
		return;
	}
	SendMessage(ErrorInvalidParams(InRequest.GetRequestID(), "Create Message Request params could not be retrieved"));
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
	if (const auto Data = GetNotificationParams<ProgressNotification::Params>(InNotification))
	{
		Logger::Notice(std::string("Progress Notification: ") + "\n	Progress Token: "
			+ Data.value()->ProgressToken.ToString() + "\n	Progress: " + std::to_string(Data.value()->Progress)
			+ "\n	Message: " + Data.value()->Message.value());
	}

	Logger::Error("Invalid progress notification");
}

void MCPClient::OnNotified_CancelRequest(const CancelledNotification& InNotification)
{
	if (const auto Data = GetNotificationParams<CancelledNotification::Params>(InNotification))
	{
		if (const bool Success = m_MessageManager->UnregisterResponseHandler(Data.value()->CancelRequestID); !Success)
		{
			Logger::Error("Failed to cancel request: " + Data.value()->CancelRequestID.ToString());
			return;
		}
		Logger::Notice("Cancelled request: " + Data.value()->CancelRequestID.ToString());
	}

	Logger::Error("Invalid cancel request");
}

MCP_NAMESPACE_END