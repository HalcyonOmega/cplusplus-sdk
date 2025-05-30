#pragma once

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

class Transport;

namespace CPP_MCP {

using std::condition_variable;
using std::mutex;
using std::shared_ptr;
using std::string;
using std::thread;
using std::unordered_map;
using std::vector;

enum class SessionState {
    Default,
    Initializing,
    Initialized,
    Disconnected,
    Error,
    ShuttingDown,
    Shutdown
};

class Session {
  public:
    ~Session() = default;

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    static SessionState GetState();
    static void SetState(SessionState InState);

    void Ready();
    void Run();
    void Terminate();

    void SetTransport(shared_ptr<Transport>& InTransport);
    void SetServerInfo(const ServerInfo& InServerInfo); // TODO: Implementation was original keyword

    void SetServerInfo(const Implementation& InServerInfo);
    void SetServerCapabilities(const ServerCapabilities& InCapabilities);
    void SetServerToolsPagination(bool InPagination);
    void SetServerTools(const vector<Tool>& InTools);
    void SetServerCallToolsTasks(
        const unordered_map<string, shared_ptr<ProcessCallToolRequest>>& InHashCallToolsTasks);
    Implementation GetServerInfo() const;
    ServerCapabilities GetServerCapabilities() const;
    bool GetServerToolsPagination() const;
    vector<Tool> GetServerTools() const;
    shared_ptr<Transport> GetTransport() const;
    SessionState GetSessionState() const;
    shared_ptr<ProcessRequest> GetServerCallToolsTask(const string& InStrToolName);

  private:
    Session() = default;
    void ParseMessage(const string& InMessage, shared_ptr<Message>& Parsed);
    void ParseRequest(const string& InMessage, shared_ptr<Message>& Parsed);
    void ParseResponse(const string& InMessage, shared_ptr<Message>& Parsed);
    void ParseNotification(const string& InMessage, shared_ptr<Message>& Parsed);
    void ProcessMessage(int ErrCode, const shared_ptr<Message>& Parsed);
    void ProcessRequest(int ErrCode, const shared_ptr<Message>& Parsed);
    void ProcessResponse(int ErrCode, const shared_ptr<Message>& Parsed);
    void ProcessNotification(int ErrCode, const shared_ptr<Message>& Parsed);
    void SwitchState(SessionState InState);

    // Threading
    void CommitAsyncTask(const shared_ptr<Task>& InTaskPtr);
    void CancelAsyncTask(const RequestId& InRequestId);
    void StartAsyncTaskThread();
    void StopAsyncTaskThread();
    void AsyncThreadProc();

    static Session SessionInstance;

    SessionState CurrentState{SessionState::Default};
    shared_ptr<Transport> SessionTransport;

    Implementation ServerInfo;
    ServerCapabilities Capabilities;
    vector<Tool> Tools;
    bool ToolsPagination{false};

    unordered_map<MessageCategory, vector<shared_ptr<Message>>> MessageMap;
    unordered_map<string, shared_ptr<ProcessCallToolRequest>> CallToolsTasks;

    unique_ptr<thread> TaskThread;
    atomic_bool RunAsyncTask{true};
    mutex AsyncThread;
    condition_variable AsyncThreadCondition;
    deque<shared_ptr<Task>> AsyncTasks;
    vector<RequestId> CancelledTaskIDs;
    vector<shared_ptr<Task>> AsyncTasksCache;
};

} // namespace CPP_MCP
