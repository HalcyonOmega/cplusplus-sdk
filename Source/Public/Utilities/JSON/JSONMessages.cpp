#include "JSONMessages.h"

MCP_NAMESPACE_BEGIN

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

std::optional<RequestID> ExtractRequestID(const JSONValue& InMessage) {
    if (InMessage.contains("id")) {
        if (InMessage["id"].is_string()) { return RequestID{InMessage["id"].get<std::string>()}; }
        if (InMessage["id"].is_number()) { return RequestID{InMessage["id"].get<int64_t>()}; }
    }
    return std::nullopt;
}

JSONValue ExtractParams(const JSONValue& InMessage) {
    if (InMessage.contains("params")) { return InMessage["params"]; }
    return JSONValue::object();
}

bool IsValidJSONRPC(const JSONValue& InMessage) {
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

std::optional<MessageType> GetValidMessageType(const JSONValue& InMessage) {
    if (InMessage.contains("id") && InMessage.contains("method")) { return MessageType::Request; }
    if (InMessage.contains("id") && InMessage.contains("result") && !InMessage.contains("error")) {
        return MessageType::Response;
    }
    if (InMessage.contains("id") && InMessage.contains("error")) { return MessageType::Error; }
    if (InMessage.contains("method") && !InMessage.contains("id")) {
        return MessageType::Notification;
    }
    return std::nullopt;
}

MCP_NAMESPACE_END