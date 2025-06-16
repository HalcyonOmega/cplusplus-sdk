#include "Communication/Transport/StdioTransport.h"

#include "Communication/Utilities/TransportUtilities.h"
#include "Core.h"
#include "Core/Constants/TransportConstants.h"

MCP_NAMESPACE_BEGIN

StdioTransport::StdioTransport() : m_IsRunning(false) {}

StdioTransport::~StdioTransport() {
    Close();
}

future<void> StdioTransport::Start() {
    if (m_IsRunning) {
        return async(launch::async, []() {});
    }

    m_IsRunning = true;
    CallOnStart();

    // Start reading thread
    m_ReadThread = std::thread([this] { ReadLoop(); });
}

future<void> StdioTransport::Close() {
    if (!m_IsRunning) {
        return async(launch::async, []() {});
    }

    m_IsRunning = false;
    if (m_ReadThread.joinable()) { m_ReadThread.join(); }

    CallOnClose();
}

future<void> StdioTransport::Send(const MessageBase& InMessage,
                                  const TransportSendOptions& InOptions) {
    if (!m_IsRunning) {
        CallOnError(TRANSPORT_ERR_NOT_RUNNING);
        return async(launch::async, []() {});
    }

    try {
        // Validate UTF-8 encoding
        if (!TransportUtilities::IsValidUTF8(InMessage)) {
            CallOnError(TRANSPORT_ERR_INVALID_UTF8);
            return async(launch::async, []() {});
        }

        // Validate JSON-RPC message format
        if (!TransportUtilities::IsValidJSONRPC(InMessage)) {
            CallOnError(TRANSPORT_ERR_INVALID_JSON_RPC);
            return async(launch::async, []() {});
        }

        // Handle resumption token if provided
        if (InOptions.ResumptionToken && InOptions.OnResumptionToken) {
            InOptions.OnResumptionToken.value()(*InOptions.ResumptionToken);
        }

        std::cout << InMessage << std::endl;
        if (std::cout.fail()) { CallOnError("Failed to write to stdout"); }
    } catch (const std::exception& Exception) {
        CallOnError(std::string("Error sending message: ") + Exception.what());
    }
}

void StdioTransport::WriteSSEEvent(const string& InEvent, const string& InData) {
    string SSEMessage = "event: " + InEvent + TSPT_EVENT_DELIMITER + TSPT_EVENT_DATA_PREFIX + InData
                        + TSPT_EVENT_DELIMITER;
    Send(SSEMessage);
}

// New method for resumability support
bool StdioTransport::Resume(const string& InResumptionToken) {
    // Stdio transport does not support resumption
    CallOnError("Resumption not supported by StdioTransport");
    return false;
}

void StdioTransport::ReadLoop() {
    std::string Line;
    while (m_IsRunning && std::getline(std::cin, Line)) {
        try {
            // Validate UTF-8 encoding
            if (!TransportUtilities::IsValidUTF8(Line)) {
                CallOnError(TRANSPORT_ERR_INVALID_UTF8);
                continue;
            }

            // Validate JSON-RPC message format
            if (!TransportUtilities::IsValidJSONRPC(Line)) {
                CallOnError(TRANSPORT_ERR_INVALID_JSON_RPC);
                continue;
            }

            CallOnMessage(Line);
        } catch (const std::exception& Exception) {
            CallOnError(std::string("Error processing message: ") + Exception.what());
        }
    }

    // Handle stdin close
    CallOnClose();
}

MCP_NAMESPACE_END