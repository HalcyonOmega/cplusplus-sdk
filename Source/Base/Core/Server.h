#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

class Transport;
namespace CPP_MCP {

using namespace std;

template <class T> class Server {
  public:
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    static Server& GetInstance() {
        return T::ServerInstance;
    }

    void SetServerInfo(const Implementation& InServerInfo) {
        Session::GetInstance().SetServerInfo(InServerInfo);
    }

    void SetTransport(const shared_ptr<Transport>& InTransport) {
        Session::GetInstance().SetTransport(InTransport);
    }

    void RegisterServerToolsCapabilities(const Tools& InTools) {
        ServerCapabilities.tools = InTools;
    }

    void RegisterServerResourcesCapabilities(const Resources& InResources) {
        ServerCapabilities.resources = InResources;
    }

    void RegisterServerPromptsCapabilities(const Prompts& InPrompts) {
        ServerCapabilities.prompts = InPrompts;
    }

    void RegisterServerTools(const vector<Tool>& InTools, bool bPagination) {
        Session::GetInstance().SetServerToolsPagination(bPagination);
        Session::GetInstance().SetServerTools(InTools);
    }

    void RegisterToolsTasks(const string& InStrToolName,
                            shared_ptr<ProcessCallToolRequest> InSpTask) {
        CallToolsTasksMap[InStrToolName] = InSpTask;
    }

    virtual int Initialize() = 0;

    int Start() {
        if (!Session::GetInstance().GetTransport())
            Session::GetInstance().SetTransport(make_shared<CStdioTransport>());
        Session::GetInstance().SetServerCapabilities(ServerCapabilities);
        Session::GetInstance().SetServerCallToolsTasks(CallToolsTasksMap);

        int ErrCode = Session::GetInstance().Ready();
        if (ERRCODE_OK != ErrCode) return ErrCode;

        Session::GetInstance().Run();

        return ERRCODE_OK;
    }

    int Stop() {
        Session::GetInstance().Terminate();

        return ERRCODE_OK;
    }

  protected:
    Server() = default;
    ~Server() = default;

    ServerCapabilities ServerCapabilities;
    unordered_map<string, shared_ptr<ProcessCallToolRequest>> CallToolsTasksMap;
};

} // namespace CPP_MCP