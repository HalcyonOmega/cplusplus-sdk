#pragma once

#include <memory>
#include <string>

#include "CoreSDK/Common/Macros.h"
#include "Poco/Net/NameValueCollection.h"

MCP_NAMESPACE_BEGIN

HTTP_NAMESPACE_BEGIN

enum class EMethod
{
	Get,
	Post,
	Put,
	Delete,
	Head,
	Options,
	Patch,
	Connect,
	Trace
};

enum class EVersion
{
	V1_0,
	V1_1,
	V2_0
};

enum class EStatus
{
	// Informational 1xx
	Continue = 100,
	SwitchingProtocols = 101,
	Processing = 102,

	// Successful 2xx
	Ok = 200,
	Created = 201,
	Accepted = 202,
	NonAuthoritativeInformation = 203,
	NoContent = 204,
	ResetContent = 205,
	PartialContent = 206,

	// Redirection 3xx
	MultipleChoices = 300,
	MovedPermanently = 301,
	Found = 302,
	SeeOther = 303,
	NotModified = 304,
	TemporaryRedirect = 307,
	PermanentRedirect = 308,

	// Client Error 4xx
	BadRequest = 400,
	Unauthorized = 401,
	Forbidden = 403,
	NotFound = 404,
	MethodNotAllowed = 405,
	NotAcceptable = 406,
	RequestTimeout = 408,
	Conflict = 409,
	Gone = 410,
	LengthRequired = 411,
	PreconditionFailed = 412,
	PayloadTooLarge = 413,
	UriTooLong = 414,
	UnsupportedMediaType = 415,
	TooManyRequests = 429,

	// Server Error 5xx
	InternalServerError = 500,
	NotImplemented = 501,
	BadGateway = 502,
	ServiceUnavailable = 503,
	GatewayTimeout = 504,
	HTTPVersionNotSupported = 505,

	Unknown = 0 // Default or error placeholder
};

/**
 * @class Headers
 * @brief Represents a collection of HTTP headers.
 *
 * This class provides an interface to manage HTTP header key-value pairs
 * for HTTP requests and responses.
 */
class Headers
{
public:
	Headers();
	~Headers();
	Headers(Headers&& other) noexcept;			  // Move constructor
	Headers& operator=(Headers&& other) noexcept; // Move assignment operator

	// Deleted copy operations because of unique_ptr m_PImpl
	Headers(const Headers&) = delete;
	Headers& operator=(const Headers&) = delete;

	void Add(const std::string& name, const std::string& value);
	void Set(const std::string& name, const std::string& value);
	std::string Get(const std::string& name) const;
	bool Has(const std::string& name) const;
	void Remove(const std::string& name);
	void Clear();

private:
	friend class Client; // Allow HTTP_Client to access internal Poco collection

	class HeadersImpl;
	std::unique_ptr<HeadersImpl> m_PImpl;

	// Internal access for HTTP_Client to the underlying Poco collection
	// These are not meant for public use.
	[[nodiscard]] const Poco::Net::NameValueCollection& _GetInternalPocoCollection() const;
	Poco::Net::NameValueCollection& _GetInternalPocoCollection();
};

/**
 * @class Request
 * @brief Represents an HTTP request.
 *
 * Encapsulates method, URI, version, headers, and body for an HTTP request.
 */
class Request
{
public:
	Request();
	~Request();
	Request(Request&& other) noexcept;			  // Move constructor
	Request& operator=(Request&& other) noexcept; // Move assignment operator

	// Deleted copy operations because of HTTP_Headers member which is non-copyable
	Request(const Request&) = delete;
	Request& operator=(const Request&) = delete;

	void SetMethod(EMethod method);
	EMethod GetMethod() const;

	void SetURI(const std::string& uri);
	std::string GetURI() const;

	Headers& GetHeaders();
	const Headers& GetHeaders() const;

	void SetBody(const std::string& body);
	std::string GetBody() const;

	void SetVersion(EVersion version);
	EVersion GetVersion() const;

private:
	EMethod m_Method;
	std::string m_URI;
	EVersion m_Version;
	Headers m_Headers;
	std::string m_Body;
};

