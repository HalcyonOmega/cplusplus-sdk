#include "HTTP.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <deque>
#include <random>
#include <regex>
#include <shared_mutex>
#include <sstream>

MCP_NAMESPACE_BEGIN

namespace HTTP {

namespace {
constexpr int DEFAULT_HTTP_PORT = 80;
constexpr int DEFAULT_HTTPS_PORT = 443;
constexpr int BUFFER_SIZE = 4096;
constexpr int MAX_REDIRECTS = 5;
constexpr size_t DEFAULT_MAX_CONNECTIONS = 10;
constexpr int DEFAULT_CONNECTION_TIMEOUT = 5000;

// Connection pool entry
struct Connection {
    SocketType socket;
    std::string host;
    int port;
    std::chrono::system_clock::time_point last_used;
    bool in_use;
};

// Connection pool
class ConnectionPool {
  public:
    ConnectionPool(size_t max_size) : _max_size(max_size) {}

    // Add move constructor
    ConnectionPool(ConnectionPool&& other) noexcept
        : _connections(std::move(other._connections)), _max_size(other._max_size) {
        other._max_size = 0;
    }

    // Add move assignment operator
    ConnectionPool& operator=(ConnectionPool&& other) noexcept {
        if (this != &other) {
            _connections = std::move(other._connections);
            _max_size = other._max_size;
            other._max_size = 0;
        }
        return *this;
    }

    // Delete copy constructor and assignment operator
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

    std::optional<SocketType> acquire(const std::string& host, int port) {
        std::unique_lock<std::shared_mutex> lock(_mutex);

        // Try to find an existing connection
        auto now = std::chrono::system_clock::now();
        for (auto& conn : _connections) {
            if (!conn.in_use && conn.host == host && conn.port == port) {
                conn.in_use = true;
                conn.last_used = now;
                return conn.socket;
            }
        }

        // Create new connection if pool isn't full
        if (_connections.size() < _max_size) {
            SocketType sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (sock != INVALID_SOCKET_HANDLE) {
                Connection conn{sock, host, port, now, true};
                _connections.push_back(conn);
                return sock;
            }
        }

        return std::nullopt;
    }

    void release(SocketType sock) {
        std::unique_lock<std::shared_mutex> lock(_mutex);
        for (auto& conn : _connections) {
            if (conn.socket == sock) {
                conn.in_use = false;
                break;
            }
        }
    }

    void cleanup() {
        std::unique_lock<std::shared_mutex> lock(_mutex);
        auto now = std::chrono::system_clock::now();
        auto it = _connections.begin();
        while (it != _connections.end()) {
            if (!it->in_use && (now - it->last_used) > std::chrono::seconds(30)) {
                CLOSE_SOCKET(it->socket);
                it = _connections.erase(it);
            } else {
                ++it;
            }
        }
    }

