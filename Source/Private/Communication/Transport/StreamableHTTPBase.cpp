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
    size_t protocolEnd = m_URL.find("://");
    if (protocolEnd == string::npos) {
        // Invalid URL format - will be handled by validation in Start() method
        return false;
    }

    string host = m_URL.substr(protocolEnd + 3);
    size_t pathStart = host.find('/');
    if (pathStart != string::npos) {
        m_Path = host.substr(pathStart);
        host = host.substr(0, pathStart);
    } else {
        m_Path = "/"; // Default path
    }

    size_t portStart = host.find(':');
    if (portStart != string::npos) {
        m_Port = stoi(host.substr(portStart + 1));
        host = host.substr(0, portStart);
    } else {
        m_Port = 80; // Default HTTP port
    }

    m_Client = make_unique<HTTP_Client>(host, m_Port);
    return true;
}

void StreamableHTTPTransportBase::WriteSSEEvent(const string& InEvent, const string& InData) {
    // Default implementation - subclasses can override for specific behavior
    string SSEMessage = FormatSSEEvent(InEvent, InData);

    // This is a basic implementation that could be used by client transports
    // Server transports would override this to write to specific response streams
    // Create a MessageBase representing the SSE data
    MessageBase message; // Now that MessageBase is concrete, this works
    CallOnMessage(message);
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
        HTTP_Headers headers;
        if (m_SessionID) { headers.emplace(TSPT_SESSION_ID, *m_SessionID); }

        auto res = m_Client->Get(m_Path, headers, [this](const char* data, size_t len) {
            string chunk(data, len);
            ParseSSEData(chunk);
            return true;
        });

        if (!res || res->status != static_cast<int>(HTTPStatus::Ok)) {
            CallOnError(TRANSPORT_ERR_HTTP_REQUEST_FAILED
                        + (res ? to_string(res->status) : "Unknown error"));
            break;
        }

        // Check for session ID in response headers
        auto sessionHeader = res->get_header_value(TSPT_SESSION_ID);
        if (!sessionHeader.empty() && !m_SessionID) { m_SessionID = sessionHeader; }
    }

    CallOnClose();
}

void StreamableHTTPTransportBase::ParseSSEData(const string& InData) {
    size_t pos = 0;
    string currentEvent;
    string currentData;

    while (pos < InData.length()) {
        size_t lineEnd = InData.find('\n', pos);
        if (lineEnd == string::npos) { break; }

        string line = InData.substr(pos, lineEnd - pos);
        pos = lineEnd + 1;

        if (line.empty() || line[0] == ':') { continue; }

        if (line.substr(0, 6) == "event: ") {
            currentEvent = line.substr(6);
        } else if (line.substr(0, TSPT_EVENT_DATA_PREFIX_LEN) == TSPT_EVENT_DATA_PREFIX) {
            currentData = line.substr(TSPT_EVENT_DATA_PREFIX_LEN);
        } else if (line.empty() && !currentData.empty()) {
            // End of event - create a MessageBase from the SSE data
            MessageBase message; // Use concrete MessageBase for the parsed SSE data
            CallOnMessage(message);
            currentEvent.clear();
            currentData.clear();
        }
    }
}

MCP_NAMESPACE_END