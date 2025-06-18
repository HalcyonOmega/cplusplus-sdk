#pragma once

#include <Poco/Mutex.h>
#include <Poco/Pipe.h>
#include <Poco/PipeStream.h>
#include <Poco/Process.h>
#include <Poco/Runnable.h>
#include <Poco/Thread.h>

#include <queue>
#include <sstream>
#include <thread>

#include "ITransport.h"

class StdioTransport : public ITransport, public Poco::Runnable {
  public:
    explicit StdioTransport(const StdioTransportOptions& InOptions);
    ~StdioTransport() override;

    // ITransport interface
    MCPTaskVoid Start() override;
    MCPTaskVoid Stop() override;
    bool IsConnected() const override;
    TransportState GetState() const override;

    MCPTask<std::string> SendRequest(const std::string& InMethod,
                                     const nlohmann::json& InParams) override;
    MCPTaskVoid SendResponse(const std::string& InRequestID,
                             const nlohmann::json& InResult) override;
    MCPTaskVoid SendErrorResponse(const std::string& InRequestID, int64_t InErrorCode,
                                  const std::string& InErrorMessage,
                                  const nlohmann::json& InErrorData = {}) override;
    MCPTaskVoid SendNotification(const std::string& InMethod,
                                 const nlohmann::json& InParams = {}) override;

    void SetMessageHandler(MessageHandler InHandler) override;
    void SetRequestHandler(RequestHandler InHandler) override;
    void SetResponseHandler(ResponseHandler InHandler) override;
    void SetNotificationHandler(NotificationHandler InHandler) override;
    void SetErrorHandler(ErrorHandler InHandler) override;
    void SetStateChangeHandler(StateChangeHandler InHandler) override;

    std::string GetConnectionInfo() const override;

  protected:
    // Poco::Runnable interface
    void run() override;

  private:
    void ProcessIncomingData();
    void ProcessLine(const std::string& InLine);
    MCPTaskVoid WriteMessage(const nlohmann::json& InMessage);
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

    Poco::Thread m_ReadThread;
    std::atomic<bool> m_ShouldStop{false};
    std::string m_Buffer;

    // Response tracking
    struct PendingRequest {
        std::string RequestID;
        std::promise<std::string> Promise;
        std::chrono::steady_clock::time_point StartTime;
    };

    std::unordered_map<std::string, std::unique_ptr<PendingRequest>> m_PendingRequests;
    mutable Poco::Mutex m_RequestsMutex;
    mutable Poco::Mutex m_WriteMutex;
};

class StdioServerTransport : public ITransport, public Poco::Runnable {
  public:
    StdioServerTransport();
    ~StdioServerTransport() override;

    // ITransport interface
    MCPTaskVoid Start() override;
    MCPTaskVoid Stop() override;
    bool IsConnected() const override;
    TransportState GetState() const override;

    MCPTask<std::string> SendRequest(const std::string& InMethod,
                                     const nlohmann::json& InParams) override;
    MCPTaskVoid SendResponse(const std::string& InRequestID,
                             const nlohmann::json& InResult) override;
    MCPTaskVoid SendErrorResponse(const std::string& InRequestID, int64_t InErrorCode,
                                  const std::string& InErrorMessage,
                                  const nlohmann::json& InErrorData = {}) override;
    MCPTaskVoid SendNotification(const std::string& InMethod,
                                 const nlohmann::json& InParams = {}) override;

    void SetMessageHandler(MessageHandler InHandler) override;
    void SetRequestHandler(RequestHandler InHandler) override;
    void SetResponseHandler(ResponseHandler InHandler) override;
    void SetNotificationHandler(NotificationHandler InHandler) override;
    void SetErrorHandler(ErrorHandler InHandler) override;
    void SetStateChangeHandler(StateChangeHandler InHandler) override;

    std::string GetConnectionInfo() const override;

  protected:
    // Poco::Runnable interface
    void run() override;

  private:
    void ProcessIncomingData();
    void ProcessLine(const std::string& InLine);
    MCPTaskVoid WriteMessage(const nlohmann::json& InMessage);
    void HandleError(const std::string& InError);

    Poco::Thread m_ReadThread;
    std::atomic<bool> m_ShouldStop{false};
    std::string m_Buffer;

    // Response tracking
    struct PendingRequest {
        std::string RequestID;
        std::promise<std::string> Promise;
        std::chrono::steady_clock::time_point StartTime;
    };

    std::unordered_map<std::string, std::unique_ptr<PendingRequest>> m_PendingRequests;
    mutable Poco::Mutex m_RequestsMutex;
    mutable Poco::Mutex m_WriteMutex;
};