#include "CoreSDK/Core/MCPClient.h"

#include "CoreSDK/Transport/ITransport.h"

MCP_NAMESPACE_BEGIN

MCPClient::MCPClient(TransportType InTransportType,
                     std::optional<std::unique_ptr<TransportOptions>> InOptions)
    : MCPProtocol(TransportFactory::CreateTransport(InTransportType, TransportSide::Client,
                                                    std::move(InOptions))) {}

MCPTask_Void MCPClient::Initialize(const MCPClientInfo& InClientInfo,
                                   const std::optional<MCPServerInfo>& InServerInfo) {
    if (IsInitialized()) {
        HandleRuntimeError("Protocol already initialized");
        co_return;
    }

    try {
        // Start transport
        co_await m_Transport->Connect();

        // Send initialize request
        InitializeRequest initRequest;
        initRequest.ProtocolVersion = PROTOCOL_VERSION;
        initRequest.ClientInfo = InClientInfo;

        if (InServerInfo.has_value()) { initRequest.ServerInfo = InServerInfo.value(); }

        auto responseJson = co_await SendRequest(RequestBase("initialize", initRequest));
        auto response = responseJson.get<InitializeResponse>();

        // Store negotiated capabilities
        m_ClientCapabilities = response.Capabilities;
        m_ServerInfo = response.ServerInfo;

        // Send initialized notification
        co_await SendNotification(NotificationBase("initialized"));

        m_IsInitialized = true;

        if (m_InitializedHandler) { m_InitializedHandler(response); }

    } catch (const std::exception& e) {
        HandleRuntimeError("Failed to initialize protocol: " + std::string(e.what()));
    }

    co_return;
}

MCPTask_Void MCPClient::Connect(const MCPClientInfo& InClientInfo) {
    if (m_Transport->IsConnected()) {
        HandleRuntimeError("Client already connected");
        co_return;
    }

    try {
        co_await Initialize(InClientInfo, std::nullopt);
        m_Transport->IsConnected() = true;
        m_ClientInfo = InClientInfo;
    } catch (const std::exception& e) {
        HandleRuntimeError("Failed to connect: " + std::string(e.what()));
        co_return;
    }

    co_return;
}

MCPTask_Void MCPClient::Disconnect() {
    if (!m_Transport->IsConnected()) { co_return; }

    try {
        co_await Stop();
        m_Transport->IsConnected() = false;
    } catch (const std::exception& e) {
        HandleRuntimeError("Failed to disconnect: " + std::string(e.what()));
        co_return;
    }

    co_return;
}

bool MCPClient::IsConnected() const {
    return m_Transport->IsConnected();
}

MCPTask<ToolListResponse> MCPClient::ListTools(const std::optional<std::string>& InCursor) {
    ToolListRequest request;
    if (InCursor.has_value()) { request.Cursor = InCursor.value(); }

    auto response = co_await SendRequest("tools/list", JSONValue(request));
    co_return response.get<ToolListResponse>();
}

MCPTask<ToolCallResponse> MCPClient::CallTool(const std::string& InToolName,
                                              const JSONValue& InArguments) {
    if (!m_Transport->IsConnected()) {
        HandleRuntimeError("Client not connected");
        co_return;
    }

    ToolCallRequest request;
    request.Name = InToolName;
    request.Arguments = InArguments;

    auto response = co_await SendRequest("tools/call", JSONValue(request));
    co_return response.get<ToolCallResponse>();
}

MCPTask<PromptListResponse> MCPClient::ListPrompts(const std::optional<std::string>& InCursor) {
    if (!m_Transport->IsConnected()) {
        HandleRuntimeError("Client not connected");
        co_return;
    }

    PromptListRequest request;
    if (InCursor.has_value()) { request.Cursor = InCursor.value(); }

    auto response = co_await SendRequest("prompts/list", JSONValue(request));
    co_return response.get<PromptListResponse>();
}

MCPTask<PromptGetResponse> MCPClient::GetPrompt(const std::string& InPromptName,
                                                const std::optional<JSONValue>& InArguments) {
    if (!m_Transport->IsConnected()) {
        HandleRuntimeError("Client not connected");
        co_return;
    }

    PromptGetRequest request;
    request.Name = InPromptName;
    if (InArguments.has_value()) { request.Arguments = InArguments.value(); }

    auto response = co_await SendRequest("prompts/get", JSONValue(request));
    co_return response.get<PromptGetResponse>();
}

MCPTask<ResourceListResponse> MCPClient::ListResources(const std::optional<std::string>& InCursor) {
    if (!m_Transport->IsConnected()) {
        HandleRuntimeError("Client not connected");
        co_return;
    }

    ResourceListRequest request;
    if (InCursor.has_value()) { request.Cursor = InCursor.value(); }

    auto response = co_await SendRequest("resources/list", JSONValue(request));
    co_return response.get<ResourceListResponse>();
}

MCPTask<ResourceReadResponse> MCPClient::ReadResource(const std::string& InResourceURI) {
    if (!m_Transport->IsConnected()) {
        HandleRuntimeError("Client not connected");
        co_return;
    }

    ResourceReadRequest request;
    request.URI = InResourceURI;

    auto response = co_await SendRequest("resources/read", JSONValue(request));
    co_return response.get<ResourceReadResponse>();
}

