#pragma once

#include <Poco/Pipe.h>
#include <Poco/PipeStream.h>
#include <Poco/Process.h>

#include <future>
#include <mutex>
#include <thread>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Messages/ErrorBase.h"
#include "CoreSDK/Transport/ITransport.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

class StdioTransport : public ITransport {
  public:
    explicit StdioTransport(const StdioTransportOptions& InOptions);
    ~StdioTransport() noexcept override;

    // ITransport interface
    MCPTask_Void Start() override;
    MCPTask_Void Stop() override;
    bool IsConnected() const override;
    TransportState GetState() const override;

    MCPTask<std::string> SendRequest(const std::string& InMethod,
                                     const JSONValue& InParams) override;
    MCPTask_Void SendResponse(const std::string& InRequestID, const JSONValue& InResult) override;
    MCPTask_Void SendErrorResponse(const std::string& InRequestID, MCP::Errors InErrorCode,
                                   const std::string& InErrorMessage,
                                   const JSONValue& InErrorData = {}) override;
    MCPTask_Void SendNotification(const std::string& InMethod,
                                  const JSONValue& InParams = {}) override;

    void SetMessageHandler(MessageHandler InHandler) override;
    void SetRequestHandler(RequestHandler InHandler) override;
    void SetResponseHandler(ResponseHandler InHandler) override;
    void SetNotificationHandler(NotificationHandler InHandler) override;
    void SetErrorHandler(ErrorHandler InHandler) override;
    void SetStateChangeHandler(StateChangeHandler InHandler) override;

    std::string GetConnectionInfo() const override;

  protected:
    void ReaderThread(std::stop_token InStopToken);

  private:
    void ProcessIncomingData();
    void ProcessLine(const std::string& InLine);
    MCPTask_Void WriteMessage(const JSONValue& InMessage);
    void HandleError(const std::string& InError);
    void Cleanup();

    StdioTransportOptions m_Options;
    std::unique_ptr<Poco::ProcessHandle> m_ProcessHandle;
    std::unique_ptr<Poco::Pipe> m_StdinPipe;
    std::unique_ptr<Poco::Pipe> m_StdoutPipe;
    std::unique_ptr<Poco::Pipe> m_StderrPipe;
    std::unique_ptr<Poco::PipeOutputStream> m_StdinStream;
    std::unique_ptr<Poco::PipeInputStream> m_StdoutStream;
    std::unique_ptr<Poco::PipeInputStream> m_StderrStream;

    std::jthread m_ReadThread;
    std::string m_Buffer;

    // Response tracking
    struct PendingRequest {
        std::string RequestID;
        std::promise<std::string> Promise;
        std::chrono::steady_clock::time_point StartTime;
    };

    std::unordered_map<std::string, std::unique_ptr<PendingRequest>> m_PendingRequests;
    mutable std::mutex m_RequestsMutex;
    mutable std::mutex m_WriteMutex;
};

class StdioServerTransport : public ITransport {
  public:
    StdioServerTransport();
    ~StdioServerTransport() noexcept override;

    // ITransport interface
    MCPTask_Void Start() override;
    MCPTask_Void Stop() override;
    bool IsConnected() const override;
    TransportState GetState() const override;

    MCPTask<std::string> SendRequest(const std::string& InMethod,
                                     const JSONValue& InParams) override;
    MCPTask_Void SendResponse(const std::string& InRequestID, const JSONValue& InResult) override;
    MCPTask_Void SendErrorResponse(const std::string& InRequestID, int64_t InErrorCode,
                                   const std::string& InErrorMessage,
                                   const JSONValue& InErrorData = {}) override;
    MCPTask_Void SendNotification(const std::string& InMethod,
                                  const JSONValue& InParams = {}) override;

    void SetMessageHandler(MessageHandler InHandler) override;
    void SetRequestHandler(RequestHandler InHandler) override;
    void SetResponseHandler(ResponseHandler InHandler) override;
    void SetNotificationHandler(NotificationHandler InHandler) override;
    void SetErrorHandler(ErrorHandler InHandler) override;
    void SetStateChangeHandler(StateChangeHandler InHandler) override;

    std::string GetConnectionInfo() const override;

  protected:
    void ReaderThread(std::stop_token InStopToken);

  private:
    void ProcessIncomingData();
    void ProcessLine(const std::string& InLine);
    MCPTask_Void WriteMessage(const JSONValue& InMessage);
    void HandleError(const std::string& InError);

    std::jthread m_ReadThread;
    std::string m_Buffer;

    // Response tracking
    struct PendingRequest {
        std::string RequestID;
        std::promise<std::string> Promise;
        std::chrono::steady_clock::time_point StartTime;
    };

    std::unordered_map<std::string, std::unique_ptr<PendingRequest>> m_PendingRequests;
    mutable std::mutex m_RequestsMutex;
    mutable std::mutex m_WriteMutex;
};

MCP_NAMESPACE_END