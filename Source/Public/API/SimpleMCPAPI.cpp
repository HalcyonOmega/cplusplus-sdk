#include "SimpleMCPAPI.h"

MCP_NAMESPACE_BEGIN

// SimpleMCPClient Implementation
SimpleMCPClient::SimpleMCPClient(const std::string& InCommand,
                                 const std::vector<std::string>& InArguments) {
    auto options = std::make_unique<StdioTransportOptions>();
    options->Command = InCommand;
    options->Arguments = InArguments;

    m_Client = std::make_unique<MCPClient>(TransportType::Stdio, std::move(options));
}

SimpleMCPClient::SimpleMCPClient(const std::string& InHost, uint16_t InPort,
                                 const std::string& InPath) {
    auto options = std::make_unique<HTTPTransportOptions>();
    options->Host = InHost;
    options->Port = InPort;
    options->Path = InPath;

    m_Client = std::make_unique<MCPClient>(TransportType::StreamableHTTP, std::move(options));
}

SimpleMCPClient::~SimpleMCPClient() = default;

MCPTask_Void SimpleMCPClient::Connect(const std::string& InClientName,
                                      const std::string& InClientVersion) {
    MCPClientInfo clientInfo;
    clientInfo.Name = InClientName;
    clientInfo.Version = InClientVersion;

    co_await m_Client->Connect(clientInfo);
}

MCPTask_Void SimpleMCPClient::Disconnect() {
    co_await m_Client->Disconnect();
}

bool SimpleMCPClient::IsConnected() const {
    return m_Client->IsConnected();
}

MCPTask<std::vector<Tool>> SimpleMCPClient::ListTools() {
    auto response = co_await m_Client->ListTools();
    co_return response.Tools;
}

MCPTask<ToolCallResponse> SimpleMCPClient::CallTool(const std::string& InToolName,
                                                    const nlohmann::json& InArguments) {
    co_return co_await m_Client->CallTool(InToolName, InArguments);
}

MCPTask<std::vector<Prompt>> SimpleMCPClient::ListPrompts() {
    auto response = co_await m_Client->ListPrompts();
    co_return response.Prompts;
}

MCPTask<PromptGetResponse> SimpleMCPClient::GetPrompt(const std::string& InPromptName,
                                                      const nlohmann::json& InArguments) {
    co_return co_await m_Client->GetPrompt(InPromptName, InArguments);
}

MCPTask<std::vector<Resource>> SimpleMCPClient::ListResources() {
    auto response = co_await m_Client->ListResources();
    co_return response.Resources;
}

MCPTask<ResourceReadResponse> SimpleMCPClient::ReadResource(const std::string& InResourceURI) {
    co_return co_await m_Client->ReadResource(InResourceURI);
}

MCPTask_Void SimpleMCPClient::SubscribeToResource(const std::string& InResourceURI) {
    co_await m_Client->SubscribeToResource(InResourceURI);
}

MCPTask_Void SimpleMCPClient::UnsubscribeFromResource(const std::string& InResourceURI) {
    co_await m_Client->UnsubscribeFromResource(InResourceURI);
}

void SimpleMCPClient::OnToolListChanged(std::function<void()> InCallback) {
    m_Client->SetToolListChangedHandler(
        [InCallback](const ToolListChangedNotification&) { InCallback(); });
}

void SimpleMCPClient::OnPromptListChanged(std::function<void()> InCallback) {
    m_Client->SetPromptListChangedHandler(
        [InCallback](const PromptListChangedNotification&) { InCallback(); });
}

void SimpleMCPClient::OnResourceListChanged(std::function<void()> InCallback) {
    m_Client->SetResourceListChangedHandler(
        [InCallback](const ResourceListChangedNotification&) { InCallback(); });
}

void SimpleMCPClient::OnResourceUpdated(std::function<void(const std::string&)> InCallback) {
    m_Client->SetResourceUpdatedHandler(
        [InCallback](const ResourceUpdatedNotification& InNotification) {
            InCallback(InNotification.URI);
        });
}

void SimpleMCPClient::OnProgress(
    std::function<void(const std::string&, double, double)> InCallback) {
    m_Client->SetProgressHandler([InCallback](const ProgressNotification& InNotification) {
        InCallback(InNotification.ProgressToken, InNotification.Progress, InNotification.Total);
    });
}