  private:
    std::deque<Connection> _connections;
    size_t _max_size;
    std::shared_mutex _mutex;
};

// Basic compression using run-length encoding
std::string compressData(const std::string& data) {
    if (data.empty()) return data;

    std::string compressed;
    compressed.reserve(data.size());

    char current = data[0];
    int count = 1;

    for (size_t i = 1; i < data.size(); ++i) {
        if (data[i] == current && count < 255) {
            ++count;
        } else {
            compressed.push_back(static_cast<char>(count));
            compressed.push_back(current);
            current = data[i];
            count = 1;
        }
    }

    // Add the last run
    compressed.push_back(static_cast<char>(count));
    compressed.push_back(current);

    return compressed;
}

// Basic decompression
std::string decompressData(const std::string& data) {
    if (data.empty()) return data;

    std::string decompressed;
    decompressed.reserve(data.size() * 2); // Estimate size

    for (size_t i = 0; i < data.size(); i += 2) {
        if (i + 1 >= data.size()) break;

        int count = static_cast<unsigned char>(data[i]);
        char value = data[i + 1];

        decompressed.append(count, value);
    }

    return decompressed;
}

// Helper function to parse URL
URL parseURL(const std::string& url) {
    URL result;
    std::regex url_regex(R"(^(https?)://([^:/]+)(?::(\d+))?(/[^?#]*)?(?:\?([^#]*))?(?:#(.*))?$)");
    std::smatch matches;

    if (std::regex_match(url, matches, url_regex)) {
        result.scheme = matches[1].str();
        result.host = matches[2].str();
        result.port = matches[3].matched
                          ? std::stoi(matches[3].str())
                          : (result.scheme == "https" ? DEFAULT_HTTPS_PORT : DEFAULT_HTTP_PORT);
        result.path = matches[4].matched ? matches[4].str() : "/";
        result.query = matches[5].matched ? matches[5].str() : "";
        result.fragment = matches[6].matched ? matches[6].str() : "";
    }

    return result;
}

// Helper function to create HTTP request string
std::string createRequestString(const RequestConfig& config, const URL& url) {
    std::stringstream ss;

    // Request line
    ss << (config.method == Method::GET      ? "GET"
           : config.method == Method::POST   ? "POST"
           : config.method == Method::PUT    ? "PUT"
           : config.method == Method::DELETE ? "DELETE"
           : config.method == Method::PATCH  ? "PATCH"
           : config.method == Method::HEAD   ? "HEAD"
                                             : "OPTIONS");

    ss << " " << url.path;
    if (!url.query.empty()) { ss << "?" << url.query; }
    ss << " HTTP/1.1\r\n";

    // Headers
    ss << "Host: " << url.host;
    if (url.port != DEFAULT_HTTP_PORT && url.port != DEFAULT_HTTPS_PORT) { ss << ":" << url.port; }
    ss << "\r\n";

    // Add custom headers
    for (const auto& [key, value] : config.headers) { ss << key << ": " << value << "\r\n"; }

    // Add content length if there's a body
    if (!config.body.empty()) {
        if (config.compress) {
            std::string compressed = compressData(config.body);
            ss << "Content-Encoding: gzip\r\n";
            ss << "Content-Length: " << compressed.length() << "\r\n";
        } else {
            ss << "Content-Length: " << config.body.length() << "\r\n";
        }
    }

    // Add keep-alive header
    if (config.keep_alive) {
        ss << "Connection: keep-alive\r\n";
    } else {
        ss << "Connection: close\r\n";
    }

    // Add chunked transfer encoding if requested
    if (config.chunked) { ss << "Transfer-Encoding: chunked\r\n"; }

    ss << "\r\n";

    // Add body if present
    if (!config.body.empty()) {
        if (config.compress) {
            ss << compressData(config.body);
        } else {
            ss << config.body;
        }
    }

    return ss.str();
}

// Helper function to parse HTTP response
Response parseResponse(const std::string& response_str) {
    Response response;
    std::istringstream stream(response_str);
    std::string line;

    // Parse status line
    if (std::getline(stream, line)) {
        std::regex status_regex(R"(^HTTP/\d\.\d\s+(\d+)\s+(.*)$)");
        std::smatch matches;
        if (std::regex_match(line, matches, status_regex)) {
            response.status = static_cast<Status>(std::stoi(matches[1].str()));
        }
    }

    // Parse headers
    while (std::getline(stream, line) && !line.empty() && line != "\r") {
        size_t pos = line.find(':');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            // Trim whitespace
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            response.headers[key] = value;
        }
    }

    // Get body
    response.body = stream.str().substr(stream.tellg());

    // Decompress if needed
    if (response.headers.count("Content-Encoding") > 0
        && response.headers["Content-Encoding"] == "gzip") {
        response.body = decompressData(response.body);
    }

    response.timestamp = std::chrono::system_clock::now();

    return response;
}

// Platform-specific socket initialization
bool initializeSockets() {
#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
    return true;
#endif
}

// Platform-specific socket cleanup
void cleanupSockets() {
#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif
}

// Platform-specific socket error
int getLastSocketError() {
#if defined(_WIN32) || defined(_WIN64)
    return WSAGetLastError();
#else
    return errno;
#endif
}

// Platform-specific socket error message
std::string getSocketErrorMessage(int error) {
#if defined(_WIN32) || defined(_WIN64)
    char* message = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&message, 0, nullptr);
    std::string result = message ? message : "Unknown error";
    LocalFree(message);
    return result;
#else
    return strerror(error);
#endif
}

// Platform-specific socket timeout setting
bool setSocketTimeout(SocketType sock, int timeout_ms) {
#if defined(_WIN32) || defined(_WIN64)
    DWORD timeout = timeout_ms;
    return setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) == 0
           && setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout))
                  == 0;
