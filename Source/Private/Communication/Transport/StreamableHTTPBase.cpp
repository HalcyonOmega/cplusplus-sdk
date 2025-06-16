#include "Communication/Transport/StreamableHTTPBase.h"

#include "Communication/Utilities/TransportUtilities.h"
#include "Core.h"
#include "Core/Constants/TransportConstants.h"
#include "Utilities/JSON/JSONLayer.hpp"

MCP_NAMESPACE_BEGIN

StreamableHTTPTransportBase::StreamableHTTPTransportBase(const string& InURL)
    : m_URL(InURL), m_Port(80), m_IsRunning(false) {
    // Parse URL to extract host, port, and path - defer error reporting to ValidateURL
    ValidateURL();
}

bool StreamableHTTPTransportBase::ValidateURL() {
    // Parse URL to get host and port - handle parsing errors with callbacks rather than exceptions
    size_t ProtocolEnd = m_URL.find("://");
    if (ProtocolEnd == string::npos) {
        // Invalid URL format - will be handled by validation in Start() method
        return false;
    }

    string Host = m_URL.substr(ProtocolEnd + 3);
    size_t PathStart = Host.find('/');
    if (PathStart != string::npos) {
        m_Path = Host.substr(PathStart);
        Host = Host.substr(0, PathStart);
    } else {
        m_Path = "/"; // Default path
    }

    size_t PortStart = Host.find(':');
    if (PortStart != string::npos) {
        m_Port = stoi(Host.substr(PortStart + 1));
        Host = Host.substr(0, PortStart);
    } else {
        m_Port = 80; // Default HTTP port
    }

    m_Client = make_unique<HTTP_Client>(Host, m_Port);
    return true;
}

void StreamableHTTPTransportBase::WriteSSEEvent(const string& InEvent, const string& InData) {
    // Default implementation - subclasses can override for specific behavior
    string SSEMessage = FormatSSEEvent(InEvent, InData);

    // This is a basic implementation that could be used by client transports
    // Server transports would override this to write to specific response streams
    // TODO: @HalcyonOmega Create a MessageBase representing the SSE data
    MessageBase Message;
    CallOnMessage(Message);
}

bool StreamableHTTPTransportBase::Resume(const string& /*InResumptionToken*/) {
    // Default implementation - HTTP transport does not support resumption by default
    CallOnError("Resumption not supported by base StreamableHTTPTransport");
    return false;
}

void StreamableHTTPTransportBase::ReadLoop() {
    if (!m_Client) {
        CallOnError("Failed to initialize HTTP client");
        return;
    }

    while (m_IsRunning) {
        // Add session ID to headers if available
        HTTP_Headers Headers;
        if (m_SessionID) { Headers.emplace(TSPT_SESSION_ID, *m_SessionID); }

        auto Response = m_Client->Get(m_Path, Headers, [this](const char* Data, size_t Len) {
            string Chunk(Data, Len);
            ParseSSEData(Chunk);
            return true;
        });

        if (!Response || Response->Status != static_cast<int>(HTTPStatus::Ok)) {
            CallOnError(TRANSPORT_ERR_HTTP_REQUEST_FAILED
                        + (Response ? to_string(Response->Status) : "Unknown error"));
            break;
        }

        // Check for session ID in response headers
        auto SessionHeader = Response->get_header_value(TSPT_SESSION_ID);
        if (!SessionHeader.empty() && !m_SessionID) { m_SessionID = SessionHeader; }
    }

    CallOnClose();
}

MCP_NAMESPACE_END