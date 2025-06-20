#pragma once

#include <iostream>

#include "SimpleMCPAPI.h"

// Example 1: Simple MCP Server with Tools
MCP::MCPTask_Void ExampleServer() {
    std::cout << "Starting MCP Server Example..." << std::endl;

    // Create a simple MCP server using stdio transport
    MCP::SimpleMCPServer server;

    // Add a simple calculator tool
    server.AddTool("calculator", "Performs basic mathematical operations",
                   CreateToolSchema("object",
                                    {{"operation", CreateStringProperty(
                                                       "The operation to perform",
                                                       std::regex("add|subtract|multiply|divide"))},
                                     {"a", CreateNumberProperty("First number")},
                                     {"b", CreateNumberProperty("Second number")}},
                                    {"operation", "a", "b"}),
                   [](const nlohmann::json& InArgs) -> ToolCallResponse {
                       std::string operation = InArgs["operation"];
                       double a = InArgs["a"];
                       double b = InArgs["b"];

                       double result = 0;
                       if (operation == "add") {
                           result = a + b;
                       } else if (operation == "subtract") {
                           result = a - b;
                       } else if (operation == "multiply") {
                           result = a * b;
                       } else if (operation == "divide") {
                           if (b == 0) { return CreateTextResult("Error: Division by zero", true); }
                           result = a / b;
                       } else {
                           return CreateTextResult("Error: Unknown operation", true);
                       }

                       return CreateTextResult("Result: " + std::to_string(result), false);
                   });

    // Add a file system tool
    server.AddTool("read_file", "Reads the contents of a file",
                   CreateToolSchema("object",
                                    {{"path", CreateStringProperty("Path to the file to read")}},
                                    {"path"}),
                   [](const nlohmann::json& InArgs) -> ToolCallResponse {
                       std::string path = InArgs["path"];

                       std::ifstream file(path);
                       if (!file.is_open()) {
                           return CreateTextResult("Error: Could not open file " + path, true);
                       }

                       std::string content((std::istreambuf_iterator<char>(file)),
                                           std::istreambuf_iterator<char>());

                       return CreateTextResult(content, false);
                   });

    // Add a prompt for code generation
    server.AddPrompt(
        "generate_code", "Generates code based on requirements",
        CreateToolSchema("object",
                         {{"language", CreateStringProperty("Programming language")},
                          {"requirements", CreateStringProperty("Code requirements")}},
                         {"language", "requirements"}),
        [](const std::optional<nlohmann::json>& InArgs) -> PromptGetResponse {
            std::string language = "C++";
            std::string requirements = "Hello World program";

            if (InArgs.has_value()) {
                if (InArgs->contains("language")) { language = (*InArgs)["language"]; }
                if (InArgs->contains("requirements")) { requirements = (*InArgs)["requirements"]; }
            }

            std::vector<PromptMessage> messages = {
                CreateUserMessage("Please generate " + language + " code that: " + requirements),
                CreateAssistantMessage("I'll help you generate the requested code. Please provide "
                                       "more specific details about what you need.")};

            return CreatePromptResponse("Code generation prompt for " + language, messages);
        });

    // Add a resource for documentation
    server.AddResource("file:///docs/api.md", "API Documentation", "MCP API documentation",
                       "text/markdown", []() -> ResourceReadResponse {
                           std::string markdown = R"(
# MCP API Documentation

## Overview
This is the Model Context Protocol API documentation.

## Endpoints
- `/tools/list` - List available tools
- `/tools/call` - Call a tool
- `/prompts/list` - List available prompts
- `/prompts/get` - Get a prompt
- `/resources/list` - List available resources
- `/resources/read` - Read a resource

## Example Usage
```cpp
MCPClient client(TransportType::Stdio, options);
auto tools = client.ListTools();
```
)";
                           return CreateTextResourceResponse(markdown, "text/markdown");
                       });

    // Start the server
    co_await server.Start("ExampleServer", "1.0.0");

    std::cout << "Server started. Waiting for connections..." << std::endl;

    // Keep server running (in real application, this would be an event loop)
    // For this example, we'll just wait a bit
    std::this_thread::sleep_for(std::chrono::seconds(10));

    co_await server.Stop();
    std::cout << "Server stopped." << std::endl;
}

