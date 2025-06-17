#include "HTTPProxy.h"

// Poco::Net main headers
#include <Poco/Context.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPMessage.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/NameValueCollection.h>
#include <Poco/Net/NetException.h>
#include <Poco/Net/SSLManager.h> // For HTTPS client default context
#include <Poco/Net/ServerSocket.h>
#include <Poco/StreamCopier.h>
#include <Poco/Timespan.h>
#include <Poco/URI.h>

// Standard library
#include <iostream> // For potential debug logging in helpers
#include <sstream>
#include <stdexcept> // For std::runtime_error in helpers
#include <utility>   // For std::move

MCP_NAMESPACE_BEGIN

// --- Helper Functions for Enum Conversions ---

static std::string ToPocoMethod(HTTP_Method method) {
    switch (method) {
        case HTTP_Method::Get: return Poco::Net::HTTPRequest::HTTP_GET;
        case HTTP_Method::Post: return Poco::Net::HTTPRequest::HTTP_POST;
        case HTTP_Method::Put: return Poco::Net::HTTPRequest::HTTP_PUT;
        case HTTP_Method::Delete: return Poco::Net::HTTPRequest::HTTP_DELETE;
        case HTTP_Method::Head: return Poco::Net::HTTPRequest::HTTP_HEAD;
        case HTTP_Method::Options: return Poco::Net::HTTPRequest::HTTP_OPTIONS;
        case HTTP_Method::Patch: return Poco::Net::HTTPRequest::HTTP_PATCH;
        case HTTP_Method::Connect: return Poco::Net::HTTPRequest::HTTP_CONNECT;
        case HTTP_Method::Trace: return Poco::Net::HTTPRequest::HTTP_TRACE;
        default: throw std::runtime_error("Unknown HTTP_Method to convert to Poco method string");
    }
}

static std::string ToPocoVersion(HTTP_Version version) {
    switch (version) {
        case HTTP_Version::HTTP_1_0: return Poco::Net::HTTPMessage::HTTP_1_0;
        case HTTP_Version::HTTP_1_1: return Poco::Net::HTTPMessage::HTTP_1_1;
        default: throw std::runtime_error("Unknown HTTP_Version to convert to Poco version string");
    }
}

static HTTP_Version FromPocoVersion(const std::string& pocoVersion) {
    if (pocoVersion == Poco::Net::HTTPMessage::HTTP_1_0) return HTTP_Version::HTTP_1_0;
    if (pocoVersion == Poco::Net::HTTPMessage::HTTP_1_1) return HTTP_Version::HTTP_1_1;
    // std::cerr << "Warning: Unknown Poco HTTP version string: " << pocoVersion << ", defaulting
    // to 1.1" << std::endl;
    return HTTP_Version::HTTP_1_1;
}

static Poco::Net::HTTPResponse::HTTP::Status ToPocoStatus(HTTP_Status status) {
    return static_cast<Poco::Net::HTTPResponse::HTTP::Status>(static_cast<int>(status));
}

