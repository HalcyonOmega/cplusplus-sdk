#include <iostream>
#include <optional>

#include "../Source/Public/CoreSDK/Messages/MCPMessages.h"

int main() {
    std::cout << "=== Testing JKEY System ===" << std::endl;

    // Test 1: MessageBase
    MCP::MessageBase msg;
    MCP::JSONData msgJson = msg;
    std::cout << "\n1. MessageBase JSON:" << std::endl;
    std::cout << msgJson.dump(2) << std::endl;

    // Test 2: RequestBase
    MCP::RequestBase request;
    request.ID = std::string("req-123");
    request.Method = "test/method";
    request.Params = MCP::JSONData{{"arg1", "value1"}};

    MCP::JSONData requestJson = request;
    std::cout << "\n2. RequestBase JSON:" << std::endl;
    std::cout << requestJson.dump(2) << std::endl;

    // Test 3: ResponseBase
    MCP::ResponseBase response;
    response.ID = std::string("req-123");
    response.Result = MCP::JSONData{{"success", true}};

    MCP::JSONData responseJson = response;
    std::cout << "\n3. ResponseBase JSON:" << std::endl;
    std::cout << responseJson.dump(2) << std::endl;

    // Test 4: NotificationBase
    MCP::NotificationBase notification;
    notification.Method = "notification/event";
    notification.Params = MCP::JSONData{{"event", "test"}};

    MCP::JSONData notificationJson = notification;
    std::cout << "\n4. NotificationBase JSON:" << std::endl;
    std::cout << notificationJson.dump(2) << std::endl;

    return 0;
}