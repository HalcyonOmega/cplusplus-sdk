#pragma once

#include <future>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "CoreSDK/Common/Content.h"
#include "CoreSDK/Common/Logging.h"
#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Common/Progress.h"
#include "CoreSDK/Features/SamplingBase.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "URIProxy.h"

// Forward declarations
class MCPServer;
class ResourceManager;
class PromptManager;
class ToolManager;

MCP_NAMESPACE_BEGIN

/**
 * MCPContext provides access to MCP session capabilities within tools, resources, and prompts.
 *
 * The context object allows functions to:
 * - Log messages back to the MCP client
 * - Report progress on long-running operations
 * - Read resources from the server
 * - Request LLM sampling from the client
 * - Access request metadata
 * - Interact with the underlying server
 */
class MCPContext {
  public:
    MCPContext(const RequestID& InRequestID,
               const std::optional<std::string>& InClientID = std::nullopt,
               const std::optional<std::string>& InSessionID = std::nullopt,
               const std::optional<ProgressToken>& InProgressToken = std::nullopt,
               std::shared_ptr<MCPServer> InServer = nullptr);

    virtual ~MCPContext() = default;

    // Request Information
    [[nodiscard]] const RequestID& GetRequestID() const {
        return m_RequestID;
    }
    [[nodiscard]] const std::optional<std::string>& GetClientID() const {
        return m_ClientID;
    }
    [[nodiscard]] const std::optional<std::string>& GetSessionID() const {
        return m_SessionID;
    }

    // Logging capabilities
    std::future<void> Debug(const std::string& InMessage);
    std::future<void> Info(const std::string& InMessage);
    std::future<void> Warning(const std::string& InMessage);
    std::future<void> Error(const std::string& InMessage);
    std::future<void> Log(LoggingLevel InLevel, const std::string& InMessage,
                          const std::optional<std::string>& InLoggerName = std::nullopt);

    // Progress reporting
    std::future<void> ReportProgress(float InProgress,
                                     const std::optional<float>& InTotal = std::nullopt,
                                     const std::optional<std::string>& InMessage = std::nullopt);

    // Resource access
    std::future<std::vector<std::variant<TextResourceContents, BlobResourceContents>>>
    ReadResource(const std::string& InURI);
    std::future<std::vector<std::variant<TextResourceContents, BlobResourceContents>>>
    ReadResource(const MCP::URI& InURI);

    // LLM Sampling
    std::future<SamplingResult>
    Sample(const std::string& InMessage,
           const std::optional<std::string>& InSystemPrompt = std::nullopt,
           const std::optional<float>& InTemperature = std::nullopt,
           const std::optional<int>& InMaxTokens = std::nullopt,
           const std::optional<ModelPreferences>& InModelPreferences = std::nullopt);

    std::future<SamplingResult>
    Sample(const std::vector<SamplingMessage>& InMessages,
           const std::optional<std::string>& InSystemPrompt = std::nullopt,
           const std::optional<float>& InTemperature = std::nullopt,
           const std::optional<int>& InMaxTokens = std::nullopt,
           const std::optional<ModelPreferences>& InModelPreferences = std::nullopt);

    // Component notifications (rarely used directly)
    std::future<void> SendToolListChanged();
    std::future<void> SendResourceListChanged();
    std::future<void> SendPromptListChanged();

    // Advanced access
    [[nodiscard]] std::shared_ptr<MCPServer> GetServer() const {
        return m_Server;
    }
    [[nodiscard]] bool HasProgressToken() const {
        return m_ProgressToken.has_value();
    }

    // Synchronous versions of async methods
    void DebugSync(const std::string& InMessage);
    void InfoSync(const std::string& InMessage);
    void WarningSync(const std::string& InMessage);
    void ErrorSync(const std::string& InMessage);
    void LogSync(LoggingLevel InLevel, const std::string& InMessage,
                 const std::optional<std::string>& InLoggerName = std::nullopt);

    void ReportProgressSync(float InProgress, const std::optional<float>& InTotal = std::nullopt,
                            const std::optional<std::string>& InMessage = std::nullopt);

    std::vector<std::variant<TextResourceContents, BlobResourceContents>>
    ReadResourceSync(const std::string& InURI);
    std::vector<std::variant<TextResourceContents, BlobResourceContents>>
    ReadResourceSync(const MCP::URI& InURI);

    SamplingResult
    SampleSync(const std::string& InMessage,
               const std::optional<std::string>& InSystemPrompt = std::nullopt,
               const std::optional<float>& InTemperature = std::nullopt,
               const std::optional<int>& InMaxTokens = std::nullopt,
               const std::optional<ModelPreferences>& InModelPreferences = std::nullopt);

    SamplingResult
    SampleSync(const std::vector<SamplingMessage>& InMessages,
               const std::optional<std::string>& InSystemPrompt = std::nullopt,
               const std::optional<float>& InTemperature = std::nullopt,
               const std::optional<int>& InMaxTokens = std::nullopt,
               const std::optional<ModelPreferences>& InModelPreferences = std::nullopt);

  private:
    RequestID m_RequestID;
    std::optional<std::string> m_ClientID;
    std::optional<std::string> m_SessionID;
    std::optional<ProgressToken> m_ProgressToken;
    std::shared_ptr<MCPServer> m_Server;

    // Helper methods for async operations
    template <typename T> std::future<T> CreateCompletedFuture(T&& InValue);

    std::future<void> CreateCompletedVoidFuture();
};

MCP_NAMESPACE_END