MCPTask_Void MCPClient::SubscribeToResource(const std::string& InResourceURI) {
    if (!m_Transport->IsConnected()) {
        HandleRuntimeError("Client not connected");
        co_return;
    }

    ResourceSubscribeRequest request;
    request.URI = InResourceURI;

    co_await SendRequest("resources/subscribe", JSONValue(request));
}

MCPTask_Void MCPClient::UnsubscribeFromResource(const std::string& InResourceURI) {
    if (!m_Transport->IsConnected()) {
        HandleRuntimeError("Client not connected");
        co_return;
    }

    ResourceUnsubscribeRequest request;
    request.URI = InResourceURI;

    co_await SendRequest("resources/unsubscribe", JSONValue(request));
}

MCPTask<SamplingCreateMessageResponse>
MCPClient::CreateMessage(const SamplingCreateMessageRequest& InRequest) {
    if (!m_Transport->IsConnected()) {
        HandleRuntimeError("Client not connected");
        co_return;
    }

    auto response = co_await SendRequest("sampling/createMessage", JSONValue(InRequest));
    co_return response.get<SamplingCreateMessageResponse>();
}

MCPTask<CompletionCompleteResponse>
MCPClient::CompleteText(const CompletionCompleteRequest& InRequest) {
    if (!m_Transport->IsConnected()) {
        HandleRuntimeError("Client not connected");
        co_return;
    }

    auto response = co_await SendRequest("completion/complete", JSONValue(InRequest));
    co_return response.get<CompletionCompleteResponse>();
}

void MCPClient::SetResourceUpdatedHandler(ResourceUpdatedHandler InHandler) {
    m_ResourceUpdatedHandler = InHandler;

    // Set up protocol notification handler
    m_Protocol->SetNotificationHandler(
        "notifications/resources/updated", [this](const JSONValue& InParams) {
            if (m_ResourceUpdatedHandler) {
                auto notification = InParams.get<ResourceUpdatedNotification>();
                m_ResourceUpdatedHandler(notification);
            }
        });
}

void MCPClient::SetResourceListChangedHandler(ResourceListChangedHandler InHandler) {
    m_ResourceListChangedHandler = InHandler;

    m_Protocol->SetNotificationHandler(
        "notifications/resources/list_changed", [this](const JSONValue& InParams) {
            if (m_ResourceListChangedHandler) {
                auto notification = InParams.get<ResourceListChangedNotification>();
                m_ResourceListChangedHandler(notification);
            }
        });
}

void MCPClient::SetToolListChangedHandler(ToolListChangedHandler InHandler) {
    m_ToolListChangedHandler = InHandler;

    m_Protocol->SetNotificationHandler(
        "notifications/tools/list_changed", [this](const JSONValue& InParams) {
            if (m_ToolListChangedHandler) {
                auto notification = InParams.get<ToolListChangedNotification>();
                m_ToolListChangedHandler(notification);
            }
        });
}

void MCPClient::SetPromptListChangedHandler(PromptListChangedHandler InHandler) {
    m_PromptListChangedHandler = InHandler;

    m_Protocol->SetNotificationHandler(
        "notifications/prompts/list_changed", [this](const JSONValue& InParams) {
            if (m_PromptListChangedHandler) {
                auto notification = InParams.get<PromptListChangedNotification>();
                m_PromptListChangedHandler(notification);
            }
        });
}

void MCPClient::SetProgressHandler(ProgressHandler InHandler) {
    m_ProgressHandler = InHandler;

    m_Protocol->SetNotificationHandler("notifications/progress", [this](const JSONValue& InParams) {
        if (m_ProgressHandler) {
            auto notification = InParams.get<ProgressNotification>();
            m_ProgressHandler(notification);
        }
    });
}

void MCPClient::SetLogHandler(LogHandler InHandler) {
    m_LogHandler = InHandler;

    m_Protocol->SetNotificationHandler("notifications/message", [this](const JSONValue& InParams) {
        if (m_LogHandler) {
            auto notification = InParams.get<LoggingMessageNotification>();
            m_LogHandler(notification);
        }
    });
}

void MCPClient::CreateTransport() {
    switch (m_TransportType) {
        case TransportType::Stdio: {
            auto stdioOptions =
                dynamic_cast<StdioClientTransportOptions*>(m_TransportOptions.get());
            if (!stdioOptions) {
                throw std::invalid_argument("Invalid options for stdio transport");
            }
            m_Transport = std::make_unique<StdioClientTransport>(*stdioOptions);
            break;
        }
        case TransportType::StreamableHTTP: {
            auto httpOptions = dynamic_cast<HTTPTransportOptions*>(m_TransportOptions.get());
            if (!httpOptions) { throw std::invalid_argument("Invalid options for HTTP transport"); }
            m_Transport = std::make_unique<HTTPTransportClient>(*httpOptions);
            break;
        }
        default: throw std::invalid_argument("Unsupported transport type");
    }
}

MCP_NAMESPACE_END