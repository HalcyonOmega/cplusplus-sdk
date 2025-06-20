#include <iostream>
#include <optional>

#include "../Proxies/JSONProxy.h"

MCP_NAMESPACE_BEGIN

// Basic types for testing
using JSONValue = nlohmann::json;

struct MCPError {
    int Code;
    std::string Message;

    JKEY(CODEKEY, Code, "code")
    JKEY(MESSAGEKEY, Message, "message")

    DEFINE_TYPE_JSON(MCPError, CODEKEY, MESSAGEKEY)
};

// Base message types
struct MessageBase {
    std::string_view JSONRPCVersion = "2.0";

    JKEY(JSONRPCKEY, JSONRPCVersion, "jsonrpc")

    DEFINE_TYPE_JSON(MessageBase, JSONRPCKEY)
};

struct RequestBase : MessageBase {
    std::string ID;
    std::string Method;
    std::optional<JSONValue> Params;

    JKEY(IDKEY, ID, "id")
    JKEY(METHODKEY, Method, "method")
    JKEY(PARAMSKEY, Params, "params")

    DEFINE_TYPE_JSON_DERIVED(RequestBase, MessageBase, IDKEY, METHODKEY, PARAMSKEY)
};

struct ResponseBase : MessageBase {
    std::string ID;
    std::optional<MCPError> Error;
    std::optional<JSONValue> Result;

    JKEY(IDKEY, ID, "id")
    JKEY(RESULTKEY, Result, "result")
    JKEY(ERRORKEY, Error, "error")

    DEFINE_TYPE_JSON(ResponseBase, IDKEY, RESULTKEY, ERRORKEY)
};

struct NotificationBase : MessageBase {
    std::string Method;
    std::optional<JSONValue> Params;

    JKEY(METHODKEY, Method, "method")
    JKEY(PARAMSKEY, Params, "params")

    DEFINE_TYPE_JSON(NotificationBase, METHODKEY, PARAMSKEY)
};

MCP_NAMESPACE_END

int main() {
    std::cout << "=== Testing JKEY System ===" << std::endl;

    // Test 1: MessageBase
    MCP::MessageBase msg;
    nlohmann::json msgJson = msg;
    std::cout << "\n1. MessageBase JSON:" << std::endl;
    std::cout << msgJson.dump(2) << std::endl;

    // Test 2: RequestBase
    MCP::RequestBase request;
    request.ID = std::string("req-123");
    request.Method = "test/method";
    request.Params = nlohmann::json{{"arg1", "value1"}};

    nlohmann::json requestJson = request;
    std::cout << "\n2. RequestBase JSON:" << std::endl;
    std::cout << requestJson.dump(2) << std::endl;

    // Test 3: ResponseBase
    MCP::ResponseBase response;
    response.ID = std::string("req-123");
    response.Result = nlohmann::json{{"success", true}};

    nlohmann::json responseJson = response;
    std::cout << "\n3. ResponseBase JSON:" << std::endl;
    std::cout << responseJson.dump(2) << std::endl;

    // Test 4: NotificationBase
    MCP::NotificationBase notification;
    notification.Method = "notification/event";
    notification.Params = nlohmann::json{{"event", "test"}};

    nlohmann::json notificationJson = notification;
    std::cout << "\n4. NotificationBase JSON:" << std::endl;
    std::cout << notificationJson.dump(2) << std::endl;

    return 0;
}