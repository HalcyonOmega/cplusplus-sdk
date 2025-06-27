#include "CoreSDK/Common/MCPContext.h"

#include <sstream>

#include "CoreSDK/Core/MCPServer.h"
#include "CoreSDK/Features/PromptManager.h"
#include "CoreSDK/Features/ResourceManager.h"
#include "CoreSDK/Features/ToolManager.h"

MCP_NAMESPACE_BEGIN

MCPContext::MCPContext(const RequestID& InRequestID, const std::optional<std::string>& InClientID,
                       const std::optional<std::string>& InSessionID,
                       const std::optional<ProgressToken>& InProgressToken,
                       std::shared_ptr<MCPServer> InServer)
    : m_RequestID(InRequestID), m_ClientID(InClientID), m_SessionID(InSessionID),
      m_ProgressToken(InProgressToken), m_Server(InServer) {}

// Logging capabilities
std::future<void> MCPContext::Debug(const std::string& InMessage) {
    return Log(LoggingLevel::Debug, InMessage);
}

std::future<void> MCPContext::Info(const std::string& InMessage) {
    return Log(LoggingLevel::Info, InMessage);
}

std::future<void> MCPContext::Warning(const std::string& InMessage) {
    return Log(LoggingLevel::Warning, InMessage);
}

std::future<void> MCPContext::Error(const std::string& InMessage) {
    return Log(LoggingLevel::Error, InMessage);
}

std::future<void> MCPContext::Log(LoggingLevel InLevel, const std::string& InMessage,
                                  const std::optional<std::string>& InLoggerName) {
    // For now, log locally and return completed future
    // TODO: In a full implementation, this would send a logging notification to the MCP client
    MCP::Logger::Log(InMessage, InLevel);

    return CreateCompletedVoidFuture();
}

// Progress reporting
std::future<void> MCPContext::ReportProgress(float InProgress, const std::optional<float>& InTotal,
                                             const std::optional<std::string>& InMessage) {
    if (!m_ProgressToken.has_value()) {
        // No progress token available, return completed future
        return CreateCompletedVoidFuture();
    }

    // TODO: In a full implementation, this would send a progress notification to the MCP client
    std::stringstream ProgressMessage;
    ProgressMessage << "Progress: " << InProgress;
    if (InTotal.has_value()) { ProgressMessage << "/" << InTotal.value(); }
    if (InMessage.has_value()) { ProgressMessage << " - " << InMessage.value(); }

    Logger::Info(ProgressMessage.str());
    return CreateCompletedVoidFuture();
}

// Resource access
std::future<std::vector<std::variant<TextResourceContents, BlobResourceContents>>>
MCPContext::ReadResource(const std::string& InURI) {
    return ReadResource(MCP::URI(InURI));
}

std::future<std::vector<std::variant<TextResourceContents, BlobResourceContents>>>
MCPContext::ReadResource(const MCP::URI& InURI) {
    std::promise<std::vector<std::variant<TextResourceContents, BlobResourceContents>>> Promise;
    auto Future = Promise.get_future();

    try {
        if (!m_Server) { throw std::runtime_error("No server instance available in context"); }

        // TODO: Access ResourceManager from server and read the resource
        // For now, return empty result
        std::vector<std::variant<TextResourceContents, BlobResourceContents>> Result;
        Promise.set_value(std::move(Result));
    } catch (const std::exception& Ex) { Promise.set_exception(std::current_exception()); }

    return Future;
}

// LLM Sampling
std::future<SamplingResult>
MCPContext::Sample(const std::string& InMessage, const std::optional<std::string>& InSystemPrompt,
                   const std::optional<float>& InTemperature, const std::optional<int>& InMaxTokens,
                   const std::optional<ModelPreferences>& InModelPreferences) {
    // Convert string message to SamplingMessage
    SamplingMessage Message;
    Message.MessageRole = Role::User;

    TextContent TextMsg;
    TextMsg.Text = InMessage;
    Message.Content = TextMsg;

    return Sample({Message}, InSystemPrompt, InTemperature, InMaxTokens, InModelPreferences);
}

