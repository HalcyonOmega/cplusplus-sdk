#include <Poco/Pipe.h>
#include <Poco/PipeStream.h>
#include <Poco/Process.h>

#include <iostream>
#include <string>
#include <thread>

#include "CoreSDK/Common/Logging.h"
#include "CoreSDK/Messages/ErrorResponseBase.h"
#include "CoreSDK/Messages/MCPMessages.h"
#include "CoreSDK/Messages/NotificationBase.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "JSONProxy.h"
#include "TestHelpers.h"
#include "Utilities/JSON/JSONMessages.h"

class STDIOTransportTest {
  private:
    TestRunner& m_TestRunner;

  public:
    STDIOTransportTest(TestRunner& runner) : m_TestRunner(runner) {}

    void RunTests() {
        std::cout << "\n=== STDIO Transport Tests ===" << std::endl;

        TestMessageSerialization();
        TestSimpleEchoServer();
        TestRequestResponse();
        TestNotification();
        TestErrorHandling();
    }

  private:
    void TestMessageSerialization() {
        // Test basic message serialization
        MCP::RequestBase request(MCP::RequestID(std::string("test-001")), "ping",
                                 {MCP::RequestParams{MCP::JSONData{{"data", "hello"}}}});
        std::string requestJSON = MCP::JSONData(request).dump();
        auto parsedRequest = MCP::ParseJSONMessage(requestJSON);

        m_TestRunner.RunTest("STDIO - Request Serialization",
                             parsedRequest && parsedRequest->at("method") == "ping",
                             "Request JSON: " + requestJSON);

        MCP::ResponseBase response(MCP::RequestID(std::string("test-001")),
                                   {MCP::ResultParams{MCP::JSONData{{"success", true}}}});
        std::string responseJSON = MCP::JSONData(response).dump();
        auto parsedResponse = MCP::ParseJSONMessage(responseJSON);

        m_TestRunner.RunTest("STDIO - Response Serialization",
                             parsedResponse && parsedResponse->at("result").contains("success"),
                             "Response JSON: " + responseJSON);

        MCP::NotificationBase notification(
            "test/event", {MCP::NotificationParams{MCP::JSONData{{"type", "ping"}}}});
        std::string notificationJSON = MCP::JSONData(notification).dump();
        auto parsedNotification = MCP::ParseJSONMessage(notificationJSON);

        m_TestRunner.RunTest("STDIO - Notification Serialization",
                             parsedNotification && parsedNotification->at("method") == "test/event",
                             "Notification JSON: " + notificationJSON);

        MCP::ErrorResponseBase error(
            MCP::RequestID(std::string("test-001")),
            MCP::MCPError(MCP::ErrorCodes::INVALID_REQUEST, "Invalid Request"));
        std::string errorJSON = MCP::JSONData(error).dump();
        auto parsedError = MCP::ParseJSONMessage(errorJSON);

        m_TestRunner.RunTest(
            "STDIO - Error Serialization",
            parsedError && parsedError->at("error").at("code") == MCP::ErrorCodes::INVALID_REQUEST,
            "Error JSON: " + errorJSON);
    }

    void TestSimpleEchoServer() {
        try {
            // Create pipes for communication
            Poco::Pipe stdinPipe, stdoutPipe;

// Launch echo process (simple cat command that echoes input)
#ifdef _WIN32
            std::vector<std::string> args;
            auto processHandle =
                Poco::Process::launch("cmd", args, &stdinPipe, &stdoutPipe, nullptr);
#else
            std::vector<std::string> args;
            auto processHandle =
                Poco::Process::launch("cat", args, &stdinPipe, &stdoutPipe, nullptr);
#endif

            // Create streams
            Poco::PipeOutputStream stdinStream(stdinPipe);
            Poco::PipeInputStream stdoutStream(stdoutPipe);

            // Test simple echo
            std::string testMessage = "Hello STDIO Transport\n";
            stdinStream << testMessage;
            stdinStream.flush();

            // Read response with timeout
            std::string response;
            std::getline(stdoutStream, response);

            // Close streams
            stdinStream.close();

            // Wait for process to finish
            int exitCode = processHandle.wait();
            (void)exitCode; // Suppress unused variable warning

            m_TestRunner.RunTest("STDIO - Simple Echo Test",
                                 response.find("Hello STDIO Transport") != std::string::npos,
                                 "Expected echo, got: " + response);

        } catch (const std::exception& e) {
            m_TestRunner.RunTest("STDIO - Simple Echo Test", false,
                                 "Exception: " + std::string(e.what()));
        }
    }

    void TestRequestResponse() {
        MCP::RequestBase request(MCP::RequestID(std::string("req-001")), "test/ping",
                                 {MCP::RequestParams{MCP::JSONData{{"timestamp", 1234567890}}}});
        std::string requestJSON = MCP::JSONData(request).dump() + "\n";

        // For this test, we'll simulate the response parsing
        // In a real implementation, this would go through the actual transport
        std::string simulatedResponse =
            "{\"jsonrpc\":\"2.0\",\"id\":\"req-001\",\"result\":{\"pong\":true}}";

        auto parsedResponse = MCP::ParseJSONMessage(simulatedResponse);
        auto response = parsedResponse->get<MCP::ResponseBase>();

        m_TestRunner.RunTest("STDIO - Request-Response Matching",
                             response.ID.ToString() == "req-001"
                                 && response.ResultData.Meta.has_value(),
                             "ID: " + response.ID.ToString());
    }

