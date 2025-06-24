#pragma once

#include <Poco/Pipe.h>
#include <Poco/PipeStream.h>
#include <Poco/Process.h>

#include <future>
#include <mutex>
#include <thread>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Transport/ITransport.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

class StdioClientTransport : public ITransport {
  public:
    explicit StdioClientTransport(const StdioClientTransportOptions& InOptions);
    ~StdioClientTransport() noexcept override;

    // ITransport interface
    MCPTask_Void Start() override;
    MCPTask_Void Stop() override;

    MCPTask_Void TransmitMessage(const JSONValue& InMessage) override;

    std::string GetConnectionInfo() const override;

  protected:
    void ReaderThread(std::stop_token InStopToken);

  private:
    void ProcessIncomingData();
    void ProcessLine(const std::string& InLine);
    void Cleanup();

    StdioClientTransportOptions m_Options;
    std::unique_ptr<Poco::ProcessHandle> m_ProcessHandle;
    std::unique_ptr<Poco::Pipe> m_StdinPipe;
    std::unique_ptr<Poco::Pipe> m_StdoutPipe;
    std::unique_ptr<Poco::Pipe> m_StderrPipe;
    std::unique_ptr<Poco::PipeOutputStream> m_StdinStream;
    std::unique_ptr<Poco::PipeInputStream> m_StdoutStream;
    std::unique_ptr<Poco::PipeInputStream> m_StderrStream;

    bool m_ShouldStop{false};
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

    MCPTask_Void TransmitMessage(const JSONValue& InMessage) override;

    std::string GetConnectionInfo() const override;

  protected:
    void ReaderThread(std::stop_token InStopToken);

  private:
    void ProcessIncomingData();
    void ProcessLine(const std::string& InLine);

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