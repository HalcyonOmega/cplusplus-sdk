#pragma once

#include "Auth/Types/Auth.h"
#include "Core.h"
#include "Poco/Net/HTTPClientSession.h"
#include "httplib.h"

MCP_NAMESPACE_BEGIN

using HTTP_Client = httplib::Client;
using HTTP_Server = httplib::Server;
using HTTP_Headers = httplib::Headers;
using HTTP_Result = httplib::Result;
using HTTP_Error = httplib::Error;

// HTTP status codes
enum class HTTPStatus {
    Ok = 200,
    BadRequest = 400,
    Unauthorized = 401,
    NotFound = 404,
    MethodNotAllowed = 405,
    NotAcceptable = 406,
    Conflict = 409,
    UnsupportedMediaType = 415,
    InternalServerError = 500
};

class HTTP_Response {
  public:
    HTTPStatus Status;
    HTTP_Headers Headers;
    JSON Body;

    void SetStatus(HTTPStatus Status);
    void SetJSON(const JSON& Data);
    void WriteHead(HTTPStatus InStatus, const optional<HTTP_Headers>& InHeaders = nullopt);
    void Write(const string& InData);
    void End(const optional<string>& InData = nullopt);
    void On(const string& InEvent, optional<function<void()>> InCallback);
    bool IsOK() const;
    future<string> Text() const;
    function<void()> FlushHeaders;
    bool Closed = false;
    bool IsEnded = false;
};

class HTTP_Request {
  public:
    JSON Body;
    shared_ptr<OAuthClientInformationFull> Client = nullptr;
    optional<AuthInfo> Auth;
    string Method;
    HTTP_Headers Headers;
};

using NextFunction = function<void()>;
using RequestHandler = function<future<void>(HTTP_Request&, HTTP_Response&, NextFunction)>;

MCP_NAMESPACE_END