/**
 * @class Response
 * @brief Represents an HTTP response.
 *
 * Encapsulates status code, reason phrase, version, headers, and body for an HTTP response.
 */
class Response
{
public:
	Response();
	~Response();
	Response(Response&& other) noexcept;			// Move constructor
	Response& operator=(Response&& other) noexcept; // Move assignment operator

	// Deleted copy operations because of HTTP_Headers member
	Response(const Response&) = delete;
	Response& operator=(const Response&) = delete;

	void SetStatus(EStatus status);
	EStatus GetStatus() const;

	void SetReasonPhrase(const std::string& reason);
	std::string GetReasonPhrase() const;

	Headers& GetHeaders();
	const Headers& GetHeaders() const;

	void SetBody(const std::string& body);
	std::string GetBody() const;

	void SetVersion(EVersion version);
	EVersion GetVersion() const;

private:
	EStatus m_Status;
	std::string m_ReasonPhrase;
	EVersion m_Version;
	Headers m_Headers;
	std::string m_Body;
};

/**
 * @class Client
 * @brief An HTTP client for sending requests and receiving responses.
 */
class Client
{
public:
	Client();
	~Client();
	Client(Client&& other) noexcept;			// Move constructor
	Client& operator=(Client&& other) noexcept; // Move assignment operator

	Client(const Client&) = delete;
	Client& operator=(const Client&) = delete;

	void SetHost(const std::string& host);
	void SetPort(unsigned short port);
	void SetSecure(bool isSecure); // true for HTTPS

	Response SendRequest(Request& request);

private:
	// PImpl is recommended here to hide Poco::Net::HTTPClientSession details.
	class ClientImpl;
	std::unique_ptr<ClientImpl> m_PImpl;
	// Moved host, port, isSecure into PImpl if they are session-specific
	// If they are more like configurations for the client instance, they can stay
	// But typically they'd be part of the Impl that holds the Poco session
	// std::string m_Host;
	// unsigned short m_Port;
	// bool m_IsSecure;
};

/**
 * @class Server
 * @brief An HTTP server for handling incoming requests.
 *
 * Note: A full server abstraction is complex. This provides a basic outline.
 * Actual implementation would involve Poco::Net::HTTPServer and a RequestHandlerFactory.
 */
class Server
{
public:
	Server();
	~Server();
	Server(Server&& other) noexcept;			// Move constructor
	Server& operator=(Server&& other) noexcept; // Move assignment operator

	Server(const Server&) = delete;
	Server& operator=(const Server&) = delete;

	void SetPort(unsigned short port);
	unsigned short GetPort() const;

	void SetRequestHandlerFactory(std::unique_ptr<Poco::Net::HTTPRequestHandlerFactory> factory);

	bool Start();
	void Stop();
	bool IsRunning() const;

private:
	// PImpl is highly recommended here to hide Poco::Net::HTTPServer details.
	class ServerImpl;
	std::unique_ptr<ServerImpl> m_PImpl;
	// unsigned short m_Port; // Likely moved to PImpl
	// bool m_IsRunning;      // Likely moved to PImpl
};

/**
 * @class Error
 * @brief Represents an error related to HTTP operations or Poco library errors.
 */
class Error
{
public:
	Error(int ErrorCode, const std::string& Message);
	Error(EStatus Status, const std::string& Message);
	~Error();

	int GetErrorCode() const; // Could also return HTTP_Status if more appropriate
	const std::string& GetMessage() const;
	EStatus GetStatus() const; // If constructed with HTTP_Status

private:
	// According to memory: private members 'm_'.
	int m_ErrorCode; // Or EStatus m_Status;
	std::string m_Message;
	EStatus m_Status{ EStatus::Unknown }; // Store status if provided
};

HTTP_NAMESPACE_END

using NextFunction = function<void()>;
using RequestHandler = function<future<void>(HTTP::Request&, HTTP::Response&, NextFunction)>;

MCP_NAMESPACE_END