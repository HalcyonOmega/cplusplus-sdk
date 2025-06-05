#pragma once

#include "Core.h"
#include "Message.h"

struct ParamsType;

MCP_NAMESPACE_BEGIN

class NotificationBase : public MessageBase {
  private:
    string Method;

  public:
    NotificationBase(const string& method) : Method(method) {}

    const string& GetMethod() const {
        return Method;
    }
};

template <typename ParamsType> class Notification : public NotificationBase {
  private:
    ParamsType Params;

  public:
    Notification(const std::string& method, const ParamsType& params)
        : NotificationBase(method), Params(params) {}

    const ParamsType& GetParams() const {
        return Params;
    }
};

MCP_NAMESPACE_END
