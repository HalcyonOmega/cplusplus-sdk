#include "CoreSDK/Core/MCPClient.h"

#include "CoreSDK/Transport/ITransport.h"

#define RETURN_IF_CLIENT_NOT_CONNECTED              \
	if (!IsConnected())                             \
	{                                               \
		HandleRuntimeError("Client not connected"); \
		co_return;                                  \
	}

MCP_NAMESPACE_BEGIN

MCPClient::MCPClient(TransportType InTransportType, std::optional<std::unique_ptr<TransportOptions>> InOptions,
	const Implementation& InClientInfo, const ClientCapabilities& InCapabilities) :
	MCPProtocol(TransportFactory::CreateTransport(InTransportType, TransportSide::Client, std::move(InOptions))),
	m_ClientInfo(InClientInfo),
	m_ClientCapabilities(InCapabilities)
{}

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
		// Start transport
		co_await m_Transport->Connect();

		// Send initialize request
		InitializeRequest Request{ InitializeRequest::Params{
			.ProtocolVersion = m_ClientInfo.Version,
			.Capabilities = m_ClientCapabilities,
			.ClientInfo = m_ClientInfo,
		} };

		InitializeResponse Response = co_await SendRequest(Request);

		// Store negotiated capabilities
		m_ClientCapabilities = Response.Result.Capabilities;
		m_ServerInfo = Response.Result.ServerInfo;

		m_IsInitialized = true;

		if (m_InitializedHandler)
		{
			m_InitializedHandler(response);
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

	ToolListRequest request;
	if (InCursor.has_value())
	{
		request.Cursor = InCursor.value();
	}

	auto response = co_await SendRequest("tools/list", JSONData(request));
	co_return response.get<ToolListResponse>();
}

MCPTask<CallToolResponse::Result> MCPClient::Request_CallTool(const CallToolRequest::Params& InParams)
{
	RETURN_IF_CLIENT_NOT_CONNECTED

	SendRequest(CallToolRequest{ InParams });

	auto response = co_await SendRequest("tools/call", JSONData(request));
	co_return response.get<ToolCallResponse>();
}

void MCPClient::OnNotified_ToolListChanged(const ToolListChangedNotification& InNotification) {}

MCPTask<ListPromptsResponse::Result> MCPClient::Request_ListPrompts(const PaginatedRequestParams& InParams)
{
	RETURN_IF_CLIENT_NOT_CONNECTED

	SendRequest(ListPromptsRequest{ InParams });
	response = co_await SendRequest("prompts/list", JSONData(request));
	co_return response.get<PromptListResponse>();
}
MCPTask<GetPromptResponse::Result> MCPClient::Request_GetPrompt(const GetPromptRequest::Params& InParams)
{
	RETURN_IF_CLIENT_NOT_CONNECTED

	PromptGetRequest request;
	request.Name = InPromptName;
	if (InArguments.has_value())
	{
		request.Arguments = InArguments.value();
	}

	auto response = co_await SendRequest("prompts/get", JSONData(request));
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

MCPTask_Void MCPClient::Request_Complete(const CompleteRequest::Params& InParams)
{
	RETURN_IF_CLIENT_NOT_CONNECTED

	auto response = co_await SendRequest(CompleteRequest{ InRequest });
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
void MCPClient::OnNotified_Progress(const ProgressNotification& InNotification) {}
void MCPClient::OnNotified_CancelRequest(const CancelledNotification& InNotification) {}

// Old Handler Setters
// void MCPClient::SetResourceUpdatedHandler(ResourceUpdatedHandler InHandler) {
//     m_ResourceUpdatedHandler = InHandler;

//     // Set up protocol notification handler
//     SetNotificationHandler("notifications/resources/updated", [this](const JSONData& InParams) {
//         if (m_ResourceUpdatedHandler) {
//             auto notification = InParams.get<ResourceUpdatedNotification>();
//             m_ResourceUpdatedHandler(notification);
//         }
//     });
// }

// void MCPClient::SetResourceListChangedHandler(ResourceListChangedHandler InHandler) {
//     m_ResourceListChangedHandler = InHandler;

//     SetNotificationHandler(
//         "notifications/resources/list_changed", [this](const JSONData& InParams) {
//             if (m_ResourceListChangedHandler) {
//                 auto notification = InParams.get<ResourceListChangedNotification>();
//                 m_ResourceListChangedHandler(notification);
//             }
//         });
// }

// void MCPClient::SetToolListChangedHandler(ToolListChangedHandler InHandler) {
//     m_ToolListChangedHandler = InHandler;

//     SetNotificationHandler("notifications/tools/list_changed", [this](const JSONData& InParams) {
//         if (m_ToolListChangedHandler) {
//             auto notification = InParams.get<ToolListChangedNotification>();
//             m_ToolListChangedHandler(notification);
//         }
//     });
// }

// void MCPClient::SetPromptListChangedHandler(PromptListChangedHandler InHandler) {
//     m_PromptListChangedHandler = InHandler;

//     SetNotificationHandler("notifications/prompts/list_changed", [this](const JSONData& InParams)
//     {
//         if (m_PromptListChangedHandler) {
//             auto notification = InParams.get<PromptListChangedNotification>();
//             m_PromptListChangedHandler(notification);
//         }
//     });
// }

// void MCPClient::SetProgressHandler(ProgressHandler InHandler) {
//     m_ProgressHandler = InHandler;

//     SetNotificationHandler("notifications/progress", [this](const JSONData& InParams) {
//         if (m_ProgressHandler) {
//             auto notification = InParams.get<ProgressNotification>();
//             m_ProgressHandler(notification);
//         }
//     });
// }

// void MCPClient::SetLogHandler(LogHandler InHandler) {
//     m_LogHandler = InHandler;

//     SetNotificationHandler("notifications/message", [this](const JSONData& InParams) {
//         if (m_LogHandler) {
//             auto notification = InParams.get<LoggingMessageNotification>();
//             m_LogHandler(notification);
//         }
//     });
// }

MCP_NAMESPACE_END