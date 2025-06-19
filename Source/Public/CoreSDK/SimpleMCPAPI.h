#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "MCPTask.h"
#include "Macros.h"
#include "json.hpp"

// Forward declarations
class MCPClient;
class MCPServer;

MCP_NAMESPACE_BEGIN

// Basic types needed for the simple API
struct ToolResult {
    std::vector<std::string> TextOutputs;
    bool IsError = false;
    std::string ErrorMessage;
};

struct PromptResult {
    std::string Description;
    std::vector<std::string> Messages;
};

struct ResourceContent {
    std::string URI;
    std::string Text;
    std::string MimeType;
};

// Simple tool interface
class IMCPTool {
  public:
    virtual ~IMCPTool() = default;
    virtual std::string GetName() const = 0;
    virtual std::string GetDescription() const = 0;
    virtual MCPTask<ToolResult>
    Execute(const std::unordered_map<std::string, nlohmann::json>& InArguments) = 0;
};

// Simple prompt interface
class IMCPPrompt {
  public:
    virtual ~IMCPPrompt() = default;
    virtual std::string GetName() const = 0;
    virtual std::string GetDescription() const = 0;
    virtual MCPTask<PromptResult>
    Generate(const std::unordered_map<std::string, std::string>& InArguments) = 0;
};

// Simple resource interface
class IMCPResource {
  public:
    virtual ~IMCPResource() = default;
    virtual std::string GetURI() const = 0;
    virtual std::string GetName() const = 0;
    virtual MCPTask<ResourceContent> Read() = 0;
};

// Configuration for simple client
struct SimpleClientConfig {
    std::string ClientName = "SimpleMCPClient";
    std::string ClientVersion = "1.0.0";
};

// Configuration for simple server
struct SimpleServerConfig {
    std::string ServerName = "SimpleMCPServer";
    std::string ServerVersion = "1.0.0";
};

// Simple MCP Client
class SimpleMCPClient {
  public:
    // Factory methods
    static std::unique_ptr<SimpleMCPClient>
    CreateStdio(const std::string& InCommand, const std::vector<std::string>& InArguments = {},
                const SimpleClientConfig& InConfig = {});

    static std::unique_ptr<SimpleMCPClient> CreateHTTP(const std::string& InHost, uint16_t InPort,
                                                       const SimpleClientConfig& InConfig = {});

    ~SimpleMCPClient();

    // Connection management
    MCPTask<bool> Connect();
    MCPTaskVoid Disconnect();
    bool IsConnected() const;

    // Tool operations
    MCPTask<std::vector<std::string>> ListTools();
    MCPTask<ToolResult>
    CallTool(const std::string& InName,
             const std::unordered_map<std::string, nlohmann::json>& InArguments = {});

    // Prompt operations
    MCPTask<std::vector<std::string>> ListPrompts();
    MCPTask<PromptResult>
    GetPrompt(const std::string& InName,
              const std::unordered_map<std::string, std::string>& InArguments = {});

    // Resource operations
    MCPTask<std::vector<std::string>> ListResources();
    MCPTask<ResourceContent> ReadResource(const std::string& InURI);

  private:
    explicit SimpleMCPClient(std::unique_ptr<MCPClient> InClient);
    std::unique_ptr<MCPClient> m_Client;
};

// Simple MCP Server
class SimpleMCPServer {
  public:
    // Factory methods
    static std::unique_ptr<SimpleMCPServer> CreateStdio(const SimpleServerConfig& InConfig = {});
    static std::unique_ptr<SimpleMCPServer> CreateHTTP(uint16_t InPort,
                                                       const SimpleServerConfig& InConfig = {});

    ~SimpleMCPServer();

    // Server lifecycle
    MCPTask<bool> Start();
    MCPTaskVoid Stop();
    bool IsRunning() const;

    // Tool management
    void AddTool(std::unique_ptr<IMCPTool> InTool);
    void RemoveTool(const std::string& InName);

    // Prompt management
    void AddPrompt(std::unique_ptr<IMCPPrompt> InPrompt);
    void RemovePrompt(const std::string& InName);

    // Resource management
    void AddResource(std::unique_ptr<IMCPResource> InResource);
    void RemoveResource(const std::string& InURI);

    // Logging
    void LogInfo(const std::string& InMessage);
    void LogError(const std::string& InMessage);

  private:
    explicit SimpleMCPServer(std::unique_ptr<MCPServer> InServer);
    std::unique_ptr<MCPServer> m_Server;

    std::unordered_map<std::string, std::unique_ptr<IMCPTool>> m_Tools;
    std::unordered_map<std::string, std::unique_ptr<IMCPPrompt>> m_Prompts;
    std::unordered_map<std::string, std::unique_ptr<IMCPResource>> m_Resources;
};

// Helper function to create a simple tool
template <typename TFunc>
std::unique_ptr<IMCPTool> CreateTool(const std::string& InName, const std::string& InDescription,
                                     TFunc InFunction) {
    class LambdaTool : public IMCPTool {
      public:
        LambdaTool(const std::string& InToolName, const std::string& InToolDescription,
                   TFunc InToolFunction)
            : m_Name(InToolName), m_Description(InToolDescription), m_Function(InToolFunction) {}

        std::string GetName() const override {
            return m_Name;
        }
        std::string GetDescription() const override {
            return m_Description;
        }

        MCPTask<ToolResult>
        Execute(const std::unordered_map<std::string, nlohmann::json>& InArguments) override {
            co_return m_Function(InArguments);
        }

      private:
        std::string m_Name;
        std::string m_Description;
        TFunc m_Function;
    };

    return std::make_unique<LambdaTool>(InName, InDescription, InFunction);
}

// Helper function to create a simple prompt
template <typename TFunc>
std::unique_ptr<IMCPPrompt> CreatePrompt(const std::string& InName,
                                         const std::string& InDescription, TFunc InFunction) {
    class LambdaPrompt : public IMCPPrompt {
      public:
        LambdaPrompt(const std::string& InPromptName, const std::string& InPromptDescription,
                     TFunc InPromptFunction)
            : m_Name(InPromptName), m_Description(InPromptDescription),
              m_Function(InPromptFunction) {}

        std::string GetName() const override {
            return m_Name;
        }
        std::string GetDescription() const override {
            return m_Description;
        }

        MCPTask<PromptResult>
        Generate(const std::unordered_map<std::string, std::string>& InArguments) override {
            co_return m_Function(InArguments);
        }

      private:
        std::string m_Name;
        std::string m_Description;
        TFunc m_Function;
    };

    return std::make_unique<LambdaPrompt>(InName, InDescription, InFunction);
}

// Helper function to create a simple resource
template <typename TFunc>
std::unique_ptr<IMCPResource> CreateResource(const std::string& InURI, const std::string& InName,
                                             TFunc InFunction) {
    class LambdaResource : public IMCPResource {
      public:
        LambdaResource(const std::string& InResourceURI, const std::string& InResourceName,
                       TFunc InResourceFunction)
            : m_URI(InResourceURI), m_Name(InResourceName), m_Function(InResourceFunction) {}

        std::string GetURI() const override {
            return m_URI;
        }
        std::string GetName() const override {
            return m_Name;
        }

        MCPTask<ResourceContent> Read() override {
            co_return m_Function();
        }

      private:
        std::string m_URI;
        std::string m_Name;
        TFunc m_Function;
    };

    return std::make_unique<LambdaResource>(InURI, InName, InFunction);
}

MCP_NAMESPACE_END