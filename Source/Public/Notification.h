#pragma once

#include "Core.h"
#include "Message.h"

struct ParamsType;

MCP_NAMESPACE_BEGIN

class NotificationBase : public MessageBase {
  private:
    string m_Method;

  public:
    NotificationBase(const string& InMethod) : m_Method(InMethod) {}

    const string& GetMethod() const {
        return m_Method;
    }
};

template <typename ParamsType> class Notification : public NotificationBase {
  private:
    ParamsType m_Params;

  public:
    Notification(const std::string& InMethod, const ParamsType& InParams)
        : NotificationBase(InMethod), m_Params(InParams) {}

    const ParamsType& GetParams() const {
        return m_Params;
    }
};

MCP_NAMESPACE_END
