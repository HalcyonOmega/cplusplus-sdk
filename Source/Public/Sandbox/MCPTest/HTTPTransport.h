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
#include <unordered_map>
#include <unordered_set>

#include "ITransport.h"

// Forward declarations
class HTTPTransportClient;
class HTTPTransportServer;

// HTTP Client Transport
class HTTPTransportClient : public ITransport, public Poco::Runnable {
  public:
    explicit HTTPTransportClient(const HTTPTransportOptions& InOptions);
    ~HTTPTransportClient() override;

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
    // Poco::Runnable interface for SSE reading
    void run() override;

  private:
    MCPTaskVoid ConnectToServer();
    MCPTaskVoid SendHTTPMessage(const nlohmann::json& InMessage);
    void StartSSEConnection();
    void ProcessSSELine(const std::string& InLine);
    void HandleConnectionError(const std::string& InError);
    void Cleanup();

    HTTPTransportOptions m_Options;
    std::unique_ptr<Poco::Net::HTTPClientSession> m_HTTPSession;
    std::unique_ptr<std::istream> m_SSEStream;

    Poco::Thread m_SSEThread;
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
class MCPHTTPRequestHandler : public Poco::Net::HTTPRequestHandler {
  public:
    explicit MCPHTTPRequestHandler(HTTPTransportServer* InServer);
    void handleRequest(Poco::Net::HTTPServerRequest& InRequest,
                       Poco::Net::HTTPServerResponse& InResponse) override;

  private:
    HTTPTransportServer* m_Server;
};

// HTTP Server Transport Request Handler Factory
class MCPHTTPRequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory {
  public:
    explicit MCPHTTPRequestHandlerFactory(HTTPTransportServer* InServer);
    Poco::Net::HTTPRequestHandler*
    createRequestHandler(const Poco::Net::HTTPServerRequest& InRequest) override;

  private:
    HTTPTransportServer* m_Server;
};

// HTTP Server Transport
class HTTPTransportServer : public ITransport {
  public:
    explicit HTTPTransportServer(const HTTPTransportOptions& InOptions);
    ~HTTPTransportServer() override;

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

    // Server-specific methods
    void HandleHTTPRequest(Poco::Net::HTTPServerRequest& InRequest,
                           Poco::Net::HTTPServerResponse& InResponse);
    MCPTaskVoid HandleGetMessageEndpoint(Poco::Net::HTTPServerRequest& InRequest,
                                         Poco::Net::HTTPServerResponse& InResponse);
    void RegisterSSEClient(const std::string& InClientID,
                           Poco::Net::HTTPServerResponse& InResponse);
    void UnregisterSSEClient(const std::string& InClientID);
    MCPTaskVoid StreamMessagesToClient(const std::string& InClientID);
    std::string GenerateUniqueClientID() const;

  private:
    MCPTaskVoid SendToSSEClients(const nlohmann::json& InMessage);
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