void SimpleMCPClient::OnLog(std::function<void(LoggingLevel, const std::string&)> InCallback) {
    m_Client->SetLogHandler([InCallback](const LoggingMessageNotification& InNotification) {
        InCallback(InNotification.Level, InNotification.Data);
    });
}

// SimpleMCPServer Implementation
SimpleMCPServer::SimpleMCPServer() {
    m_Server = std::make_unique<MCPServer>(TransportType::Stdio, nullptr);
}

SimpleMCPServer::SimpleMCPServer(uint16_t InPort, const std::string& InPath) {
    auto options = std::make_unique<HTTPTransportOptions>();
    options->Port = InPort;
    options->Path = InPath;

    m_Server = std::make_unique<MCPServer>(TransportType::StreamableHTTP, std::move(options));
}

SimpleMCPServer::~SimpleMCPServer() = default;

MCPTask_Void SimpleMCPServer::Start(const std::string& InServerName,
                                    const std::string& InServerVersion) {
    MCPServerInfo serverInfo;
    serverInfo.Name = InServerName;
    serverInfo.Version = InServerVersion;

    co_await m_Server->Start(serverInfo);
}

MCPTask_Void SimpleMCPServer::Stop() {
    co_await m_Server->Stop();
}

bool SimpleMCPServer::IsRunning() const {
    return m_Server->IsRunning();
}

void SimpleMCPServer::AddTool(const std::string& InName, const std::string& InDescription,
                              const nlohmann::json& InInputSchema,
                              std::function<ToolCallResponse(const nlohmann::json&)> InHandler) {
    Tool tool;
    tool.Name = InName;
    tool.Description = InDescription;
    tool.InputSchema = InInputSchema;

    m_Server->AddTool(InName, tool, InHandler);
}

void SimpleMCPServer::AddPrompt(
    const std::string& InName, const std::string& InDescription,
    const std::optional<nlohmann::json>& InArgumentsSchema,
    std::function<PromptGetResponse(const std::optional<nlohmann::json>&)> InHandler) {
    Prompt prompt;
    prompt.Name = InName;
    prompt.Description = InDescription;
    if (InArgumentsSchema.has_value()) { prompt.ArgumentsSchema = InArgumentsSchema.value(); }

    m_Server->AddPrompt(InName, prompt, InHandler);
}

void SimpleMCPServer::AddResource(const std::string& InURI, const std::string& InName,
                                  const std::string& InDescription, const std::string& InMimeType,
                                  std::function<ResourceReadResponse()> InHandler) {
    Resource resource;
    resource.URI = InURI;
    resource.Name = InName;
    resource.Description = InDescription;
    resource.MimeType = InMimeType;

    m_Server->AddResource(InURI, resource, InHandler);
}

MCPTask_Void SimpleMCPServer::NotifyResourceUpdated(const std::string& InURI) {
    co_await m_Server->NotifyResourceUpdated(InURI);
}

MCPTask_Void SimpleMCPServer::SendProgress(const std::string& InProgressToken, double InProgress,
                                           double InTotal) {
    co_await m_Server->SendProgress(InProgressToken, InProgress, InTotal);
}

MCPTask_Void SimpleMCPServer::SendLog(LoggingLevel InLevel, const std::string& InMessage) {
    co_await m_Server->SendLog(InLevel, InMessage);
}

void SimpleMCPServer::SetSamplingHandler(
    std::function<SamplingCreateMessageResponse(const SamplingCreateMessageRequest&)> InHandler) {
    m_Server->SetSamplingHandler(InHandler);
}

// Helper Functions Implementation
Tool CreateTool(const std::string& InName, const std::string& InDescription,
                const nlohmann::json& InInputSchema) {
    Tool tool;
    tool.Name = InName;
    tool.Description = InDescription;
    tool.InputSchema = InInputSchema;
    return tool;
}

Prompt CreatePrompt(const std::string& InName, const std::string& InDescription,
                    const std::optional<nlohmann::json>& InArgumentsSchema) {
    Prompt prompt;
    prompt.Name = InName;
    prompt.Description = InDescription;
    if (InArgumentsSchema.has_value()) { prompt.ArgumentsSchema = InArgumentsSchema.value(); }
    return prompt;
}

