#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "Core.h"

// Platform-specific includes
#if defined(_WIN32) || defined(_WIN64)
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
using SocketType = SOCKET;
    #define INVALID_SOCKET_HANDLE INVALID_SOCKET
    #define CLOSE_SOCKET closesocket
#else
    #include <arpa/inet.h>
    #include <fcntl.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <poll.h>
    #include <sys/socket.h>
    #include <unistd.h>
using SocketType = int;
    #define INVALID_SOCKET_HANDLE (-1)
    #define CLOSE_SOCKET close
#endif

MCP_NAMESPACE_BEGIN

namespace HTTP {

// HTTP Method enum
enum class Method { GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS };

// HTTP Status codes
enum class Status {
    OK = 200,
    Created = 201,
    MovedPermanently = 301,
    Found = 302,
    SeeOther = 303,
    TemporaryRedirect = 307,
    PermanentRedirect = 308,
    BadRequest = 400,
    Unauthorized = 401,
    Forbidden = 403,
    NotFound = 404,
    InternalServerError = 500
};

// URL parsing result
struct URL {
    std::string scheme;
    std::string host;
    int port;
    std::string path;
    std::string query;
    std::string fragment;
};

// Authentication types
struct BasicAuth {
    std::string username;
    std::string password;
};

struct BearerAuth {
    std::string token;
};

using Auth = std::variant<BasicAuth, BearerAuth>;

// Proxy configuration
struct ProxyConfig {
    enum class Type { HTTP, SOCKS4, SOCKS5 };
    Type type;
    std::string host;
    int port;
    std::optional<Auth> auth;
};

// Request configuration
struct RequestConfig {
    std::string url;
    Method method = Method::GET;
    std::map<std::string, std::string> headers;
    std::string body;
    int timeout_ms = 30000;
    bool follow_redirects = true;
    std::optional<Auth> auth;
    std::optional<ProxyConfig> proxy;
    bool keep_alive = true;
    bool compress = false;
    bool chunked = false;
    std::chrono::milliseconds retry_delay{1000};
    int max_retries = 3;
};

// Response structure
struct Response {
    Status status;
    std::map<std::string, std::string> headers;
    std::string body;
    std::string error;
    std::chrono::system_clock::time_point timestamp;
    bool from_cache = false;
};

// Progress callback type
using ProgressCallback = std::function<void(size_t downloaded, size_t total)>;

// Event types for async operations
struct ConnectionEvent {
    enum class Type { Connected, Disconnected, Error };
    Type type;
    std::string message;
};

struct DataEvent {
    std::vector<uint8_t> data;
    bool is_complete;
};

using Event = std::variant<ConnectionEvent, DataEvent>;

// Event callback type
using EventCallback = std::function<void(const Event&)>;

// Main HTTP client class
class Client {
  public:
    Client();
    ~Client();

    // Synchronous request methods
    Response request(const RequestConfig& config);
    Response get(const std::string& url, const std::map<std::string, std::string>& headers = {});
    Response post(const std::string& url, const std::string& body,
                  const std::map<std::string, std::string>& headers = {});

    // Asynchronous request methods
    std::future<Response> requestAsync(const RequestConfig& config);
    std::future<Response> getAsync(const std::string& url,
                                   const std::map<std::string, std::string>& headers = {});
    std::future<Response> postAsync(const std::string& url, const std::string& body,
                                    const std::map<std::string, std::string>& headers = {});

    // Event-based async methods
    void requestWithEvents(const RequestConfig& config, EventCallback callback);
    void getWithEvents(const std::string& url, EventCallback callback,
                       const std::map<std::string, std::string>& headers = {});
    void postWithEvents(const std::string& url, const std::string& body, EventCallback callback,
                        const std::map<std::string, std::string>& headers = {});

    // SSE (Server-Sent Events) support
    void subscribeSSE(const std::string& url, std::function<void(const std::string&)> onEvent,
                      std::function<void(const std::string&)> onError = nullptr,
                      const std::map<std::string, std::string>& headers = {});

    // WebSocket support
    void connectWebSocket(const std::string& url, std::function<void(const std::string&)> onMessage,
                          std::function<void()> onOpen = nullptr,
                          std::function<void()> onClose = nullptr,
                          std::function<void(const std::string&)> onError = nullptr);

    // Progress tracking
    void setProgressCallback(ProgressCallback callback);

    // Configuration methods
    void setTimeout(int milliseconds);
    void setFollowRedirects(bool follow);
    void setMaxConnections(size_t max);
    void setConnectionTimeout(int milliseconds);
    void setRetryPolicy(int max_retries, std::chrono::milliseconds delay);
    void setProxy(const ProxyConfig& proxy);
    void setAuth(const Auth& auth);
    void enableCompression(bool enable);
    void enableChunkedTransfer(bool enable);
    void setKeepAlive(bool enable);

  private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace HTTP

MCP_NAMESPACE_END