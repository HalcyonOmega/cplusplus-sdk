#pragma once

#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Messages/MCPMessages.h"
#include "CoreSDK/Transport/ITransport.h"
#include "Utilities/Async/MCPTask.h"

MCP_NAMESPACE_BEGIN
static constexpr double DEFAULT_TEMPERATURE{0.7};

// Protocol state
enum class MCPProtocolState { Uninitialized, Initializing, Initialized, Error, Shutdown };

// Base protocol handler
class MCPProtocol {
  public:
    explicit MCPProtocol(std::unique_ptr<ITransport> InTransport);
    virtual ~MCPProtocol();

    // Lifecycle
    MCPTask_Void Start();
    MCPTask_Void Shutdown();
    bool IsInitialized() const {
        return m_State == MCPProtocolState::Initialized;
    }
    MCPProtocolState GetState() const {
        return m_State;
    }

    // Core protocol operations
    MCPTask<InitializeResponse::InitializeResult>
    Initialize(const std::string& InProtocolVersion, const ClientCapabilities& InCapabilities,
               const Implementation& InClientInfo);
    MCPTask_Void SendInitialized();
    MCPTask<JSONValue> Ping();

    // Protocol version validation
    static const std::vector<std::string> SUPPORTED_PROTOCOL_VERSIONS;
    void ValidateProtocolVersion(const std::string& InVersion) const;

    // Message sending utilities
    template <typename TRequest>
    MCPTask<typename TRequest::ResponseType::ResultType> SendRequest(const TRequest& InRequest);

    template <typename TResponse>
    MCPTask_Void SendResponse(const std::string& InRequestID, const TResponse& InResponse);

    template <typename TNotification>
    MCPTask_Void SendNotification(const TNotification& InNotification);

    // Error handling
    MCPTask_Void SendError(const std::string& InRequestID, int64_t InCode,
                           const std::string& InMessage, const JSONValue& InData = {});

    // Event handlers
    using RequestHandlerFunc = std::function<MCPTask_Void(
        const std::string& InMethod, const JSONValue& InParams, const std::string& InRequestID)>;
    using ResponseHandlerFunc =
        std::function<void(const JSONValue& InResult, const std::string& InRequestID)>;
    using NotificationHandlerFunc =
        std::function<void(const std::string& InMethod, const JSONValue& InParams)>;
    using ErrorHandlerFunc = std::function<void(const std::string& InError)>;

    void SetRequestHandler(RequestHandlerFunc InHandler) {
        m_RequestHandler = InHandler;
    }
    void SetResponseHandler(ResponseHandlerFunc InHandler) {
        m_ResponseHandler = InHandler;
    }
    void SetNotificationHandler(NotificationHandlerFunc InHandler) {
        m_NotificationHandler = InHandler;
    }
    void SetErrorHandler(ErrorHandlerFunc InHandler) {
        m_ErrorHandler = InHandler;
    }

    // Transport access
    ITransport* GetTransport() const {
        return m_Transport.get();
    }

  protected:
    virtual void OnInitializeRequest(const InitializeRequest& InRequest,
                                     const std::string& InRequestID) = 0;
    virtual void OnInitializedNotification() = 0;
    virtual MCPTask_Void HandleRequest(const std::string& InMethod, const JSONValue& InParams,
                                       const std::string& InRequestID);
    virtual void HandleResponse(const JSONValue& InResult, const std::string& InRequestID);
    virtual void HandleNotification(const std::string& InMethod, const JSONValue& InParams);
    virtual void HandleError(const std::string& InError);

    void SetState(MCPProtocolState InNewState);

    MCPProtocolState m_State;
    std::unique_ptr<ITransport> m_Transport;

  private:
    void SetupTransportHandlers();
    void OnTransportMessage(const std::string& InRawMessage);
    void OnTransportRequest(const std::string& InMethod, const nlohmann::json& InParams,
                            const std::string& InRequestID);
    void OnTransportResponse(const nlohmann::json& InResult, const std::string& InRequestID);
    void OnTransportNotification(const std::string& InMethod, const nlohmann::json& InParams);
    void OnTransportError(const std::string& InError);
    void OnTransportStateChange(TransportState InOldState, TransportState InNewState);