static HTTP_Status FromPocoStatus(Poco::Net::HTTPResponse::HTTP::Status pocoStatus) {
    auto intStatus = static_cast<int>(pocoStatus);
    switch (intStatus) {
        case 100: return HTTP_Status::CONTINUE_100;
        case 101: return HTTP_Status::SWITCHING_PROTOCOLS_101;
        case 102: return HTTP_Status::PROCESSING_102;
        case 200: return HTTP_Status::OK_200;
        case 201: return HTTP_Status::CREATED_201;
        case 202: return HTTP_Status::ACCEPTED_202;
        case 203: return HTTP_Status::NON_AUTHORITATIVE_INFORMATION_203;
        case 204: return HTTP_Status::NO_CONTENT_204;
        case 205: return HTTP_Status::RESET_CONTENT_205;
        case 206: return HTTP_Status::PARTIAL_CONTENT_206;
        case 300: return HTTP_Status::MULTIPLE_CHOICES_300;
        case 301: return HTTP_Status::MOVED_PERMANENTLY_301;
        case 302: return HTTP_Status::FOUND_302;
        case 303: return HTTP_Status::SEE_OTHER_303;
        case 304: return HTTP_Status::NOT_MODIFIED_304;
        case 307: return HTTP_Status::TEMPORARY_REDIRECT_307;
        case 308: return HTTP_Status::PERMANENT_REDIRECT_308;
        case 400: return HTTP_Status::BAD_REQUEST_400;
        case 401: return HTTP_Status::UNAUTHORIZED_401;
        case 403: return HTTP_Status::FORBIDDEN_403;
        case 404: return HTTP_Status::NOT_FOUND_404;
        case 405: return HTTP_Status::METHOD_NOT_ALLOWED_405;
        case 406: return HTTP_Status::NOT_ACCEPTABLE_406;
        case 408: return HTTP_Status::REQUEST_TIMEOUT_408;
        case 409: return HTTP_Status::CONFLICT_409;
        case 410: return HTTP_Status::GONE_410;
        case 411: return HTTP_Status::LENGTH_REQUIRED_411;
        case 412: return HTTP_Status::PRECONDITION_FAILED_412;
        case 413: return HTTP_Status::PAYLOAD_TOO_LARGE_413;
        case 414: return HTTP_Status::URI_TOO_LONG_414;
        case 415: return HTTP_Status::UNSUPPORTED_MEDIA_TYPE_415;
        case 429: return HTTP_Status::TOO_MANY_REQUESTS_429;
        case 500: return HTTP_Status::INTERNAL_SERVER_ERROR_500;
        case 501: return HTTP_Status::NOT_IMPLEMENTED_501;
        case 502: return HTTP_Status::BAD_GATEWAY_502;
        case 503: return HTTP_Status::SERVICE_UNAVAILABLE_503;
        case 504: return HTTP_Status::GATEWAY_TIMEOUT_504;
        case 505: return HTTP_Status::HTTP_VERSION_NOT_SUPPORTED_505;
        default:
            // std::cerr << "Warning: Unknown Poco HTTP status code: " << intStatus << ", mapping to
            // UNKNOWN" << std::endl;
            return HTTP_Status::UNKNOWN;
    }
}

// --- PImpl Struct Definitions ---

class HTTP::Headers::HeadersImpl {
  public:
    Poco::Net::NameValueCollection pocoHeaders;
};

class HTTP_Client::ClientImpl {
  public:
    std::string host;
    unsigned short port{0};
    bool isSecure{false};
    std::unique_ptr<Poco::Net::HTTPClientSession> session;
    Poco::Net::Context::Ptr httpsContext; // For HTTPS settings

    ClientImpl()
        : port(0), isSecure(false),
          httpsContext(Poco::Net::SSLManager::instance().defaultClientContext()) {}
};

// Default HTTPRequestHandler and Factory for HTTP_Server
class DefaultRequestHandler : public Poco::Net::HTTPRequestHandler {
  public:
    void handleRequest(Poco::Net::HTTPServerRequest& /*request*/,
                       Poco::Net::HTTPServerResponse& response) override {
        response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_IMPLEMENTED);
        response.setContentType("text/html; charset=utf-8");
        response.setChunkedTransferEncoding(true);
        std::ostream& ostr = response.send();
        ostr << "<html><head><title>501 Not Implemented</title></head><body><h1>501 Not "
                "Implemented</h1><p>The server handler for this request is not "
                "implemented.</p></body></html>";
    }
};

class DefaultRequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory {
  public:
    Poco::Net::HTTPRequestHandler*
    createRequestHandler(const Poco::Net::HTTPServerRequest& /*request*/) override {
        return new DefaultRequestHandler;
    }
};

class HTTP_Server::ServerImpl {
  public:
    unsigned short port{0};
    bool isRunning{false};
    std::unique_ptr<Poco::Net::HTTPServer> pocoServer;
    std::unique_ptr<Poco::Net::HTTPRequestHandlerFactory> requestHandlerFactory;

    ServerImpl() : requestHandlerFactory(std::make_unique<DefaultRequestHandlerFactory>()) {}
};

// --- Class Implementations ---

