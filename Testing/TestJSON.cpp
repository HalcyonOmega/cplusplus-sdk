#include <iostream>
#include <optional>

#include "../Source/Public/CoreSDK/MCPMessages.h"

int main() {
    std::cout << "=== Testing JKEY System ===" << std::endl;

    // Test 1: MessageBase
    MCP::MessageBase msg;
    MCP::JSONValue msgJson = msg;
    std::cout << "\n1. MessageBase JSON:" << std::endl;
    std::cout << msgJson.dump(2) << std::endl;

    // Test 2: RequestBase
    MCP::RequestBase request;
    request.ID = std::string("req-123");
    request.Method = "test/method";
    request.Params = MCP::JSONValue{{"arg1", "value1"}};

    MCP::JSONValue requestJson = request;
    std::cout << "\n2. RequestBase JSON:" << std::endl;
    std::cout << requestJson.dump(2) << std::endl;

    // Test 3: ResponseBase
    MCP::ResponseBase response;
    response.ID = std::string("req-123");
    response.Result = MCP::JSONValue{{"success", true}};

    MCP::JSONValue responseJson = response;
    std::cout << "\n3. ResponseBase JSON:" << std::endl;
    std::cout << responseJson.dump(2) << std::endl;

    // Test 4: NotificationBase
    MCP::NotificationBase notification;
    notification.Method = "notification/event";
    notification.Params = MCP::JSONValue{{"event", "test"}};

    MCP::JSONValue notificationJson = notification;
    std::cout << "\n4. NotificationBase JSON:" << std::endl;
    std::cout << notificationJson.dump(2) << std::endl;

    return 0;
}