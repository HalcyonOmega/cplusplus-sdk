#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/StreamCopier.h>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "CoreSDK/Common/Logging.h"
#include "CoreSDK/Messages/ErrorResponseBase.h"
#include "CoreSDK/Messages/MCPMessages.h"
#include "CoreSDK/Messages/NotificationBase.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "JSONProxy.h"
#include "Utilities/JSON/JSONMessages.h"

// Simple MCP HTTP Request Handler
class MCPRequestHandler : public Poco::Net::HTTPRequestHandler {
  public:
    void handleRequest(Poco::Net::HTTPServerRequest& request,
                       Poco::Net::HTTPServerResponse& response) override {
        try {
            // Read request body
            std::string requestBody;
            if (request.getContentLength() > 0) {
                Poco::StreamCopier::copyToString(request.stream(), requestBody);
            }

            std::cout << "\n[SERVER] Received: " << requestBody << std::endl;

            // Process request and generate response
            std::string responseBody = ProcessMCPRequest(requestBody);

            if (!responseBody.empty()) {
                std::cout << "[SERVER] Sending: " << responseBody << std::endl;

                // Send response
                response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                response.setContentType("application/json");
                response.setContentLength(responseBody.length());

                std::ostream& responseStream = response.send();
                responseStream << responseBody;
            } else {
                // No response for notifications
                std::cout << "[SERVER] No response (notification received)" << std::endl;
                response.setStatus(Poco::Net::HTTPResponse::HTTP_ACCEPTED);
                response.setContentLength(0);
                response.send();
            }

        } catch (const std::exception& e) {
            std::cout << "[SERVER] Error: " << e.what() << std::endl;
            response.setStatus(Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
            response.setContentType("application/json");

            MCP::ErrorResponseBase errorResponse(
                MCP::RequestID(std::string("unknown")),
                MCP::MCPError(MCP::ErrorCodes::INTERNAL_ERROR, "Internal error"));
            std::string errorJSON = MCP::JSONData(errorResponse).dump();

            response.setContentLength(errorJSON.length());
            std::ostream& responseStream = response.send();
            responseStream << errorJSON;
        }
    }

  private:
    std::string ProcessMCPRequest(const std::string& requestBody) {
        // Validate JSON-RPC
        auto jsonData = MCP::ParseJSONMessage(requestBody);
        if (!jsonData) {
            MCP::ErrorResponseBase error(
                MCP::RequestID(std::string("unknown")),
                MCP::MCPError(MCP::ErrorCodes::PARSE_ERROR, "Parse error"));
            return MCP::JSONData(error).dump();
        }

        auto messageType = MCP::GetValidMessageType(*jsonData);
        if (!messageType) {
            MCP::ErrorResponseBase error(
                MCP::RequestID(std::string("unknown")),
                MCP::MCPError(MCP::ErrorCodes::INVALID_REQUEST, "Invalid Request"));
            return MCP::JSONData(error).dump();
        }

        // Handle notification (no ID field means it's a notification)
        if (messageType == MCP::MessageType::Notification) {
            auto notification = jsonData->get<MCP::NotificationBase>();
            std::cout << "[SERVER] Notification received for method: " << notification.Method
                      << std::endl;
            return ""; // No response for notifications
        }

        auto request = jsonData->get<MCP::RequestBase>();

        // Handle different methods
        if (request.Method == "ping") {
            MCP::JSONData result;
            result["pong"] = true;
            result["timestamp"] = std::time(nullptr);
            MCP::ResponseBase response(request.ID, {result});
            return MCP::JSONData(response).dump();
        } else if (request.Method == "echo") {
            MCP::ResponseBase response(request.ID, {request.ParamsData});
            return MCP::JSONData(response).dump();
        } else if (request.Method == "test/simple") {
            MCP::JSONData result;
            result["message"] = "Hello from MCP server!";
            MCP::ResponseBase response(request.ID, {result});
            return MCP::JSONData(response).dump();
        } else {
            MCP::ErrorResponseBase error(
                request.ID, MCP::MCPError(MCP::ErrorCodes::METHOD_NOT_FOUND, "Method not found"));
            return MCP::JSONData(error).dump();
        }
    }
};

// Request Handler Factory
class MCPRequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory {
  public:
    Poco::Net::HTTPRequestHandler*
    createRequestHandler(const Poco::Net::HTTPServerRequest& request) override {
        (void)request;
        return new MCPRequestHandler();
    }
};

void RunInteractiveHTTPTest() {
    std::cout << "\n=== HTTP Transport Test ===" << std::endl;
    std::cout << "Starting HTTP server..." << std::endl;

    const int port = 9876;

    try {
        // Start server
        Poco::Net::ServerSocket serverSocket(port);
        auto factory = new MCPRequestHandlerFactory();
        Poco::Net::HTTPServer server(factory, serverSocket, new Poco::Net::HTTPServerParams);
        server.start();

        std::cout << "✓ HTTP server started on port " << port << std::endl;
        std::cout << "You can now send requests as a client to localhost:" << port << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        int choice;
        while (true) {
            std::cout << "\nHTTP Client Options:" << std::endl;
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

            // Send HTTP request to our local server
            try {
                Poco::Net::HTTPClientSession session("localhost", port);
                session.setTimeout(Poco::Timespan(5, 0));

                Poco::Net::HTTPRequest httpRequest(Poco::Net::HTTPRequest::HTTP_POST, "/");
                httpRequest.setContentType("application/json");
                httpRequest.setContentLength(message.length());

                std::cout << "\n[CLIENT] Sending: " << message << std::endl;

                std::ostream& requestStream = session.sendRequest(httpRequest);
                requestStream << message;

                Poco::Net::HTTPResponse httpResponse;
                std::istream& responseStream = session.receiveResponse(httpResponse);

                std::string responseBody;
                Poco::StreamCopier::copyToString(responseStream, responseBody);

                std::cout << "[CLIENT] HTTP Status: " << httpResponse.getStatus() << std::endl;

                if (!responseBody.empty()) {
                    std::cout << "[CLIENT] Response: " << responseBody << std::endl;

                    // Validate response
                    auto jsonResponse = MCP::ParseJSONMessage(responseBody);
                    if (jsonResponse) {
                        std::cout << "✓ Valid JSON-RPC response" << std::endl;

                        // Check if it's an error or success
                        if (jsonResponse->contains("error")) {
                            std::cout << "⚠ Server returned error" << std::endl;
                        } else if (jsonResponse->contains("result")) {
                            std::cout << "✓ Success response" << std::endl;
                        }
                    } else {
                        std::cout << "✗ Invalid JSON-RPC response" << std::endl;
                    }
                } else {
                    std::cout << "[CLIENT] No response body (normal for notifications)"
                              << std::endl;
                }

            } catch (const std::exception& e) {
                std::cout << "HTTP request failed: " << e.what() << std::endl;
            }
        }

        // Stop server
        std::cout << "\nStopping HTTP server..." << std::endl;
        server.stop();
        std::cout << "✓ HTTP server stopped" << std::endl;

    } catch (const std::exception& e) {
        std::cout << "HTTP test failed: " << e.what() << std::endl;
    }
}