// HTTP_Headers
HTTP_Headers::HTTP_Headers() : m_PImpl(std::make_unique<HeadersImpl>()) {}
HTTP_Headers::~HTTP_Headers() = default;
HTTP_Headers::HTTP_Headers(HTTP_Headers&& other) noexcept : m_PImpl(std::move(other.m_PImpl)) {}
HTTP_Headers& HTTP_Headers::operator=(HTTP_Headers&& other) noexcept {
    if (this != &other) { m_PImpl = std::move(other.m_PImpl); }
    return *this;
}
void HTTP_Headers::Add(const std::string& name, const std::string& value) {
    m_PImpl->pocoHeaders.add(name, value);
}
void HTTP_Headers::Set(const std::string& name, const std::string& value) {
    m_PImpl->pocoHeaders.set(name, value);
}
std::string HTTP_Headers::Get(const std::string& name) const {
    if (m_PImpl->pocoHeaders.has(name)) { return m_PImpl->pocoHeaders.get(name); }
    return "";
}
bool HTTP_Headers::Has(const std::string& name) const {
    return m_PImpl->pocoHeaders.has(name);
}
void HTTP_Headers::Remove(const std::string& name) {
    m_PImpl->pocoHeaders.remove(name);
}
void HTTP_Headers::Clear() {
    m_PImpl->pocoHeaders.clear();
}
// Internal accessors for friend class HTTP_Client
const Poco::Net::NameValueCollection& HTTP_Headers::_GetInternalPocoCollection() const {
    return m_PImpl->pocoHeaders;
}
Poco::Net::NameValueCollection& HTTP_Headers::_GetInternalPocoCollection() {
    return m_PImpl->pocoHeaders;
}

// HTTP_Request
HTTP_Request::HTTP_Request() : m_Method(HTTP_Method::GET), m_Version(HTTP_Version::HTTP_1_1) {}
HTTP_Request::~HTTP_Request() = default;
HTTP_Request::HTTP_Request(HTTP_Request&& other) noexcept
    : m_Method(other.m_Method), m_URI(std::move(other.m_URI)), m_Version(other.m_Version),
      m_Headers(std::move(other.m_Headers)), m_Body(std::move(other.m_Body)) {}
HTTP_Request& HTTP_Request::operator=(HTTP_Request&& other) noexcept {
    if (this != &other) {
        m_Method = other.m_Method;
        m_URI = std::move(other.m_URI);
        m_Version = other.m_Version;
        m_Headers = std::move(other.m_Headers);
        m_Body = std::move(other.m_Body);
    }
    return *this;
}
void HTTP_Request::SetMethod(HTTP_Method method) {
    m_Method = method;
}
HTTP_Method HTTP_Request::GetMethod() const {
    return m_Method;
}
void HTTP_Request::SetURI(const std::string& uri) {
    m_URI = uri;
}
std::string HTTP_Request::GetURI() const {
    return m_URI;
}
HTTP_Headers& HTTP_Request::GetHeaders() {
    return m_Headers;
}
const HTTP_Headers& HTTP_Request::GetHeaders() const {
    return m_Headers;
}
void HTTP_Request::SetBody(const std::string& body) {
    m_Body = body;
}
std::string HTTP_Request::GetBody() const {
    return m_Body;
}
void HTTP_Request::SetVersion(HTTP_Version version) {
    m_Version = version;
}
HTTP_Version HTTP_Request::GetVersion() const {
    return m_Version;
}

// HTTP_Response
HTTP_Response::HTTP_Response()
    : m_Status(HTTP_Status::UNKNOWN), m_Version(HTTP_Version::HTTP_1_1) {}
HTTP_Response::~HTTP_Response() = default;
HTTP_Response::HTTP_Response(HTTP_Response&& other) noexcept
    : m_Status(other.m_Status), m_ReasonPhrase(std::move(other.m_ReasonPhrase)),
      m_Version(other.m_Version), m_Headers(std::move(other.m_Headers)),
      m_Body(std::move(other.m_Body)) {}
HTTP_Response& HTTP_Response::operator=(HTTP_Response&& other) noexcept {
    if (this != &other) {
        m_Status = other.m_Status;
        m_ReasonPhrase = std::move(other.m_ReasonPhrase);
        m_Version = other.m_Version;
        m_Headers = std::move(other.m_Headers);
        m_Body = std::move(other.m_Body);
    }
    return *this;
}
void HTTP_Response::SetStatus(HTTP_Status status) {
    m_Status = status;
}
HTTP_Status HTTP_Response::GetStatus() const {
    return m_Status;
}
void HTTP_Response::SetReasonPhrase(const std::string& reason) {
    m_ReasonPhrase = reason;
}
std::string HTTP_Response::GetReasonPhrase() const {
    return m_ReasonPhrase;
}
HTTP_Headers& HTTP_Response::GetHeaders() {
    return m_Headers;
}
const HTTP_Headers& HTTP_Response::GetHeaders() const {
    return m_Headers;
}
void HTTP_Response::SetBody(const std::string& body) {
    m_Body = body;
}
std::string HTTP_Response::GetBody() const {
    return m_Body;
}
void HTTP_Response::SetVersion(HTTP_Version version) {
    m_Version = version;
}
HTTP_Version HTTP_Response::GetVersion() const {
    return m_Version;
}

