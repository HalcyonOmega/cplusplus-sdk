#pragma once

#include "Auth/Types/Auth.h"
#include "Core.h"

MCP_NAMESPACE_BEGIN

/* Needed Classes/Structs */
// TODO: @HalcyonOmega Implement HTTP_Client and HTTP_Server classes.
class HTTP_Client;
class HTTP_Server;

class HTTP_Response {
  private:
    int StatusCode;
    unordered_map<string, string> Headers;
    JSON Body;

  public:
    void SetStatus(int Status);
    void SetJSON(const JSON& Data);
    void WriteHead(int InStatusCode,
                   const optional<unordered_map<string, string>>& InHeaders = nullopt);
    void Write(const string& InData);
    void End(const optional<string>& InData = nullopt);
    void On(const string& InEvent, optional<function<void()>> InCallback);
    bool IsOK() const;
    future<string> text() const;
    function<void()> flushHeaders;
    bool closed = false;
    bool IsEnded = false;
};

class HTTP_Request {
  private:
    JSON Body;
    shared_ptr<OAuthClientInformationFull> Client = nullptr;
    optional<AuthInfo> Auth;
    string Method;
    unordered_map<string, string> Headers;
};

using NextFunction = function<void()>;
using RequestHandler = function<future<void>(HTTP_Request&, HTTP_Response&, NextFunction)>;

MCP_NAMESPACE_END