#else
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    return setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0
           && setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == 0;
#endif
}

// Platform-specific non-blocking socket setting
bool setNonBlocking(SocketType sock) {
#if defined(_WIN32) || defined(_WIN64)
    u_long mode = 1;
    return ioctlsocket(sock, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(sock, F_GETFL, 0);
    return fcntl(sock, F_SETFL, flags | O_NONBLOCK) != -1;
#endif
}
} // namespace

// PIMPL implementation
class Client::Impl {
  public:
    Impl()
        : _timeout(30000), _followRedirects(true), _max_connections(DEFAULT_MAX_CONNECTIONS),
          _connection_timeout(DEFAULT_CONNECTION_TIMEOUT), _pool(_max_connections) {
        if (!initializeSockets()) {
            throw std::runtime_error("Failed to initialize network subsystem");
        }
        _workerThread = std::thread(&Impl::processQueue, this);
        _cleanupThread = std::thread(&Impl::cleanupConnections, this);
    }

    ~Impl() {
        _running = false;
        _queueCV.notify_all();
        _cleanupCV.notify_all();
        if (_workerThread.joinable()) { _workerThread.join(); }
        if (_cleanupThread.joinable()) { _cleanupThread.join(); }
        cleanupSockets();
    }

    Response request(const RequestConfig& config) {
        URL url = parseURL(config.url);
        if (url.host.empty()) {
            return Response{
                Status::BadRequest, {}, {}, "Invalid URL", std::chrono::system_clock::now()};
        }

        // Handle redirects
        int redirect_count = 0;
        std::string current_url = config.url;
        RequestConfig current_config = config;

        while (redirect_count < MAX_REDIRECTS) {
            Response response = performRequest(current_config);

            if (_followRedirects
                && (response.status == Status::MovedPermanently || response.status == Status::Found
                    || response.status == Status::SeeOther
                    || response.status == Status::TemporaryRedirect
                    || response.status == Status::PermanentRedirect)
                && response.headers.count("Location") > 0) {
                current_url = response.headers["Location"];
                current_config.url = current_url;
                redirect_count++;
                continue;
            }

            return response;
        }

        return Response{Status::InternalServerError,
                        {},
                        {},
                        "Too many redirects",
                        std::chrono::system_clock::now()};
    }