// HTTP_Client
HTTP_Client::HTTP_Client() : m_PImpl(std::make_unique<ClientImpl>()) {}
HTTP_Client::~HTTP_Client() = default;
HTTP_Client::HTTP_Client(HTTP_Client&& other) noexcept : m_PImpl(std::move(other.m_PImpl)) {}
HTTP_Client& HTTP_Client::operator=(HTTP_Client&& other) noexcept {
    if (this != &other) { m_PImpl = std::move(other.m_PImpl); }
    return *this;
}
void HTTP_Client::SetHost(const std::string& host) {
    m_PImpl->host = host;
}
void HTTP_Client::SetPort(unsigned short port) {
    m_PImpl->port = port;
}
void HTTP_Client::SetSecure(bool isSecure) {
    m_PImpl->isSecure = isSecure;
}

HTTP_Response HTTP_Client::SendRequest(HTTP_Request& request) {
    HTTP_Response responseWrapper;
    try {
        if (m_PImpl->host.empty()) {
            throw std::runtime_error("Host must be set before sending request.");
        }
        unsigned short portToUse = m_PImpl->port;
        if (portToUse == 0) { portToUse = m_PImpl->isSecure ? 443 : 80; }

        if (m_PImpl->isSecure) {
            m_PImpl->session = std::make_unique<Poco::Net::HTTPSClientSession>(
                m_PImpl->host, portToUse, m_PImpl->httpsContext);
        } else {
            m_PImpl->session =
                std::make_unique<Poco::Net::HTTPClientSession>(m_PImpl->host, portToUse);
        }
        m_PImpl->session->setTimeout(Poco::Timespan(15, 0)); // 15 seconds timeout

        Poco::Net::HTTPRequest pocoRequest(ToPocoMethod(request.GetMethod()), request.GetURI(),
                                           ToPocoVersion(request.GetVersion()));
        pocoRequest.setChunkedTransferEncoding(true); // Good practice for clients

        const auto& requestPocoHeaders = request.GetHeaders()._GetInternalPocoCollection();
        for (const auto& headerPair : requestPocoHeaders) {
            pocoRequest.set(headerPair.first, headerPair.second);
        }

        if (!request.GetBody().empty()) {
            pocoRequest.setContentLength(static_cast<long>(
                request.GetBody().length())); // Poco uses long for setContentLength
            if (!request.GetHeaders().Has(Poco::Net::HTTPMessage::CONTENT_TYPE)) {
                pocoRequest.setContentType("application/json; charset=utf-8"); // Sensible default
            }
        } else if (request.GetMethod() == HTTP_Method::POST
                   || request.GetMethod() == HTTP_Method::PUT
                   || request.GetMethod() == HTTP_Method::PATCH) {
            pocoRequest.setContentLength(0); // Explicitly set 0 for empty bodies for these methods
        }

        std::ostream& ostr = m_PImpl->session->sendRequest(pocoRequest);
        if (!request.GetBody().empty()) { ostr << request.GetBody(); }

        Poco::Net::HTTPResponse pocoResponse;
        std::istream& istr = m_PImpl->session->receiveResponse(pocoResponse);

        responseWrapper.SetStatus(FromPocoStatus(pocoResponse.getStatus()));
        responseWrapper.SetReasonPhrase(pocoResponse.getReason());
        responseWrapper.SetVersion(FromPocoVersion(pocoResponse.getVersion()));

        auto& responseWrapperPocoHeaders =
            responseWrapper.GetHeaders()._GetInternalPocoCollection();
        responseWrapperPocoHeaders.clear();
        for (const auto& headerPair : pocoResponse) {
            responseWrapperPocoHeaders.add(headerPair.first, headerPair.second);
        }

        std::string responseBodyStr;
        Poco::StreamCopier::copyToString(istr, responseBodyStr);
        responseWrapper.SetBody(responseBodyStr);

    } catch (const Poco::Net::NetException& ex) {
        std::cerr << "NetException in SendRequest: " << ex.displayText() << std::endl;
        responseWrapper.SetStatus(HTTP_Status::INTERNAL_SERVER_ERROR_500);
        responseWrapper.SetReasonPhrase(ex.name());
        responseWrapper.SetBody(ex.displayText());
    } catch (const std::exception& ex) {
        std::cerr << "Exception in SendRequest: " << ex.what() << std::endl;
        responseWrapper.SetStatus(HTTP_Status::INTERNAL_SERVER_ERROR_500);
        responseWrapper.SetReasonPhrase("Standard Exception");
        responseWrapper.SetBody(ex.what());
    }
    return responseWrapper;
}