std::future<SamplingResult>
MCPContext::Sample(const std::vector<SamplingMessage>& InMessages,
                   const std::optional<std::string>& InSystemPrompt,
                   const std::optional<float>& InTemperature, const std::optional<int>& InMaxTokens,
                   const std::optional<ModelPreferences>& InModelPreferences) {
    std::promise<SamplingResult> Promise;
    auto Future = Promise.get_future();

    try {
        // TODO: In a full implementation, this would send a sampling request to the MCP client
        // For now, return a mock response
        SamplingResult Result;
        TextContent Response;
        Response.Text = "Mock LLM response";
        Result.Result = Response;
        Result.Model = "mock-model";
        Result.StopReason = "completed";

        Promise.set_value(std::move(Result));
    } catch (const std::exception& Ex) { Promise.set_exception(std::current_exception()); }

    return Future;
}

// Component notifications
std::future<void> MCPContext::SendToolListChanged() {
    // TODO: Send tool list changed notification to MCP client
    Logger::Info("Tool list changed notification (not implemented)");
    return CreateCompletedVoidFuture();
}

std::future<void> MCPContext::SendResourceListChanged() {
    // TODO: Send resource list changed notification to MCP client
    Logger::Info("Resource list changed notification (not implemented)");
    return CreateCompletedVoidFuture();
}

std::future<void> MCPContext::SendPromptListChanged() {
    // TODO: Send prompt list changed notification to MCP client
    Logger::Info("Prompt list changed notification (not implemented)");
    return CreateCompletedVoidFuture();
}

// Synchronous versions
void MCPContext::DebugSync(const std::string& InMessage) {
    Debug(InMessage).wait();
}

void MCPContext::InfoSync(const std::string& InMessage) {
    Info(InMessage).wait();
}

void MCPContext::WarningSync(const std::string& InMessage) {
    Warning(InMessage).wait();
}

void MCPContext::ErrorSync(const std::string& InMessage) {
    Error(InMessage).wait();
}

void MCPContext::LogSync(LoggingLevel InLevel, const std::string& InMessage,
                         const std::optional<std::string>& InLoggerName) {
    Log(InLevel, InMessage, InLoggerName).wait();
}

void MCPContext::ReportProgressSync(float InProgress, const std::optional<float>& InTotal,
                                    const std::optional<std::string>& InMessage) {
    ReportProgress(InProgress, InTotal, InMessage).wait();
}

std::vector<std::variant<TextResourceContents, BlobResourceContents>>
MCPContext::ReadResourceSync(const std::string& InURI) {
    return ReadResource(InURI).get();
}

std::vector<std::variant<TextResourceContents, BlobResourceContents>>
MCPContext::ReadResourceSync(const MCP::URI& InURI) {
    return ReadResource(InURI).get();
}

SamplingResult MCPContext::SampleSync(const std::string& InMessage,
                                      const std::optional<std::string>& InSystemPrompt,
                                      const std::optional<float>& InTemperature,
                                      const std::optional<int>& InMaxTokens,
                                      const std::optional<ModelPreferences>& InModelPreferences) {
    return Sample(InMessage, InSystemPrompt, InTemperature, InMaxTokens, InModelPreferences).get();
}

SamplingResult MCPContext::SampleSync(const std::vector<SamplingMessage>& InMessages,
                                      const std::optional<std::string>& InSystemPrompt,
                                      const std::optional<float>& InTemperature,
                                      const std::optional<int>& InMaxTokens,
                                      const std::optional<ModelPreferences>& InModelPreferences) {
    return Sample(InMessages, InSystemPrompt, InTemperature, InMaxTokens, InModelPreferences).get();
}

// Helper methods
template <typename T> std::future<T> MCPContext::CreateCompletedFuture(T&& InValue) {
    std::promise<T> Promise;
    auto Future = Promise.get_future();
    Promise.set_value(std::forward<T>(InValue));
    return Future;
}

std::future<void> MCPContext::CreateCompletedVoidFuture() {
    std::promise<void> Promise;
    auto Future = Promise.get_future();
    Promise.set_value();
    return Future;
}

MCP_NAMESPACE_END