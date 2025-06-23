#include "CoreSDK/Transport/ITransport.h"

#include <sstream>

MCP_NAMESPACE_BEGIN

// ITransport implementation
std::string ITransport::GenerateRequestID() const {
    const uint64_t counter = m_RequestCounter.fetch_add(1);

    // Create a more unique ID by combining timestamp and counter
    const auto now = std::chrono::steady_clock::now();
    const auto timestamp = now.time_since_epoch().count();

    std::stringstream StrStream;
    StrStream << "req_" << std::hex << timestamp << "_" << std::hex << counter;
    return StrStream.str();
}

bool ITransport::IsValidJSONRPC(const JSONValue& InMessage) const {
    if (!InMessage.is_object()) { return false; }

    // Must have jsonrpc field with value "2.0"
    if (!InMessage.contains("jsonrpc") || InMessage["jsonrpc"] != "2.0") { return false; }

    // Must be either request, response, or notification
    bool hasMethod = InMessage.contains("method");
    bool hasId = InMessage.contains("id");
    bool hasResult = InMessage.contains("result");
    bool hasError = InMessage.contains("error");

    // Request: method + id + optional params
    if (hasMethod && hasId && !hasResult && !hasError) { return true; }

    // Response: id + (result XOR error)
    if (!hasMethod && hasId && (hasResult != hasError)) { return true; }

    // Notification: method + no id
    if (hasMethod && !hasId && !hasResult && !hasError) { return true; }

    return false;
}

void ITransport::TriggerStateChange(TransportState InNewState) {
    const TransportState oldState = m_CurrentState;
    m_CurrentState = InNewState;

    if (m_StateChangeHandler && oldState != InNewState) {
        m_StateChangeHandler(oldState, InNewState);
    }
}

// TransportFactory implementation
std::unique_ptr<ITransport>
TransportFactory::CreateTransport(TransportType InType,
                                  std::unique_ptr<TransportOptions> InOptions) {
    switch (InType) {
        case TransportType::Stdio: {
            auto* StdioOptions = dynamic_cast<StdioTransportOptions*>(InOptions.get());
            if (StdioOptions == nullptr) {
                throw std::invalid_argument("Invalid options for stdio transport");
            }
            return CreateStdioTransport(*StdioOptions);
        }
        case TransportType::StreamableHTTP: {
            auto* HTTPOptions = dynamic_cast<HTTPTransportOptions*>(InOptions.get());
            if (HTTPOptions == nullptr) {
                throw std::invalid_argument("Invalid options for HTTP transport");
            }
            return CreateHTTPTransport(*HTTPOptions);
        }
        default: throw std::invalid_argument("Unsupported transport type");
    }
}

std::unique_ptr<ITransport>
TransportFactory::CreateStdioTransport(const StdioTransportOptions& InOptions) {
    // Forward declaration - will be implemented in StdioTransport.cpp
    extern std::unique_ptr<ITransport> CreateStdioTransportImpl(
        const StdioTransportOptions& InOptions);
    return CreateStdioTransportImpl(InOptions);
}

std::unique_ptr<ITransport>
TransportFactory::CreateHTTPTransport(const HTTPTransportOptions& InOptions) {
    // Forward declaration - will be implemented in HTTPTransport.cpp
    extern std::unique_ptr<ITransport> CreateHTTPTransportImpl(
        const HTTPTransportOptions& InOptions);
    return CreateHTTPTransportImpl(InOptions);
}

// MessageUtils implementation
namespace MessageUtils {

std::optional<JSONValue> ParseJSONMessage(const std::string& InRawMessage) {
    try {
        return JSONValue::parse(InRawMessage);
    } catch (const JSONValue::parse_error&) { return std::nullopt; }
}

std::string ExtractMethod(const JSONValue& InMessage) {
    if (InMessage.contains("method") && InMessage["method"].is_string()) {
        return InMessage["method"].get<std::string>();
    }
    return "";
}

std::string ExtractRequestID(const JSONValue& InMessage) {
    if (InMessage.contains("id")) {
        if (InMessage["id"].is_string()) { return InMessage["id"].get<std::string>(); }
        if (InMessage["id"].is_number()) { return std::to_string(InMessage["id"].get<int64_t>()); }
    }
    return "";
}

JSONValue ExtractParams(const JSONValue& InMessage) {
    if (InMessage.contains("params")) { return InMessage["params"]; }
    return JSONValue::object();
}

} // namespace MessageUtils

MCP_NAMESPACE_END