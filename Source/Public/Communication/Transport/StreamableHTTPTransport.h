#pragma once

#include <functional>
#include <future>
#include <memory>
#include <string>

#include "Communication/Transport/MCP_HTTPTypes.h"
#include "Communication/Transport/MCP_Transport.h"

namespace MCP {

class MCP_StreamableHTTPTransport : public MCP_Transport {
  public:
    MCP_StreamableHTTPTransport(const std::string& endpoint,
                                std::function<std::string()> sessionIdGenerator = nullptr);
    ~MCP_StreamableHTTPTransport() override;

    // Transport interface implementation
    std::future<void> Start() override;
    std::future<void> Send(const MCP_MessageBase& message,
                           const MCP_TransportSendOptions& options) override;
    std::future<void> Close() override;
    void SetCloseCallback(CloseCallback callback) override;
    void SetErrorCallback(ErrorCallback callback) override;
    void SetMessageCallback(MessageCallback callback) override;
    std::optional<std::string> GetSessionId() const override;

    // HTTP-specific methods
    std::future<void> HandleRequest(const HTTPRequest& request, HTTPResponse& response);

  private:
    void HandleMessage(const MCP_MessageBase& message, const AuthInfo* authInfo);
    void WriteStreamResponse(const MCP_MessageBase& message, const std::string& eventId);

    std::string _endpoint;
    std::string _sessionId;
    bool _started = false;
    std::shared_ptr<HTTPResponse> _activeResponse;
    CloseCallback _closeCallback;
    ErrorCallback _errorCallback;
    MessageCallback _messageCallback;

    static constexpr size_t MAXIMUM_MESSAGE_SIZE = 1024 * 1024; // 1MB
};

} // namespace MCP