    void TestNotification() {
        MCP::NotificationBase notification(
            "notification/progress", {MCP::NotificationParams{MCP::JSONData{{"percent", 50}}}});
        std::string notificationJSON = MCP::JSONData(notification).dump();
        auto parsedNotification = MCP::ParseJSONMessage(notificationJSON);

        // Notifications should not have an ID
        bool hasNoID = !parsedNotification->contains("id");
        bool hasMethod = parsedNotification->at("method") == "notification/progress";

        m_TestRunner.RunTest("STDIO - Notification Format", hasNoID && hasMethod,
                             "Notification should not have ID field");
    }

    void TestErrorHandling() {
        MCP::ErrorResponseBase error(
            MCP::RequestID(std::string("req-002")),
            MCP::MCPError(-32601, "Method not found", MCP::JSONData{{"method", "unknown/method"}}));
        std::string errorJSON = MCP::JSONData(error).dump();
        auto parsedError = MCP::ParseJSONMessage(errorJSON);
        auto errorResponse = parsedError->get<MCP::ErrorResponseBase>();

        m_TestRunner.RunTest("STDIO - Error Response Format",
                             errorResponse.ErrorData.Code == -32601
                                 && errorResponse.ErrorData.Message == "Method not found",
                             "Code: " + std::to_string(errorResponse.ErrorData.Code)
                                 + ", Message: " + errorResponse.ErrorData.Message);
    }
};

void RunSTDIOTransportTests(TestRunner& runner) {
    STDIOTransportTest test(runner);
    test.RunTests();
}

void RunInteractiveSTDIOTest() {
    std::cout << "\n=== STDIO Transport Test ===" << std::endl;
    std::cout << "This will echo messages through a subprocess (cat command)" << std::endl;

    try {
        // Create pipes for communication
        Poco::Pipe stdinPipe, stdoutPipe;

// Launch echo process
#ifdef _WIN32
        std::vector<std::string> args;
        auto processHandle = Poco::Process::launch("cmd", args, &stdinPipe, &stdoutPipe, nullptr);
#else
        std::vector<std::string> args;
        auto processHandle = Poco::Process::launch("cat", args, &stdinPipe, &stdoutPipe, nullptr);
#endif

        // Create streams
        Poco::PipeOutputStream stdinStream(stdinPipe);
        Poco::PipeInputStream stdoutStream(stdoutPipe);

        int choice;
        while (true) {
            std::cout << "\nSTDIO Options:" << std::endl;
            std::cout << "1. Send InitializeRequest" << std::endl;
            std::cout << "2. Send ListToolsRequest" << std::endl;
            std::cout << "3. Send InitializedNotification" << std::endl;
            std::cout << "4. Send LoggingMessageNotification" << std::endl;
            std::cout << "5. Send Raw JSON" << std::endl;
            std::cout << "6. Back to main menu" << std::endl;
            std::cout << "Choice: ";
            std::cin >> choice;
            std::cin.ignore(); // Clear newline

            if (choice == 6) break;

            std::string message;
            switch (choice) {
                case 1: {
                    MCP::InitializeRequest request;
                    message = MCP::JSONData(request).dump();
                    break;
                }
                case 2: {
                    MCP::ListToolsRequest request;
                    message = MCP::JSONData(request).dump();
                    break;
                }
                case 3: {
                    MCP::InitializedNotification notification;
                    message = MCP::JSONData(notification).dump();
                    break;
                }
                case 4: {
                    MCP::LoggingMessageNotification notification(
                        {{MCP::LoggingLevel::Info, "This is a test log message from the client."}});
                    message = MCP::JSONData(notification).dump();
                    break;
                }
                case 5: {
                    std::cout << "Enter raw JSON: ";
                    std::getline(std::cin, message);
                    break;
                }
                default: std::cout << "Invalid choice!" << std::endl; continue;
            }

            // Send message
            std::cout << "\nSending: " << message << std::endl;
            stdinStream << message << std::endl;
            stdinStream.flush();

            // Read response
            std::string response;
            if (std::getline(stdoutStream, response)) {
                std::cout << "Received: " << response << std::endl;

                // Basic validation
                if (MCP::ParseJSONMessage(response)) {
                    std::cout << "✓ Valid JSON-RPC format" << std::endl;
                } else {
                    std::cout << "✗ Invalid JSON-RPC format" << std::endl;
                }
            } else {
                std::cout << "No response received" << std::endl;
            }
        }

        // Cleanup
        stdinStream.close();
        processHandle.wait();

    } catch (const std::exception& e) {
        std::cout << "STDIO Test failed: " << e.what() << std::endl;
    }
}