// Example 2: MCP Client connecting to a server
MCPTask_Void ExampleClient() {
    std::cout << "Starting MCP Client Example..." << std::endl;

    // Create a client connecting to a stdio process
    SimpleMCPClient client("python", {"-m", "mcp_server_example"});

    try {
        // Connect to the server
        co_await client.Connect("ExampleClient", "1.0.0");
        std::cout << "Connected to server!" << std::endl;

        // List available tools
        auto tools = co_await client.ListTools();
        std::cout << "Available tools:" << std::endl;
        for (const auto& tool : tools) {
            std::cout << "  - " << tool.Name << ": " << tool.Description << std::endl;
        }

        // Call a tool
        nlohmann::json calcArgs = {{"operation", "add"}, {"a", 5}, {"b", 3}};

        auto result = co_await client.CallTool("calculator", calcArgs);
        std::cout << "Calculator result: ";
        for (const auto& content : result.Content) {
            if (std::holds_alternative<TextContent>(content)) {
                std::cout << std::get<TextContent>(content).Text << std::endl;
            }
        }

        // List prompts
        auto prompts = co_await client.ListPrompts();
        std::cout << "Available prompts:" << std::endl;
        for (const auto& prompt : prompts) {
            std::cout << "  - " << prompt.Name << ": " << prompt.Description << std::endl;
        }

        // Get a prompt
        nlohmann::json promptArgs = {{"language", "Python"},
                                     {"requirements", "A function to calculate factorial"}};

        auto promptResult = co_await client.GetPrompt("generate_code", promptArgs);
        std::cout << "Prompt: " << promptResult.Description << std::endl;

        // List resources
        auto resources = co_await client.ListResources();
        std::cout << "Available resources:" << std::endl;
        for (const auto& resource : resources) {
            std::cout << "  - " << resource.Name << " (" << resource.URI << ")" << std::endl;
        }

        // Read a resource
        auto resourceContent = co_await client.ReadResource("file:///docs/api.md");
        std::cout << "Resource content preview:" << std::endl;
        for (const auto& content : resourceContent.Contents) {
            if (std::holds_alternative<ResourceContent>(content)) {
                auto resContent = std::get<ResourceContent>(content);
                if (resContent.Text.has_value()) {
                    std::string text = resContent.Text.value();
                    std::cout << text.substr(0, 200) << "..." << std::endl;
                }
            }
        }

        // Subscribe to resource updates
        co_await client.SubscribeToResource("file:///docs/api.md");
        std::cout << "Subscribed to resource updates" << std::endl;

        // Set up event handlers
        client.OnResourceUpdated(
            [](const std::string& uri) { std::cout << "Resource updated: " << uri << std::endl; });

        client.OnProgress([](const std::string& token, double progress, double total) {
            std::cout << "Progress [" << token << "]: " << progress << "/" << total << std::endl;
        });

        client.OnLog([](LoggingLevel level, const std::string& message) {
            std::cout << "Log: " << message << std::endl;
        });

        // Wait a bit for any notifications
        std::this_thread::sleep_for(std::chrono::seconds(5));

        // Disconnect
        co_await client.Disconnect();
        std::cout << "Disconnected from server." << std::endl;

    } catch (const std::exception& e) { std::cout << "Error: " << e.what() << std::endl; }
}

// Example 3: HTTP Transport Server
MCPTask_Void ExampleHTTPServer() {
    std::cout << "Starting HTTP MCP Server Example..." << std::endl;

    // Create HTTP server on port 8080
    SimpleMCPServer server(8080, "/mcp");

    // Add a weather tool
    server.AddTool(
        "get_weather", "Gets current weather for a location",
        CreateToolSchema("object",
                         {{"location", CreateStringProperty("Location to get weather for")},
                          {"units", CreateStringProperty("Temperature units",
                                                         std::regex("celsius|fahrenheit"))}},
                         {"location"}),
        [](const nlohmann::json& InArgs) -> ToolCallResponse {
            std::string location = InArgs["location"];
            std::string units = InArgs.value("units", "celsius");

            // Simulate weather API call
            nlohmann::json weather = {{"location", location},
                                      {"temperature", units == "celsius" ? 22 : 72},
                                      {"units", units},
                                      {"condition", "sunny"},
                                      {"humidity", 65}};

            return CreateTextResult(weather.dump(2), false);
        });

    // Add a time tool
    server.AddTool("get_time", "Gets current time", CreateToolSchema("object", {}, {}),
                   [](const nlohmann::json& InArgs) -> ToolCallResponse {
                       auto now = std::chrono::system_clock::now();
                       auto time_t = std::chrono::system_clock::to_time_t(now);
                       auto tm = *std::localtime(&time_t);

                       char buffer[100];
                       std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);

                       return CreateTextResult(std::string(buffer), false);
                   });

    // Start server
    co_await server.Start("HTTPExampleServer", "1.0.0");

    std::cout << "HTTP Server started on port 8080" << std::endl;
    std::cout << "Access at: http://localhost:8080/mcp" << std::endl;
    std::cout << "SSE endpoint: http://localhost:8080/mcp/events" << std::endl;

    // Keep server running
    std::this_thread::sleep_for(std::chrono::seconds(30));

    co_await server.Stop();
    std::cout << "HTTP Server stopped." << std::endl;
}