    // Helper function to perform a single request
    Response performRequest(const RequestConfig& config) {
        URL url = parseURL(config.url);
        if (url.host.empty()) {
            return Response{
                Status::BadRequest, {}, {}, "Invalid URL", std::chrono::system_clock::now()};
        }

        // Try to get a connection from the pool
        auto sock = _pool.acquire(url.host, url.port);
        if (!sock) {
            return Response{Status::InternalServerError,
                            {},
                            {},
                            "No available connections",
                            std::chrono::system_clock::now()};
        }

        // Set socket options
        if (!setSocketTimeout(*sock, _timeout) || !setNonBlocking(*sock)) {
            _pool.release(*sock);
            return Response{Status::InternalServerError,
                            {},
                            {},
                            "Failed to set socket options: "
                                + getSocketErrorMessage(getLastSocketError()),
                            std::chrono::system_clock::now()};
        }

        // Connect to server
        struct hostent* server = gethostbyname(url.host.c_str());
        if (!server) {
            _pool.release(*sock);
            return Response{Status::InternalServerError,
                            {},
                            {},
                            "Failed to resolve hostname: "
                                + getSocketErrorMessage(getLastSocketError()),
                            std::chrono::system_clock::now()};
        }

        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
        serv_addr.sin_port = htons(url.port);

        // Start connection
        int ret = connect(*sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        if (ret < 0) {
#if defined(_WIN32) || defined(_WIN64)
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
#else
            if (errno != EINPROGRESS) {
#endif
                _pool.release(*sock);
                return Response{Status::InternalServerError,
                                {},
                                {},
                                "Failed to connect: " + getSocketErrorMessage(getLastSocketError()),
                                std::chrono::system_clock::now()};
            }
        }

        // Wait for connection with timeout
#if defined(_WIN32) || defined(_WIN64)
        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(*sock, &write_fds);
        struct timeval tv;
        tv.tv_sec = _timeout / 1000;
        tv.tv_usec = (_timeout % 1000) * 1000;
        ret = select(0, nullptr, &write_fds, nullptr, &tv);
#else
        struct pollfd pfd = {*sock, POLLOUT, 0};
        ret = poll(&pfd, 1, _timeout);
#endif

        if (ret <= 0) {
            _pool.release(*sock);
            return Response{Status::InternalServerError,
                            {},
                            {},
                            "Connection timeout",
                            std::chrono::system_clock::now()};
        }

        // Send request
        std::string request_str = createRequestString(config, url);
        ssize_t sent = send(*sock, request_str.c_str(), request_str.length(), 0);
        if (sent < 0) {
            _pool.release(*sock);
            return Response{Status::InternalServerError,
                            {},
                            {},
                            "Failed to send request: "
                                + getSocketErrorMessage(getLastSocketError()),
                            std::chrono::system_clock::now()};
        }

        // Receive response
        std::string response_str;
        char buffer[BUFFER_SIZE];

#if defined(_WIN32) || defined(_WIN64)
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(*sock, &read_fds);
#else
        struct pollfd rfd = {*sock, POLLIN, 0};
#endif

        while (true) {
#if defined(_WIN32) || defined(_WIN64)
            ret = select(0, &read_fds, nullptr, nullptr, &tv);
#else
            ret = poll(&rfd, 1, _timeout);
#endif

            if (ret <= 0) break;

            ssize_t received = recv(*sock, buffer, BUFFER_SIZE - 1, 0);
            if (received <= 0) break;

            buffer[received] = '\0';
            response_str.append(buffer, received);
        }

        // Release connection back to pool if keep-alive
        if (config.keep_alive) {
            _pool.release(*sock);
        } else {
            CLOSE_SOCKET(*sock);
        }

        if (response_str.empty()) {
            return Response{Status::InternalServerError,
                            {},
                            {},
                            "Empty response",
                            std::chrono::system_clock::now()};
        }

        Response response = parseResponse(response_str);

        return response;
    }

    std::future<Response> requestAsync(const RequestConfig& config) {
        auto promise = std::make_shared<std::promise<Response>>();
        auto future = promise->get_future();

        {
            std::lock_guard<std::mutex> lock(_queueMutex);
            _requestQueue.push({config, std::move(promise)});
        }
        _queueCV.notify_one();

        return future;
    }

    void requestWithEvents(const RequestConfig& config, EventCallback callback) {
        std::thread([this, config, callback]() {
            try {
                URL url = parseURL(config.url);
                if (url.host.empty()) {
                    callback(ConnectionEvent{ConnectionEvent::Type::Error, "Invalid URL"});
                    return;
                }

                auto sock = _pool.acquire(url.host, url.port);
                if (!sock) {
                    callback(
                        ConnectionEvent{ConnectionEvent::Type::Error, "No available connections"});
                    return;
                }

                // Connect and send request
                // ... (similar to request() but with event callbacks)

                callback(ConnectionEvent{ConnectionEvent::Type::Connected, "Connected"});

                // Receive data in chunks
                char buffer[BUFFER_SIZE];
                while (true) {
                    ssize_t received = recv(*sock, buffer, BUFFER_SIZE - 1, 0);
                    if (received <= 0) break;

                    buffer[received] = '\0';
                    callback(DataEvent{{buffer, buffer + received}, false});
                }

                callback(DataEvent{{}, true});
                callback(ConnectionEvent{ConnectionEvent::Type::Disconnected, "Disconnected"});

                if (config.keep_alive) {
                    _pool.release(*sock);
                } else {
                    CLOSE_SOCKET(*sock);
                }
            } catch (const std::exception& e) {
                callback(ConnectionEvent{ConnectionEvent::Type::Error, e.what()});
            }
        }).detach();
    }

    void subscribeSSE(const std::string& url, std::function<void(const std::string&)> onEvent,
                      std::function<void(const std::string&)> onError,
                      const std::map<std::string, std::string>& headers) {
        RequestConfig config;
        config.url = url;
        config.headers = headers;
        config.headers["Accept"] = "text/event-stream";
        config.headers["Cache-Control"] = "no-cache";
        config.headers["Connection"] = "keep-alive";

        requestWithEvents(config, [onEvent, onError](const Event& event) {
            if (auto* conn_event = std::get_if<ConnectionEvent>(&event)) {
                if (conn_event->type == ConnectionEvent::Type::Error && onError) {
                    onError(conn_event->message);
                }
            } else if (auto* data_event = std::get_if<DataEvent>(&event)) {
                if (!data_event->is_complete) {
                    std::string data(data_event->data.begin(), data_event->data.end());
                    std::istringstream stream(data);
                    std::string line;
                    while (std::getline(stream, line)) {
                        if (line.substr(0, 6) == "data: ") { onEvent(line.substr(6)); }
                    }
                }
            }
        });
    }

    void connectWebSocket(const std::string& url, std::function<void(const std::string&)> onMessage,
                          std::function<void()> onOpen, std::function<void()> onClose,
                          std::function<void(const std::string&)> onError) {
        RequestConfig config;
        config.url = url;
        config.method = Method::GET;
        config.headers["Upgrade"] = "websocket";
        config.headers["Connection"] = "Upgrade";
        config.headers["Sec-WebSocket-Version"] = "13";

        // Generate a random WebSocket key
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);
        const char* hex = "0123456789abcdef";
        std::string key;
        for (int i = 0; i < 16; ++i) { key += hex[dis(gen)]; }
        config.headers["Sec-WebSocket-Key"] = key;

        requestWithEvents(config, [onMessage, onOpen, onClose, onError](const Event& event) {
            if (auto* conn_event = std::get_if<ConnectionEvent>(&event)) {
                if (conn_event->type == ConnectionEvent::Type::Connected && onOpen) {
                    onOpen();
                } else if (conn_event->type == ConnectionEvent::Type::Disconnected && onClose) {
                    onClose();
                } else if (conn_event->type == ConnectionEvent::Type::Error && onError) {
                    onError(conn_event->message);
                }
            } else if (auto* data_event = std::get_if<DataEvent>(&event)) {
                if (!data_event->is_complete && onMessage) {
                    std::string message(data_event->data.begin(), data_event->data.end());
                    onMessage(message);
                }
            }
        });
    }

