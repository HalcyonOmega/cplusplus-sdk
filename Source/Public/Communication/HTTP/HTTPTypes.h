#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Communication/Transport/Transport.h"
#include "Core.h"

MCP_NAMESPACE_BEGIN

struct AuthInfo;

class HTTPRequest {
  public:
    HTTPRequest() = default;
    virtual ~HTTPRequest() = default;

    virtual std::string GetMethod() const = 0;
    virtual std::string GetHeader(const std::string& name) const = 0;
    virtual std::vector<uint8_t> GetBody() const = 0;
    virtual const AuthInfo* GetAuthInfo() const = 0;
};

class HTTPResponse {
  public:
    HTTPResponse() = default;
    virtual ~HTTPResponse() = default;

    virtual void SetStatus(int code, const std::string& message) = 0;
    virtual void SetHeader(const std::string& name, const std::string& value) = 0;
    virtual void Write(const std::string& data) = 0;
};

MCP_NAMESPACE_END