// Example 4: HTTP Transport Client
MCPTask_Void ExampleHTTPClient() {
    std::cout << "Starting HTTP MCP Client Example..." << std::endl;

    // Create HTTP client connecting to localhost:8080
    SimpleMCPClient client("localhost", 8080, "/mcp");

    try {
        co_await client.Connect("HTTPExampleClient", "1.0.0");
        std::cout << "Connected to HTTP server!" << std::endl;

        // Test tools
        auto tools = co_await client.ListTools();
        std::cout << "Available tools via HTTP:" << std::endl;
        for (const auto& tool : tools) { std::cout << "  - " << tool.Name << std::endl; }

        // Call weather tool
        nlohmann::json weatherArgs = {{"location", "San Francisco"}, {"units", "celsius"}};

        auto weatherResult = co_await client.CallTool("get_weather", weatherArgs);
        std::cout << "Weather result: ";
        for (const auto& content : weatherResult.Content) {
            if (std::holds_alternative<TextContent>(content)) {
                std::cout << std::get<TextContent>(content).Text << std::endl;
            }
        }

        // Call time tool
        auto timeResult = co_await client.CallTool("get_time", nlohmann::json::object());
        std::cout << "Time result: ";
        for (const auto& content : timeResult.Content) {
            if (std::holds_alternative<TextContent>(content)) {
                std::cout << std::get<TextContent>(content).Text << std::endl;
            }
        }

        co_await client.Disconnect();
        std::cout << "Disconnected from HTTP server." << std::endl;

    } catch (const std::exception& e) {
        std::cout << "HTTP Client Error: " << e.what() << std::endl;
    }
}

// Example 5: Advanced Server with Sampling
MCPTask_Void ExampleAdvancedServer() {
    std::cout << "Starting Advanced MCP Server Example..." << std::endl;

    SimpleMCPServer server;

    // Add AI integration tool
    server.AddTool(
        "analyze_code", "Analyzes code quality and suggests improvements",
        CreateToolSchema("object",
                         {{"code", CreateStringProperty("Code to analyze")},
                          {"language", CreateStringProperty("Programming language")}},
                         {"code", "language"}),
        [](const nlohmann::json& InArgs) -> ToolCallResponse {
            std::string code = InArgs["code"];
            std::string language = InArgs["language"];

            // Simulate code analysis
            nlohmann::json analysis = {
                {"language", language},
                {"lines_of_code", std::count(code.begin(), code.end(), '\n') + 1},
                {"complexity_score", 3.5},
                {"suggestions", nlohmann::json::array({"Consider adding error handling",
                                                       "Variable names could be more descriptive",
                                                       "Add documentation comments"})}};

            return CreateTextResult(analysis.dump(2), false);
        });

    // Set up sampling handler for LLM integration
    server.SetSamplingHandler(
        [](const SamplingCreateMessageRequest& InRequest) -> SamplingCreateMessageResponse {
            SamplingCreateMessageResponse response;

            // Simulate LLM response
            SamplingMessage message;
            message.Role = SamplingRole::Assistant;

            TextContent content;
            content.Type = "text";
            content.Text = "This is a simulated response from an LLM. In a real implementation, "
                           "you would integrate with an actual language model service.";
            message.Content.Content = content;

            response.Message = message;
            response.StopReason = StopReason::EndTurn;

            return response;
        });

    co_await server.Start("AdvancedExampleServer", "1.0.0");

    std::cout << "Advanced server started with sampling capabilities." << std::endl;

    // Simulate some progress updates
    for (int i = 0; i <= 100; i += 10) {
        co_await server.SendProgress("setup", i, 100);
        co_await server.SendLog(LoggingLevel::Info, "Setup progress: " + std::to_string(i) + "%");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));

    co_await server.Stop();
    std::cout << "Advanced server stopped." << std::endl;
}

// Main function to run all examples
MCPTask_Void RunAllExamples() {
    std::cout << "=== MCP C++ SDK Examples ===" << std::endl;

    try {
        std::cout << "\n1. Basic Server Example:" << std::endl;
        co_await ExampleServer();

        std::cout << "\n2. Basic Client Example:" << std::endl;
        co_await ExampleClient();

        std::cout << "\n3. HTTP Server Example:" << std::endl;
        co_await ExampleHTTPServer();

        std::cout << "\n4. HTTP Client Example:" << std::endl;
        co_await ExampleHTTPClient();

        std::cout << "\n5. Advanced Server Example:" << std::endl;
        co_await ExampleAdvancedServer();

    } catch (const std::exception& e) { std::cout << "Example Error: " << e.what() << std::endl; }

    std::cout << "\n=== All Examples Completed ===" << std::endl;
}

// Entry point for examples (can be called from main)
int main() {
    RunAllExamples().GetResult();
    return 0;
}