Resource CreateResource(const std::string& InURI, const std::string& InName,
                        const std::string& InDescription, const std::string& InMimeType) {
    Resource resource;
    resource.URI = InURI;
    resource.Name = InName;
    resource.Description = InDescription;
    resource.MimeType = InMimeType;
    return resource;
}

nlohmann::json CreateToolSchema(const std::string& InType,
                                const std::map<std::string, nlohmann::json>& InProperties,
                                const std::vector<std::string>& InRequired) {
    nlohmann::json schema = {{"type", InType}};

    if (!InProperties.empty()) {
        schema["properties"] = nlohmann::json::object();
        for (const auto& [name, property] : InProperties) { schema["properties"][name] = property; }
    }

    if (!InRequired.empty()) { schema["required"] = InRequired; }

    return schema;
}

nlohmann::json CreateStringProperty(const std::string& InDescription,
                                    const std::optional<std::string>& InPattern) {
    nlohmann::json property = {{"type", "string"}, {"description", InDescription}};

    if (InPattern.has_value()) { property["pattern"] = InPattern.value(); }

    return property;
}

nlohmann::json CreateNumberProperty(const std::string& InDescription,
                                    const std::optional<double>& InMinimum,
                                    const std::optional<double>& InMaximum) {
    nlohmann::json property = {{"type", "number"}, {"description", InDescription}};

    if (InMinimum.has_value()) { property["minimum"] = InMinimum.value(); }

    if (InMaximum.has_value()) { property["maximum"] = InMaximum.value(); }

    return property;
}

nlohmann::json CreateBooleanProperty(const std::string& InDescription) {
    return {{"type", "boolean"}, {"description", InDescription}};
}

nlohmann::json CreateArrayProperty(const std::string& InDescription,
                                   const nlohmann::json& InItems) {
    return {{"type", "array"}, {"description", InDescription}, {"items", InItems}};
}

nlohmann::json CreateObjectProperty(const std::string& InDescription,
                                    const std::map<std::string, nlohmann::json>& InProperties) {
    nlohmann::json property = {{"type", "object"}, {"description", InDescription}};

    if (!InProperties.empty()) {
        property["properties"] = nlohmann::json::object();
        for (const auto& [name, prop] : InProperties) { property["properties"][name] = prop; }
    }

    return property;
}

ToolCallResponse CreateTextResult(const std::string& InText, bool InIsError) {
    ToolCallResponse response;

    TextContent content;
    content.Type = "text";
    content.Text = InText;

    response.Content.push_back(content);
    response.IsError = InIsError;

    return response;
}

ToolCallResponse CreateImageResult(const std::string& InData, const std::string& InMimeType,
                                   bool InIsError) {
    ToolCallResponse response;

    ImageContent content;
    content.Type = "image";
    content.Data = InData;
    content.MimeType = InMimeType;

    response.Content.push_back(content);
    response.IsError = InIsError;

    return response;
}

ToolCallResponse CreateResourceResult(const std::string& InURI,
                                      const std::optional<std::string>& InText, bool InIsError) {
    ToolCallResponse response;

    ResourceContent content;
    content.Type = "resource";
    content.Resource.URI = InURI;
    if (InText.has_value()) { content.Text = InText.value(); }

    response.Content.push_back(content);
    response.IsError = InIsError;

    return response;
}

PromptGetResponse CreatePromptResponse(const std::string& InDescription,
                                       const std::vector<PromptMessage>& InMessages) {
    PromptGetResponse response;
    response.Description = InDescription;
    response.Messages = InMessages;
    return response;
}

PromptMessage CreateUserMessage(const std::string& InText) {
    PromptMessage message;
    message.Role = SamplingRole::User;

    TextContent content;
    content.Type = "text";
    content.Text = InText;
    message.Content.Content = content;

    return message;
}

PromptMessage CreateAssistantMessage(const std::string& InText) {
    PromptMessage message;
    message.Role = SamplingRole::Assistant;

    TextContent content;
    content.Type = "text";
    content.Text = InText;
    message.Content.Content = content;

    return message;
}

ResourceReadResponse CreateTextResourceResponse(const std::string& InText,
                                                const std::string& InMimeType) {
    ResourceReadResponse response;

    ResourceContent content;
    content.Type = "resource";
    content.Resource.URI = ""; // Will be filled by server
    content.Resource.MimeType = InMimeType;
    content.Text = InText;

    response.Contents.push_back(content);
    return response;
}

MCP_NAMESPACE_END