    void setProgressCallback(ProgressCallback callback) {
        _progressCallback = callback;
    }

    void setTimeout(int milliseconds) {
        _timeout = milliseconds;
    }

    void setFollowRedirects(bool follow) {
        _followRedirects = follow;
    }

    void setMaxConnections(size_t max) {
        _max_connections = max;
        _pool = ConnectionPool(max);
    }

    void setConnectionTimeout(int milliseconds) {
        _connection_timeout = milliseconds;
    }

    void setRetryPolicy(int max_retries, std::chrono::milliseconds delay) {
        _max_retries = max_retries;
        _retry_delay = delay;
    }

    void setProxy(const ProxyConfig& proxy) {
        _proxy = proxy;
    }

    void setAuth(const Auth& auth) {
        _auth = auth;
    }

    void enableCompression(bool enable) {
        _compress = enable;
    }

    void enableChunkedTransfer(bool enable) {
        _chunked = enable;
    }

    void setKeepAlive(bool enable) {
        _keep_alive = enable;
    }

  private:
    void processQueue() {
        while (_running) {
            std::unique_lock<std::mutex> lock(_queueMutex);
            _queueCV.wait(lock, [this] { return !_requestQueue.empty() || !_running; });

            while (!_requestQueue.empty()) {
                auto [config, promise] = std::move(_requestQueue.front());
                _requestQueue.pop();
                lock.unlock();

                try {
                    Response response = request(config);
                    promise->set_value(response);
                } catch (const std::exception& e) {
                    Response error_response;
                    error_response.status = Status::InternalServerError;
                    error_response.error = e.what();
                    promise->set_value(error_response);
                }

                lock.lock();
            }
        }
    }

    void cleanupConnections() {
        while (_running) {
            std::unique_lock<std::mutex> lock(_cleanupMutex);
            _cleanupCV.wait_for(lock, std::chrono::seconds(30), [this] { return !_running; });

            if (_running) { _pool.cleanup(); }
        }
    }

