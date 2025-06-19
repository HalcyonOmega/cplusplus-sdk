#include "ITransport.h"

#include <iomanip>
#include <random>
#include <sstream>

// ITransport implementation
std::string ITransport::GenerateRequestID() const {
    uint64_t counter = m_RequestCounter.fetch_add(1);

    // Create a more unique ID by combining timestamp and counter
    auto now = std::chrono::steady_clock::now();
    auto timestamp = now.time_since_epoch().count();

    std::stringstream ss;
    ss << "req_" << std::hex << timestamp << "_" << std::hex << counter;
    return ss.str();
}

bool ITransport::IsValidJSONRPC(const nlohmann::json& InMessage) const {
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
    TransportState oldState = m_CurrentState;
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
            auto stdioOptions = dynamic_cast<StdioTransportOptions*>(InOptions.get());
            if (!stdioOptions) {
                throw std::invalid_argument("Invalid options for stdio transport");
            }
            return CreateStdioTransport(*stdioOptions);
        }
        case TransportType::StreamableHTTP: {
            auto httpOptions = dynamic_cast<HTTPTransportOptions*>(InOptions.get());
            if (!httpOptions) { throw std::invalid_argument("Invalid options for HTTP transport"); }
            return CreateHTTPTransport(*httpOptions);
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

std::optional<nlohmann::json> ParseJSONMessage(const std::string& InRawMessage) {
    try {
        auto json = nlohmann::json::parse(InRawMessage);
        return json;
    } catch (const nlohmann::json::parse_error&) { return std::nullopt; }
}

bool IsRequest(const nlohmann::json& InMessage) {
    return InMessage.contains("method") && InMessage.contains("id");
}

bool IsResponse(const nlohmann::json& InMessage) {
    return InMessage.contains("id") && (InMessage.contains("result") || InMessage.contains("error"))
           && !InMessage.contains("method");
}

bool IsNotification(const nlohmann::json& InMessage) {
    return InMessage.contains("method") && !InMessage.contains("id");
}

std::string ExtractMethod(const nlohmann::json& InMessage) {
    if (InMessage.contains("method") && InMessage["method"].is_string()) {
        return InMessage["method"].get<std::string>();
    }
    return "";
}

std::string ExtractRequestID(const nlohmann::json& InMessage) {
    if (InMessage.contains("id")) {
        if (InMessage["id"].is_string()) {
            return InMessage["id"].get<std::string>();
        } else if (InMessage["id"].is_number()) {
            return std::to_string(InMessage["id"].get<int64_t>());
        }
    }
    return "";
}

nlohmann::json ExtractParams(const nlohmann::json& InMessage) {
    if (InMessage.contains("params")) { return InMessage["params"]; }
    return nlohmann::json::object();
}

nlohmann::json ExtractResult(const nlohmann::json& InMessage) {
    if (InMessage.contains("result")) { return InMessage["result"]; }
    return nlohmann::json::null();
}

nlohmann::json ExtractError(const nlohmann::json& InMessage) {
    if (InMessage.contains("error")) { return InMessage["error"]; }
    return nlohmann::json::null();
}

} // namespace MessageUtils