#include "Communication/Transport/ITransport.h"

#include "ErrorBase.h"

MCP_NAMESPACE_BEGIN

void ITransport::CallOnConnect() const {
    lock_guard<mutex> lock(m_CallbackMutex);
    if (OnConnect) { (*OnConnect)(); }
}

void ITransport::CallOnDisconnect() const {
    lock_guard<mutex> lock(m_CallbackMutex);
    if (OnDisconnect) { (*OnDisconnect)(); }
}

void ITransport::CallOnError(const ErrorBase& InError) const {
    lock_guard<mutex> lock(m_CallbackMutex);
    if (OnError) { (*OnError)(InError); }
}

void ITransport::CallOnError(const string& InMessage) const {
    const ErrorBase Error(Errors::InternalError, InMessage);
    CallOnError(Error);
}

void ITransport::CallOnMessage(const MessageBase& InMessage,
                               const optional<AuthInfo>& InAuthInfo) const {
    lock_guard<mutex> lock(m_CallbackMutex);
    if (OnMessage) { (*OnMessage)(InMessage, InAuthInfo); }
}

MCP_NAMESPACE_END