    // Event handlers
    RequestHandlerFunc m_RequestHandler;
    ResponseHandlerFunc m_ResponseHandler;
    NotificationHandlerFunc m_NotificationHandler;
    ErrorHandlerFunc m_ErrorHandler;

    // Request tracking
    struct PendingResponse {
        std::string RequestID;
        std::promise<JSONValue> Promise;
        std::chrono::steady_clock::time_point StartTime;
    };

    std::unordered_map<std::string, std::unique_ptr<PendingResponse>> m_PendingResponses;
    mutable std::mutex m_ResponsesMutex;
};

// Client protocol handler
class MCPClient : public MCPProtocol {
  public:
    explicit MCPClient(std::unique_ptr<ITransport> InTransport);

    // Client-specific operations
    MCPTask<ListToolsResponse::ListToolsResult> ListTools();
    MCPTask<CallToolResponse::CallToolResult>
    CallTool(const std::string& InName,
             const std::unordered_map<std::string, JSONValue>& InArguments = {});

    MCPTask<ListPromptsResponse::ListPromptsResult> ListPrompts();
    MCPTask<GetPromptResponse::GetPromptResult>
    GetPrompt(const std::string& InName,
              const std::unordered_map<std::string, std::string>& InArguments = {});

    MCPTask<ListResourcesResponse::ListResourcesResult> ListResources();
    MCPTask<ReadResourceResponse::ReadResourceResult> ReadResource(const std::string& InURI);
    MCPTask_Void Subscribe(const std::string& InURI);
    MCPTask_Void Unsubscribe(const std::string& InURI);

    MCPTask<ListRootsResponse::ListRootsResult> ListRoots();
    MCPTask_Void SetLoggingLevel(LoggingLevel InLevel);

    MCPTask<CompleteResponse::CompleteResult> Complete(const std::string& InRefType,
                                                       const std::string& InRefURI,
                                                       const std::string& InArgName,
                                                       const std::string& InArgValue);

    // Sampling (for servers that want to sample via client)
    MCPTask<CreateMessageResponse::CreateMessageResult>
    CreateMessage(const std::vector<SamplingMessage>& InMessages, int64_t InMaxTokens,
                  const std::string& InSystemPrompt = "",
                  const std::string& InIncludeContext = "none",
                  double InTemperature = DEFAULT_TEMPERATURE,
                  const std::vector<std::string>& InStopSequences = {},
                  const ModelPreferences& InModelPrefs = {}, const JSONValue& InMetadata = {});

  protected:
    void OnInitializeRequest(const InitializeRequest& InRequest,
                             const std::string& InRequestID) override;
    void OnInitializedNotification() override;

  private:
    ServerCapabilities m_ServerCapabilities;
    Implementation m_ServerInfo;
};

// Server protocol handler
class MCPServer : public MCPProtocol {
  public:
    explicit MCPServer(std::unique_ptr<ITransport> InTransport, const Implementation& InServerInfo,
                       const ServerCapabilities& InCapabilities);

    // Tool management
    void RegisterTool(const Tool& InTool, std::function<MCPTask<CallToolResponse::CallToolResult>(
                                              const std::unordered_map<std::string, JSONValue>&)>
                                              InHandler);
    void UnregisterTool(const std::string& InName);
    MCPTask_Void NotifyToolListChanged();

    // Prompt management
    void RegisterPrompt(
        const Prompt& InPrompt,
        std::function<MCPTask<GetPromptResult>(const std::unordered_map<std::string, std::string>&)>
            InHandler);
    void UnregisterPrompt(const std::string& InName);
    MCPTask_Void NotifyPromptListChanged();

    // Resource management
    void
    RegisterResource(const Resource& InResource,
                     std::function<MCPTask<ReadResourceResponse::ReadResourceResult>()> InHandler);
    void RegisterResourceTemplate(
        const ResourceTemplate& InTemplate,
        std::function<MCPTask<ReadResourceResponse::ReadResourceResult>(const std::string&)>
            InHandler);
    void UnregisterResource(const std::string& InURI);
    MCPTask_Void NotifyResourceListChanged();
    MCPTask_Void NotifyResourceUpdated(const std::string& InURI);

    // Root management
    void RegisterRoot(const Root& InRoot);
    void UnregisterRoot(const std::string& InURI);
    MCPTask_Void NotifyRootsListChanged();