// HTTP_Server
HTTP_Server::HTTP_Server() : m_PImpl(std::make_unique<ServerImpl>()) {}
HTTP_Server::~HTTP_Server() {
    if (m_PImpl && m_PImpl->pocoServer && m_PImpl->isRunning) {
        try {
            Stop();
        } catch (...) { /* Log or ignore, don't throw from destructor */
        }
    }
}
HTTP_Server::HTTP_Server(HTTP_Server&& other) noexcept : m_PImpl(std::move(other.m_PImpl)) {}
HTTP_Server& HTTP_Server::operator=(HTTP_Server&& other) noexcept {
    if (this != &other) { m_PImpl = std::move(other.m_PImpl); }
    return *this;
}
void HTTP_Server::SetPort(unsigned short port) {
    m_PImpl->port = port;
}
unsigned short HTTP_Server::GetPort() const {
    return m_PImpl->port;
}

// Allows user to set a custom request handler factory before starting
void HTTP_Server::SetRequestHandlerFactory(
    std::unique_ptr<Poco::Net::HTTPRequestHandlerFactory> factory) {
    if (m_PImpl->isRunning) {
        throw std::runtime_error("Cannot set RequestHandlerFactory while server is running.");
    }
    if (!factory) {
        m_PImpl->requestHandlerFactory = std::make_unique<DefaultRequestHandlerFactory>();
    } else {
        m_PImpl->requestHandlerFactory = std::move(factory);
    }
}

bool HTTP_Server::Start() {
    if (m_PImpl->isRunning) return true;
    if (m_PImpl->port == 0) {
        // std::cerr << "Error: Port must be set before starting server." << std::endl;
        throw std::runtime_error("Port must be set before starting server.");
    }
    if (!m_PImpl->requestHandlerFactory) {
        // std::cerr << "Error: RequestHandlerFactory not set. Using default." << std::endl;
        m_PImpl->requestHandlerFactory = std::make_unique<DefaultRequestHandlerFactory>();
    }

    try {
        Poco::Net::ServerSocket svs(m_PImpl->port);
        auto params = new Poco::Net::HTTPServerParams;
        params->setKeepAlive(true);
        params->setTimeout(Poco::Timespan(10, 0)); // 10 seconds timeout for server operations
        params->setMaxThreads(4);
        m_PImpl->pocoServer = std::make_unique<Poco::Net::HTTPServer>(
            m_PImpl->requestHandlerFactory.get(), svs, params);
        m_PImpl->pocoServer->start();
        m_PImpl->isRunning = true;
        return true;
    } catch (const Poco::Net::NetException& ex) {
        // std::cerr << "Failed to start HTTP server (NetException): " << ex.displayText() <<
        // std::endl;
        m_PImpl->isRunning = false;
        return false;
    } catch (const std::exception& ex) {
        // std::cerr << "Failed to start HTTP server (StdException): " << ex.what() << std::endl;
        m_PImpl->isRunning = false;
        return false;
    }
}

void HTTP_Server::Stop() {
    if (!m_PImpl->isRunning || !m_PImpl->pocoServer) return;
    try {
        m_PImpl->pocoServer->stopAll(true); // true to wait for threads
        m_PImpl->isRunning = false;
        m_PImpl->pocoServer.reset();
    } catch (const Poco::Net::NetException& ex) {
        // std::cerr << "Error stopping HTTP server: " << ex.displayText() << std::endl;
        throw; // Rethrow as this might be important to know
    }
}

bool HTTP_Server::IsRunning() const {
    return m_PImpl->isRunning && m_PImpl->pocoServer != nullptr;
}

// HTTP_Error
HTTP_Error::HTTP_Error(int errorCode, const std::string& message)
    : m_ErrorCode(errorCode), m_Message(message), m_Status(HTTP_Status::UNKNOWN) {}
HTTP_Error::HTTP_Error(HTTP_Status status, const std::string& message)
    : m_ErrorCode(static_cast<int>(status)), m_Message(message), m_Status(status) {}
HTTP_Error::~HTTP_Error() = default;

int HTTP_Error::GetErrorCode() const {
    return m_ErrorCode;
}
const std::string& HTTP_Error::GetMessage() const {
    return m_Message;
}
HTTP_Status HTTP_Error::GetStatus() const {
    return m_Status;
}

MCP_NAMESPACE_END