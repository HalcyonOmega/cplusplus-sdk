#pragma once

#include <Poco/Event.h>
#include <Poco/Mutex.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Runnable.h>
#include <Poco/Thread.h>
#include <Poco/ThreadPool.h>

#include <chrono>
#include <future>
#include <thread>
#include <unordered_map>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Transport/ITransport.h"

// Forward declarations
class HTTPTransportClient;
class HTTPTransportServer;

MCP_NAMESPACE_BEGIN

// HTTP Client Transport
class HTTPTransportClient : public ITransport, public Poco::Runnable {
  public:
    explicit HTTPTransportClient(const HTTPTransportOptions& InOptions);
    ~HTTPTransportClient() noexcept override;

    // ITransport interface
    MCPTask_Void Start() override;
    MCPTask_Void Stop() override;
    [[nodiscard]] bool IsConnected() const override;
    [[nodiscard]] TransportState GetState() const override;

    MCPTask_Void TransmitMessage(const JSONValue& InMessage) override;

    [[nodiscard]] std::string GetConnectionInfo() const override;

  protected:
    // Poco::Runnable interface for SSE reading
    void run() override;

  private:
    MCPTask_Void ConnectToServer();
    void StartSSEConnection();
    void ProcessSSELine(const std::string& InLine);
    void HandleConnectionError(const std::string& InError);
    void Cleanup();

    HTTPTransportOptions m_Options;
    std::unique_ptr<Poco::Net::HTTPClientSession> m_HTTPSession;
    std::unique_ptr<std::istream> m_SSEStream;

    std::jthread m_SSEThread;
    std::atomic<bool> m_ShouldStop{false};
    std::string m_SSEBuffer;

    // Response tracking
    struct PendingRequest {
        std::string RequestID;
        std::promise<std::string> Promise;
        std::chrono::steady_clock::time_point StartTime;
    };

    std::unordered_map<std::string, std::unique_ptr<PendingRequest>> m_PendingRequests;
    mutable Poco::Mutex m_RequestsMutex;
    mutable Poco::Mutex m_ConnectionMutex;
};

// HTTP Server Transport Request Handler
class MCPHTTPRequestHandler final : public Poco::Net::HTTPRequestHandler {
  public:
    explicit MCPHTTPRequestHandler(HTTPTransportServer* InServer);
    ~MCPHTTPRequestHandler() noexcept override;
    void handleRequest(Poco::Net::HTTPServerRequest& InRequest,
                       Poco::Net::HTTPServerResponse& InResponse) override;

  private:
    HTTPTransportServer* m_Server;
};

// HTTP Server Transport Request Handler Factory
class MCPHTTPRequestHandlerFactory final : public Poco::Net::HTTPRequestHandlerFactory {
  public:
    explicit MCPHTTPRequestHandlerFactory(HTTPTransportServer* InServer);
    ~MCPHTTPRequestHandlerFactory() noexcept override;
    [[nodiscard]] Poco::Net::HTTPRequestHandler*
    createRequestHandler(const Poco::Net::HTTPServerRequest& InRequest) override;

  private:
    HTTPTransportServer* m_Server;
};

// HTTP Server Transport
class HTTPTransportServer : public ITransport {
  public:
    explicit HTTPTransportServer(const HTTPTransportOptions& InOptions);
    ~HTTPTransportServer() noexcept override;

    // ITransport interface
    MCPTask_Void Start() override;
    MCPTask_Void Stop() override;
    [[nodiscard]] bool IsConnected() const override;
    [[nodiscard]] TransportState GetState() const override;

    MCPTask_Void TransmitMessage(const JSONValue& InMessage) override;

    [[nodiscard]] std::string GetConnectionInfo() const override;

    // Server-specific methods
    void HandleHTTPRequest(Poco::Net::HTTPServerRequest& InRequest,
                           Poco::Net::HTTPServerResponse& InResponse);
    MCPTask_Void HandleGetMessageEndpoint(Poco::Net::HTTPServerRequest& InRequest,
                                          Poco::Net::HTTPServerResponse& InResponse);
    void RegisterSSEClient(const std::string& InClientID,
                           Poco::Net::HTTPServerResponse& InResponse);
    void UnregisterSSEClient(const std::string& InClientID);
    MCPTask_Void StreamMessagesToClient(const std::string& InClientID);
    std::string GenerateUniqueClientID() const;

  private:
    void ProcessReceivedMessage(const std::string& InMessage);
    std::string GenerateClientID() const;

    HTTPTransportOptions m_Options;
    std::unique_ptr<Poco::Net::HTTPServer> m_HTTPServer;
    std::unique_ptr<Poco::Net::ServerSocket> m_ServerSocket;
    std::unique_ptr<MCPHTTPRequestHandlerFactory> m_HandlerFactory;

    // SSE client management
    struct SSEClient {
        std::string ClientID;
        Poco::Net::HTTPServerResponse* Response;
        std::ostream* Stream;
        std::chrono::steady_clock::time_point ConnectedTime;
        bool IsActive;
    };

    std::unordered_map<std::string, std::unique_ptr<SSEClient>> m_SSEClients;
    mutable Poco::Mutex m_ClientsMutex;

    // Response tracking
    struct PendingRequest {
        std::string RequestID;
        std::promise<std::string> Promise;
        std::chrono::steady_clock::time_point StartTime;
    };

    std::unordered_map<std::string, std::unique_ptr<PendingRequest>> m_PendingRequests;
    mutable Poco::Mutex m_RequestsMutex;

    std::atomic<uint64_t> m_ClientCounter{0};
};

MCP_NAMESPACE_END