    // Logging
    MCPTask_Void LogMessage(LoggingLevel InLevel, const std::string& InLogger,
                            const JSONValue& InData);

    // Sampling requests (servers can request sampling from clients)
    MCPTask<CreateMessageResponse::CreateMessageResult>
    RequestSampling(const std::vector<SamplingMessage>& InMessages, int64_t InMaxTokens,
                    const std::string& InSystemPrompt = "",
                    const std::string& InIncludeContext = "none",
                    double InTemperature = DEFAULT_TEMPERATURE,
                    const std::vector<std::string>& InStopSequences = {},
                    const ModelPreferences& InModelPrefs = {}, const JSONValue& InMetadata = {});

    // Progress reporting
    MCPTask_Void ReportProgress(const std::string& InRequestID, double InProgress,
                                int64_t InTotal = -1);
    MCPTask_Void CancelRequest(const std::string& InRequestID, const std::string& InReason = "");

  protected:
    void OnInitializeRequest(const InitializeRequest& InRequest,
                             const std::string& InRequestID) override;
    void OnInitializedNotification() override;
    MCPTask_Void HandleRequest(const std::string& InMethod, const JSONValue& InParams,
                               const std::string& InRequestID) override;

  private:
    Implementation m_ServerInfo;
    ServerCapabilities m_ServerCapabilities;
    ClientCapabilities m_ClientCapabilities;

    // Tool handlers
    struct ToolHandler {
        Tool ToolInfo;
        std::function<MCPTask<CallToolResult>(const std::unordered_map<std::string, JSONValue>&)>
            Handler;
    };
    std::unordered_map<std::string, ToolHandler> m_Tools;

    // Prompt handlers
    struct PromptHandler {
        Prompt PromptInfo;
        std::function<MCPTask<GetPromptResult>(const std::unordered_map<std::string, std::string>&)>
            Handler;
    };
    std::unordered_map<std::string, PromptHandler> m_Prompts;

    // Resource handlers
    struct ResourceHandler {
        Resource ResourceInfo;
        std::function<MCPTask<ReadResourceResult>()> Handler;
    };
    std::unordered_map<std::string, ResourceHandler> m_Resources;

    struct ResourceTemplateHandler {
        ResourceTemplate TemplateInfo;
        std::function<MCPTask<ReadResourceResult>(const std::string&)> Handler;
    };
    std::unordered_map<std::string, ResourceTemplateHandler> m_ResourceTemplates;

    // Root storage
    std::unordered_map<std::string, Root> m_Roots;

    // Subscription management
    std::unordered_map<std::string, std::unordered_set<std::string>>
        m_Subscriptions; // URI -> Set of client IDs

    mutable std::mutex m_HandlersMutex;

    // New additions for compliance fixes

    // Pagination helper methods
    std::string EncodeCursor(size_t InIndex) const;
    size_t DecodeCursor(const std::string& InCursor) const;
    static constexpr size_t DEFAULT_PAGE_SIZE = 100;

    // Progress tracking and tool execution
    MCPTask<CallToolResponse> ExecuteToolWithProgress(
        const Tool& InTool,
        const std::optional<std::unordered_map<std::string, nlohmann::json>>& InArguments,
        const std::string& InRequestID);

    // Resource subscription management
    MCPTask_Void NotifyResourceSubscribers(const std::string& InURI);
    std::string GetCurrentClientID() const;
    MCPTask_Void SendNotificationToClient(const std::string& InClientID,
                                          const ResourceUpdatedNotification& InNotification);

    // Updated subscription management with client tracking
    std::unordered_map<std::string, std::set<std::string>>
        m_ResourceSubscriptions; // URI -> Set of client IDs
    mutable std::mutex m_ResourceSubscriptionsMutex;
};

// Progress tracking class for long-running operations
class ProgressTracker {
  public:
    ProgressTracker(const std::string& InRequestID, std::shared_ptr<MCPProtocol> InProtocol);

    MCPTask_Void UpdateProgress(double InProgress, std::optional<int64_t> InTotal = {});
    MCPTask_Void CompleteProgress();

  private:
    std::string m_RequestID;
    std::shared_ptr<MCPProtocol> m_Protocol;
    std::atomic<bool> m_IsComplete{false};
};

MCP_NAMESPACE_END