    std::thread _workerThread;
    std::thread _cleanupThread;
    std::queue<std::pair<RequestConfig, std::shared_ptr<std::promise<Response>>>> _requestQueue;
    std::mutex _queueMutex;
    std::mutex _cleanupMutex;
    std::condition_variable _queueCV;
    std::condition_variable _cleanupCV;
    std::atomic<bool> _running{true};
    ProgressCallback _progressCallback;
    int _timeout;
    bool _followRedirects;
    size_t _max_connections;
    int _connection_timeout;
    int _max_retries;
    std::chrono::milliseconds _retry_delay;
    std::optional<ProxyConfig> _proxy;
    std::optional<Auth> _auth;
    bool _compress;
    bool _chunked;
    bool _keep_alive;
    ConnectionPool _pool;
};

// Client implementation
Client::Client() : pImpl(std::make_unique<Impl>()) {}
Client::~Client() = default;

Response Client::request(const RequestConfig& config) {
    return pImpl->request(config);
}

Response Client::get(const std::string& url, const std::map<std::string, std::string>& headers) {
    RequestConfig config;
    config.url = url;
    config.method = Method::GET;
    config.headers = headers;
    return request(config);
}

Response Client::post(const std::string& url, const std::string& body,
                      const std::map<std::string, std::string>& headers) {
    RequestConfig config;
    config.url = url;
    config.method = Method::POST;
    config.body = body;
    config.headers = headers;
    return request(config);
}

std::future<Response> Client::requestAsync(const RequestConfig& config) {
    return pImpl->requestAsync(config);
}

std::future<Response> Client::getAsync(const std::string& url,
                                       const std::map<std::string, std::string>& headers) {
    RequestConfig config;
    config.url = url;
    config.method = Method::GET;
    config.headers = headers;
    return requestAsync(config);
}

std::future<Response> Client::postAsync(const std::string& url, const std::string& body,
                                        const std::map<std::string, std::string>& headers) {
    RequestConfig config;
    config.url = url;
    config.method = Method::POST;
    config.body = body;
    config.headers = headers;
    return requestAsync(config);
}

void Client::requestWithEvents(const RequestConfig& config, EventCallback callback) {
    pImpl->requestWithEvents(config, callback);
}

void Client::getWithEvents(const std::string& url, EventCallback callback,
                           const std::map<std::string, std::string>& headers) {
    RequestConfig config;
    config.url = url;
    config.method = Method::GET;
    config.headers = headers;
    requestWithEvents(config, callback);
}

void Client::postWithEvents(const std::string& url, const std::string& body, EventCallback callback,
                            const std::map<std::string, std::string>& headers) {
    RequestConfig config;
    config.url = url;
    config.method = Method::POST;
    config.body = body;
    config.headers = headers;
    requestWithEvents(config, callback);
}

void Client::subscribeSSE(const std::string& url, std::function<void(const std::string&)> onEvent,
                          std::function<void(const std::string&)> onError,
                          const std::map<std::string, std::string>& headers) {
    pImpl->subscribeSSE(url, onEvent, onError, headers);
}

void Client::connectWebSocket(const std::string& url,
                              std::function<void(const std::string&)> onMessage,
                              std::function<void()> onOpen, std::function<void()> onClose,
                              std::function<void(const std::string&)> onError) {
    pImpl->connectWebSocket(url, onMessage, onOpen, onClose, onError);
}

void Client::setProgressCallback(ProgressCallback callback) {
    pImpl->setProgressCallback(callback);
}

void Client::setTimeout(int milliseconds) {
    pImpl->setTimeout(milliseconds);
}

void Client::setFollowRedirects(bool follow) {
    pImpl->setFollowRedirects(follow);
}

void Client::setMaxConnections(size_t max) {
    pImpl->setMaxConnections(max);
}

void Client::setConnectionTimeout(int milliseconds) {
    pImpl->setConnectionTimeout(milliseconds);
}

void Client::setRetryPolicy(int max_retries, std::chrono::milliseconds delay) {
    pImpl->setRetryPolicy(max_retries, delay);
}

void Client::setProxy(const ProxyConfig& proxy) {
    pImpl->setProxy(proxy);
}

void Client::setAuth(const Auth& auth) {
    pImpl->setAuth(auth);
}

void Client::enableCompression(bool enable) {
    pImpl->enableCompression(enable);
}

void Client::enableChunkedTransfer(bool enable) {
    pImpl->enableChunkedTransfer(enable);
}

void Client::setKeepAlive(bool enable) {
    pImpl->setKeepAlive(enable);
}

} // namespace HTTP

MCP_NAMESPACE_END