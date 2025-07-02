#include "CoreSDK/Core/MCPClient.h"

#include "CoreSDK/Transport/ITransport.h"

MCP_NAMESPACE_BEGIN

MCPClient::MCPClient(TransportType InTransportType,
                     std::optional<std::unique_ptr<TransportOptions>> InOptions,
                     const Implementation& InClientInfo, const ClientCapabilities& InCapabilities)
    : MCPProtocol(TransportFactory::CreateTransport(InTransportType, TransportSide::Client,
                                                    std::move(InOptions))),
      m_ClientInfo(InClientInfo), m_ClientCapabilities(InCapabilities) {}

MCPTask_Void MCPClient::Start() {
    if (IsConnected()) {
        HandleRuntimeError("Client already connected");
        co_return;
    }

    try {
        InitializeResponse::InitializeResult Result = co_await Request_Initialize();
        IsConnected() = true;
        m_ClientInfo = InClientInfo;
    } catch (const std::exception& e) {
        HandleRuntimeError("Failed to connect: " + std::string(e.what()));
        co_return;
    }

    co_return;
}

MCPTask_Void MCPClient::Stop() {
    if (!IsConnected()) { co_return; }

    try {
        co_await Stop();
        IsConnected() = false;
    } catch (const std::exception& e) {
        HandleRuntimeError("Failed to disconnect: " + std::string(e.what()));
        co_return;
    }

    co_return;
}

MCPTask<InitializeResponse::Result> MCPClient::Request_Initialize() {
    if (IsInitialized()) {
        HandleRuntimeError("Protocol already initialized");
        co_return;
    }

    try {
        // Start transport
        co_await m_Transport->Connect();

        // Send initialize request t
        InitializeRequest Request{InitializeRequest::Params{
            .ProtocolVersion = m_ClientInfo.Version,
            .Capabilities = m_ClientCapabilities,
            .ClientInfo = m_ClientInfo,
        }};

        InitializeResponse Response = co_await SendRequest<InitializeResponse>(Request);

        // Store negotiated capabilities
        m_ClientCapabilities = Response.Result.Capabilities;
        m_ServerInfo = Response.Result.ServerInfo;

        // Send initialized notification
        co_await SendNotification(NotificationBase("initialized"));

        m_IsInitialized = true;

        if (m_InitializedHandler) { m_InitializedHandler(response); }

    } catch (const std::exception& e) {
        HandleRuntimeError("Failed to initialize protocol: " + std::string(e.what()));
    }

    co_return;
}

void MCPClient::OnNotified_Initialized() {}

MCPTask<ListToolsResponse>
MCPClient::Request_ListTools(const std::optional<std::string>& InCursor) {
    ToolListRequest request;
    if (InCursor.has_value()) { request.Cursor = InCursor.value(); }

    auto response = co_await SendRequest("tools/list", JSONData(request));
    co_return response.get<ToolListResponse>();
}

MCPTask<CallToolResponse> MCPClient::Request_CallTool(const std::string& InToolName,
                                                      const JSONData& InArguments) {
    if (!IsConnected()) {
        HandleRuntimeError("Client not connected");
        co_return;
    }

    ToolCallRequest request;
    request.Name = InToolName;
    request.Arguments = InArguments;

    auto response = co_await SendRequest("tools/call", JSONData(request));
    co_return response.get<ToolCallResponse>();
}

void MCPClient::OnNotified_ToolListChanged() {}

MCPTask<ListPromptsResponse>
MCPClient::Request_ListPrompts(const std::optional<std::string>& InCursor) {
    if (!IsConnected()) {
        HandleRuntimeError("Client not connected");
        co_return;
    }

    PromptListRequest request;
    if (InCursor.has_value()) { request.Cursor = InCursor.value(); }

    auto response = co_await SendRequest("prompts/list", JSONData(request));
    co_return response.get<PromptListResponse>();
}

MCPTask<GetPromptResponse>
MCPClient::Request_GetPrompt(const std::string& InPromptName,
                             const std::optional<JSONData>& InArguments) {
    if (!IsConnected()) {
        HandleRuntimeError("Client not connected");
        co_return;
    }

    PromptGetRequest request;
    request.Name = InPromptName;
    if (InArguments.has_value()) { request.Arguments = InArguments.value(); }

    auto response = co_await SendRequest("prompts/get", JSONData(request));
    co_return response.get<PromptGetResponse>();
}

void MCPClient::OnNotified_PromptListChanged() {}

MCPTask<ListResourcesResponse>
MCPClient::Request_ListResources(const std::optional<std::string>& InCursor) {
    if (!IsConnected()) {
        HandleRuntimeError("Client not connected");
        co_return;
    }

    ResourceListRequest request;
    if (InCursor.has_value()) { request.Cursor = InCursor.value(); }

    auto response = co_await SendRequest("resources/list", JSONData(request));
    co_return response.get<ResourceListResponse>();
}

MCPTask<ReadResourceResponse> MCPClient::Request_ReadResource(const std::string& InResourceURI) {
    if (!IsConnected()) {
        HandleRuntimeError("Client not connected");
        co_return;
    }

    ResourceReadRequest request;
    request.URI = InResourceURI;

    auto response = co_await SendRequest("resources/read", JSONData(request));
    co_return response.get<ResourceReadResponse>();
}

MCPTask_Void MCPClient::Request_Subscribe(const std::string& InResourceURI) {
    if (!IsConnected()) {
        HandleRuntimeError("Client not connected");
        co_return;
    }

    ResourceSubscribeRequest request;
    request.URI = InResourceURI;

    co_await SendRequest("resources/subscribe", JSONData(request));
}

MCPTask_Void MCPClient::Request_Unsubscribe(const std::string& InResourceURI) {
    if (!IsConnected()) {
        HandleRuntimeError("Client not connected");
        co_return;
    }

    ResourceUnsubscribeRequest request;
    request.URI = InResourceURI;

    co_await SendRequest("resources/unsubscribe", JSONData(request));
}

void MCPClient::OnNotified_ResourceListChanged() {}

void MCPClient::OnNotified_ResourceUpdated() {}

MCPTask<ListRootsResponse> MCPClient::Request_ListRoots() {
    if (!IsConnected()) {
        HandleRuntimeError("Client not connected");
        co_return;
    }

    RootListRequest request;

    auto response = co_await SendRequest("roots/list", JSONData(request));
    co_return response.get<RootListResponse>();
}

void MCPClient::OnNotified_RootsListChanged() {}

MCPTask_Void MCPClient::Request_SetLoggingLevel(LoggingLevel InLevel) {
    if (!IsConnected()) {
        HandleRuntimeError("Client not connected");
        co_return;
    }

    LoggingLevelRequest request;
    request.Level = InLevel;

    co_await SendRequest("logging/setLevel", JSONData(request));
}

void MCPClient::OnNotified_LogMessage() {}

void MCPClient::OnRequest_CreateMessage(const CreateMessageRequest& InRequest) {
    if (!IsConnected()) {
        HandleRuntimeError("Client not connected");
        co_return;
    }

    auto response = co_await SendRequest("sampling/createMessage", JSONData(InRequest));
    co_return response.get<SamplingCreateMessageResponse>();
}

MCPTask<CompleteResponse::Result> MCPClient::Request_Complete(const CompleteRequest& InRequest) {
    if (!IsConnected()) {
        HandleRuntimeError("Client not connected");
        co_return;
    }

    auto response = co_await SendRequest(InRequest);
    co_return response.get<CompleteResponse>();
}

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