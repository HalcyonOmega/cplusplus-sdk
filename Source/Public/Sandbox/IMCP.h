#pragma once

#include <concepts>
#include <coroutine>
#include <functional>
#include <memory>
#include <string>
#include <variant>

#include "Core.h"
#include "Core/Types/Initialization.h"
#include "ITransport.h"
#include "NotificationBase.h"
#include "RequestBase.h"
#include "ResponseBase.h"

MCP_NAMESPACE_BEGIN

// TODO: @HalcyonOmega Begin Direct Translation Code
// The default request timeout, in milliseconds.
static constexpr const int64_t DEFAULT_REQUEST_TIMEOUT_MSEC = 60000;

// Additional initialization options.
struct ProtocolOptions {
    /**
     * Whether to restrict emitted requests to only those that the remote side has indicated that
     * they can handle, through their advertised capabilities.
     *
     * Note that this DOES NOT affect checking of _local_ side capabilities, as it is considered a
     * logic error to mis-specify those.
     *
     * Currently this defaults to false, for backwards compatibility with SDK versions that did not
     * advertise capabilities correctly. In future, this will default to true.
     */
    optional<bool> EnforceStrictCapabilities;
};

// Extra data given to request handlers.
template <typename SendRequestT, typename SendNotificationT> struct RequestHandlerExtra {
    // An abort signal used to communicate if the request was cancelled from the sender's side.
    AbortSignal Signal;

    // Information about a validated access token, provided to request handlers.
    optional<AuthInfo> AuthInfo;

    // The session ID from the transport, if available.
    optional<string> SessionID;

    // Metadata from the original request.
    optional<RequestMeta> Meta;

    // The JSON-RPC ID of the request being handled.
    // This can be useful for tracking or logging purposes.
    RequestID RequestID;

    // Sends a notification that relates to the current request being handled.
    // This is used by certain transports to correctly associate related messages.
    function<future<void>(const SendNotificationT&)> SendNotification;

    // Sends a request that relates to the current request being handled.
    // This is used by certain transports to correctly associate related messages.
    template <typename ResultT, typename SchemaT>
    function<future<ResultT>(const SendRequestT&, const SchemaT&, const optional<RequestOptions>&)>
        SendRequest;
};

template <typename T> T MergeCapabilities(const T& InBase, const T& InAdditional) {
    T Result = InBase;

    // Simple capability merging - merge JSON fields
    // This assumes T has a member that can be JSON-merged
    // The exact implementation would depend on the specific capability structure
    // TODO: Fix External Ref: Capabilities - Implement proper capability structure merging

    return Result;
}

// Information about a request's timeout state
struct TimeoutInfo {
    // TODO: Fix External Ref: Timer/Timeout mechanism
    uint64_t TimeoutId;
    chrono::steady_clock::time_point StartTime;
    int64_t Timeout;
    optional<int64_t> MaxTotalTimeout;
    bool ResetTimeoutOnProgress;
    function<void()> OnTimeout;
};
// TODO: @HalcyonOmega End Direct Translation Code

// Coroutine task for clean async operations
template <typename T> struct MCPTask {
    struct promise_type {
        std::variant<T, std::string> m_Result;

        MCPTask get_return_object() {
            return MCPTask{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_never initial_suspend() {
            return {};
        }
        std::suspend_never final_suspend() noexcept {
            return {};
        }

        void return_value(T Value)
            requires(!std::is_void_v<T>)
        {
            m_Result = std::move(Value);
        }

        void return_void()
            requires std::is_void_v<T>
        {
            m_Result = std::monostate{};
        }

        void unhandled_exception() {
            m_Result = "Coroutine exception occurred";
        }
    };

    std::coroutine_handle<promise_type> m_Handle;

    MCPTask(std::coroutine_handle<promise_type> Handle) : m_Handle(Handle) {}

    ~MCPTask() {
        if (m_Handle) { m_Handle.destroy(); }
    }

    // Non-copyable, movable
    MCPTask(const MCPTask&) = delete;
    MCPTask& operator=(const MCPTask&) = delete;
    MCPTask(MCPTask&& Other) noexcept : m_Handle(std::exchange(Other.m_Handle, {})) {}
    MCPTask& operator=(MCPTask&& Other) noexcept {
        if (this != &Other) {
            if (m_Handle) { m_Handle.destroy(); }
            m_Handle = std::exchange(Other.m_Handle, {});
        }
        return *this;
    }

    // Awaitable interface
    bool await_ready() const {
        return m_Handle.done();
    }
    void await_suspend(std::coroutine_handle<>) const {}

    T await_resume() const
        requires(!std::is_void_v<T>)
    {
        if (std::holds_alternative<std::string>(m_Handle.promise().m_Result)) {
            throw std::runtime_error(std::get<std::string>(m_Handle.promise().m_Result));
        }
        return std::get<T>(m_Handle.promise().m_Result);
    }

    void await_resume() const
        requires std::is_void_v<T>
    {
        if (std::holds_alternative<std::string>(m_Handle.promise().m_Result)) {
            throw std::runtime_error(std::get<std::string>(m_Handle.promise().m_Result));
        }
    }
};

// Specialized void task
using MCPTask_Void = MCPTask<void>;

// Result wrapper for operations that may fail
template <typename T> struct MCPResult {
    std::variant<T, std::string> Value;

    MCPResult(T&& Value) : Value(std::forward<T>(Value)) {}
    MCPResult(const T& Value) : Value(Value) {}
    MCPResult(const std::string& Error) : Value(Error) {}
    MCPResult(std::string&& Error) : Value(std::move(Error)) {}

    bool HasValue() const {
        return std::holds_alternative<T>(Value);
    }
    bool HasError() const {
        return std::holds_alternative<std::string>(Value);
    }

    const T& GetValue() const {
        return std::get<T>(Value);
    }
    T& GetValue() {
        return std::get<T>(Value);
    }
    const std::string& GetError() const {
        return std::get<std::string>(Value);
    }
};

// Protocol configuration with sensible defaults
struct ProtocolConfig {
    string ProtocolVersion = LATEST_PROTOCOL_VERSION;
    chrono::milliseconds RequestTimeout{30000};
    chrono::milliseconds DefaultTimeout{10000};
    bool AllowBatchRequests{true};
    size_t MaxConcurrentRequests{100};
    bool EnforceStrictCapabilities{false};
};

// Protocol callbacks for message handling
struct ProtocolCallbacks {
    function<void(const string&)> OnError;
    function<void(const NotificationBase&)> OnNotification;
    function<void(const RequestBase&, function<void(const ResponseBase&)>)> OnRequest;
    function<void(const ResponseBase&)> OnResponse;
    function<void()> OnInitialized;
    function<void(const ErrorBase&)> OnProtocolError;
};

// Core Protocol Interface
class IMCP {
  public:
    virtual ~IMCP() = default;

    /* Core Protocol Interface */
    // === Lifecycle Management ===
    virtual MCPTask<InitializeResult> Initialize(const InitializeRequest& InRequest) = 0;
    virtual MCPTask_Void Initialized() = 0;
    virtual MCPTask_Void Shutdown() = 0;

    // === Connection Management ===
    virtual bool IsConnected() const = 0;
    virtual bool IsInitialized() const = 0;
    virtual string GetProtocolVersion() const = 0;
    virtual MCPTask_Void Connect() = 0;
    virtual MCPTask_Void Disconnect() = 0;

    // === Transport ===
    void SetTransport(shared_ptr<ITransport> Transport) {
        m_Transport = Transport;
        // TODO: @HalcyonOmega - InitializeCallbacks()
        m_Transport->InitializeCallbacks();
    }
    shared_ptr<ITransport> GetTransport() const {
        return m_Transport;
    }

    // === Error Handling ===
    virtual void OnError(function<void(const ErrorBase&)> Callback) = 0;
    virtual void OnDisconnected(function<void()> Callback) = 0;

    // === Ping/Utility ===
    virtual MCPTask_Void Ping() = 0;

    /* Message Interface*/

    // === Senders ===

    virtual MCPTask_Void SendMessage(const MessageBase& InMessage);
    virtual MCPTask<ResponseBase> SendRequest(const RequestBase& InRequest);
    virtual MCPTask_Void SendResponse(const ResponseBase& InResponse);
    virtual MCPTask_Void SendNotification(const NotificationBase& InNotification);
    virtual MCPTask_Void SendError(const ErrorBase& InError);

    // === Handlers ===

    void SetCallbacks(const ProtocolCallbacks& InCallbacks) {
        m_Callbacks = InCallbacks;
    }
    const ProtocolCallbacks& GetCallbacks() const {
        return m_Callbacks;
    }

    void HandleIncomingMessage(const MessageBase& InMessage);
    virtual MCPTask<ResponseBase> HandleRequest(const RequestBase& InRequest) = 0;
    void HandleResponse(const ResponseBase& InResponse);
    virtual MCPTask_Void HandleNotification(const NotificationBase& InNotification) = 0;
    void RegisterPendingRequest(const string& InRequestID,
                                function<void(const ResponseBase&)> InHandler);

    /* Configuration */

    const ProtocolConfig& GetConfig() const {
        return m_Config;
    }
    void SetConfig(const ProtocolConfig& InConfig) {
        m_Config = InConfig;
    }

    // TODO: @HalcyonOmega From direct translation - is this needed?
  protected:
    // A method to check if a capability is supported by the remote side, for the given method to be
    // called.
    // This should be implemented by subclasses.
    virtual void AssertCapabilityForMethod(const string& InMethod) = 0;

    // A method to check if a notification is supported by the local side, for the given method to
    // be sent.
    // This should be implemented by subclasses.
    virtual void AssertNotificationCapability(const string& InMethod) = 0;

    // A method to check if a request handler is supported by the local side, for the given method
    // to be handled.
    // This should be implemented by subclasses.
    virtual void AssertRequestHandlerCapability(const string& InMethod) = 0;

    // Asserts that a request handler has not already been set for the given method, in preparation
    // for a new one being automatically installed.
    void AssertCanSetRequestHandler(const string& InMethod) {
        lock_guard<mutex> Lock(m_HandlersMutex);
        if (m_RequestHandlers.find(InMethod) != m_RequestHandlers.end()) {
            throw runtime_error("A request handler for " + InMethod
                                + " already exists, which would be overridden");
        }
    }

    /* Senders */
  public:
    // Sends a request and wait for a response.
    // Do not use this method to emit notifications! Use Notification() instead.
    template <typename ResultT>
    future<ResultT> Request(const SendRequestT& InRequest,
                            const optional<RequestOptions>& InOptions = nullopt) {
        if (!m_Transport) { throw runtime_error("Not connected"); }

        if (InOptions && InOptions->EnforceStrictCapabilities.value_or(false)) {
            AssertCapabilityForMethod(InRequest.Method);
        }

        if (InOptions && InOptions->Signal && InOptions->Signal->IsAborted()) {
            throw runtime_error("Request was aborted");
        }

        int64_t RequestID = m_RequestRequestID++;

        RequestBase Request;
        Request.JSON_RPC = MSG_JSON_RPC_VERSION;
        Request.ID = RequestID;
        Request.Method = InRequest.Method;
        Request.Params = InRequest.Params;

        promise<ResultT> ResultPromise;
        auto ResultFuture = ResultPromise.get_future();

        if (InOptions && InOptions->OnProgress) {
            lock_guard<mutex> Lock(m_HandlersMutex);
            m_ProgressHandlers[RequestID] = *InOptions->OnProgress;

            if (!Request.Params) { Request.Params = JSON::object(); }
            if (!Request.Params->contains(MSG_META)) {
                (*Request.Params)[MSG_META] = JSON::object();
            }
            (*Request.Params)[MSG_META][MSG_PROGRESS_TOKEN] = RequestID;
        }

        auto Cancel = [this, RequestID](const string& InReason) {
            {
                lock_guard<mutex> Lock(m_HandlersMutex);
                m_ResponseHandlers.erase(RequestID);
                m_ProgressHandlers.erase(RequestID);
            }
            CleanupTimeout(RequestID);

            // Send cancellation notification
            if (m_Transport) {
                NotificationBase CancelNotification;
                CancelNotification.JSON_RPC = MSG_JSON_RPC_VERSION;
                CancelNotification.Method = MTHD_NOTIFICATION_CANCELLED;
                CancelNotification.Params = JSON::object();
                (*CancelNotification.Params)[MSG_REQUEST_ID] = RequestID;
                (*CancelNotification.Params)["reason"] = InReason;

                m_Transport->Send(CancelNotification);
            }
        };

        {
            lock_guard<mutex> Lock(m_HandlersMutex);
            m_ResponseHandlers[RequestID] =
                [ResultPromise = move(ResultPromise), Cancel,
                 InOptions](const variant<ResponseBase, McpError>& InResponse) mutable {
                    // TODO: Fix External Ref: AbortSignal - Check if request was aborted
                    // if (options && options->Signal && options->Signal->IsAborted()) {
                    //     return;
                    // }

                    if (holds_alternative<McpError>(InResponse)) {
                        ResultPromise.set_exception(make_exception_ptr(get<McpError>(InResponse)));
                        return;
                    }

                    try {
                        const auto& Response = get<ResponseBase>(InResponse);
                        // Parse response result as ResultT using simple JSON conversion
                        if constexpr (std::is_same_v<ResultT, JSON>) {
                            ResultPromise.set_value(Response.Result);
                        } else {
                            // Simple JSON to type conversion
                            ResultT Result = Response.Result.get<ResultT>();
                            ResultPromise.set_value(Result);
                        }
                    } catch (const exception& InException) {
                        ResultPromise.set_exception(current_exception());
                    }
                };
        }

        // TODO: Fix External Ref: AbortSignal - Set up signal cancellation callback
        // if (options && options->Signal) {
        //     options->Signal->OnAbort([cancel]() { cancel("Request was cancelled"); });
        // }

        int64_t Timeout =
            InOptions && InOptions->Timeout ? *InOptions->Timeout : DEFAULT_REQUEST_TIMEOUT_MSEC;
        auto TimeoutHandler = [Cancel]() { Cancel("Request timed out"); };

        SetupTimeout(RequestID, Timeout,
                     InOptions && InOptions->MaxTotalTimeout ? *InOptions->MaxTotalTimeout
                                                             : nullopt,
                     TimeoutHandler,
                     InOptions && InOptions->ResetTimeoutOnProgress.value_or(false)
                         ? *InOptions->ResetTimeoutOnProgress
                         : false);

        // Send request via transport
        // TODO: Relook RequestJSON type
        try {
            JSON RequestJSON = {{MSG_JSON_RPC, Request.JSON_RPC},
                                {MSG_ID, Request.ID},
                                {MSG_METHOD, Request.Method}};
            if (Request.Params) { RequestJSON[MSG_PARAMS] = *Request.Params; }

            Transport::TransportSendOptions TransportOptions;
            if (InOptions) {
                TransportOptions.RelatedRequestID = InOptions->RelatedRequestID;
                TransportOptions.ResumptionToken = InOptions->ResumptionToken;
                TransportOptions.OnResumptionToken = InOptions->OnResumptionToken;
            }

            m_Transport->Send(RequestJSON, TransportOptions);
        } catch (const exception& InException) {
            CleanupTimeout(RequestID);
            throw;
        }

        return ResultFuture;
    }

    // Emits a notification, which is a one-way message that does not expect a response.
    future<void> Notification(const SendNotificationT& InNotification,
                              const optional<NotificationOptions>& InOptions = nullopt) {
        if (!m_Transport) { throw runtime_error("Not connected"); }

        AssertNotificationCapability(InNotification.Method);

        NotificationBase Notification;
        Notification.JSON_RPC = MSG_JSON_RPC_VERSION;
        Notification.Method = InNotification.Method;
        Notification.Params = InNotification.Params;

        JSON NotificationJSON = {{MSG_JSON_RPC, Notification.JSON_RPC},
                                 {MSG_METHOD, Notification.Method}};
        if (Notification.Params) { NotificationJSON[MSG_PARAMS] = *Notification.Params; }

        Transport::TransportSendOptions TransportOptions;
        if (InOptions) { TransportOptions.RelatedRequestID = InOptions->RelatedRequestID; }

        m_Transport->Send(NotificationJSON, TransportOptions);

        promise<void> Promise;
        Promise.set_value();
        return Promise.get_future();
    }

    /* Handlers */

    // Registers a handler to invoke when this protocol object receives a request with the given
    // method.
    // Note that this will replace any previous request handler for the same method.
    void SetRequestHandler(
        const string& InMethod,
        function<future<SendResultT>(const RequestBase&,
                                     const RequestHandlerExtra<SendRequestT, SendNotificationT>&)>
            InHandler) {
        AssertRequestHandlerCapability(InMethod);

        lock_guard<mutex> Lock(m_HandlersMutex);
        m_RequestHandlers[InMethod] = move(InHandler);
    }

    // Registers a handler to invoke when this protocol object receives a notification with the
    // given method.
    // Note that this will replace any previous notification handler for the same method.
    void SetNotificationHandler(const string& InMethod,
                                function<future<void>(const NotificationBase&)> InHandler) {
        lock_guard<mutex> Lock(m_HandlersMutex);
        m_NotificationHandlers[InMethod] = move(InHandler);
    }

    // Removes the request handler for the given method.
    void RemoveRequestHandler(const string& InMethod) {
        lock_guard<mutex> Lock(m_HandlersMutex);
        m_RequestHandlers.erase(InMethod);
    }

    // Removes the notification handler for the given method.
    void RemoveNotificationHandler(const string& InMethod) {
        lock_guard<mutex> Lock(m_HandlersMutex);
        m_NotificationHandlers.erase(InMethod);
    }

  private:
    shared_ptr<Transport> m_Transport;
    RequestID m_RequestRequestID{0};

    unordered_map<string, function<future<SendResultT>(
                              const RequestBase&,
                              const RequestHandlerExtra<SendRequestT, SendNotificationT>&)>>
        m_RequestHandlers;
    unordered_map<RequestID, AbortSignal> m_RequestHandlerAbortControllers;
    unordered_map<string, function<future<void>(const NotificationBase&)>> m_NotificationHandlers;
    unordered_map<int64_t, function<void(const variant<ResponseBase, ErrorBase>&)>>
        m_ResponseHandlers;
    unordered_map<int64_t, ProgressCallback> m_ProgressHandlers;
    unordered_map<int64_t, TimeoutInfo> m_TimeoutInfo;

    mutex m_HandlersMutex;
    optional<ProtocolOptions> m_Options;

  public:
    // Callback for when the connection is closed for any reason.
    // This is invoked when close() is called as well.
    function<void()> OnClose;

    // Callback for when an error occurs.
    // Note that errors are not necessarily fatal; they are used for reporting any kind of
    // exceptional condition out of band.
    function<void(const exception&)> OnError;

    // A handler to invoke for any request types that do not have their own handler installed.
    function<future<SendResultT>(const RequestBase&)> FallbackRequestHandler;

    // A handler to invoke for any notification types that do not have their own handler installed.
    function<future<void>(const NotificationBase&)> FallbackNotificationHandler;

    explicit IMCPProtocol(const optional<ProtocolOptions>& InOptions = nullopt)
        : m_Options(InOptions) {
        // Set up default handlers for cancelled notifications and progress notifications
        SetNotificationHandler(MTHD_NOTIFICATION_CANCELLED,
                               [this](const NotificationBase& InNotification) -> future<void> {
                                   // Extract RequestID and reason from notification params
                                   if (InNotification.GetParams()
                                       && InNotification.GetParams()->contains(MSG_REQUEST_ID)) {
                                       RequestID RequestID =
                                           (*InNotification.GetParams())[MSG_REQUEST_ID];

                                       lock_guard<mutex> lock(m_HandlersMutex);
                                       auto it = m_RequestHandlerAbortControllers.find(RequestID);
                                       if (it != m_RequestHandlerAbortControllers.end()) {
                                           // TODO: Fix External Ref: AbortSignal - Signal abort
                                           // with reason
                                           string Reason = InNotification.GetParams()->value(
                                               "reason", "Request cancelled");
                                           // it->second.abort(reason);
                                       }
                                   }
                                   promise<void> Promise;
                                   Promise.set_value();
                                   return Promise.get_future();
                               });

        SetNotificationHandler(MTHD_NOTIFICATION_PROGRESS,
                               [this](const NotificationBase& InNotification) -> future<void> {
                                   OnProgress(InNotification);
                                   promise<void> Promise;
                                   Promise.set_value();
                                   return Promise.get_future();
                               });

        // Automatic pong by default for ping requests
        SetRequestHandler(MTHD_PING,
                          [](const RequestBase& InRequest,
                             const RequestHandlerExtra<SendRequestT, SendNotificationT>& InExtra)
                              -> future<SendResultT> {
                              // Return empty object as ping response
                              promise<SendResultT> Promise;
                              Promise.set_value(SendResultT{});
                              return Promise.get_future();
                          });
    }

  private:
    void OnProgress(const NotificationBase& InNotification) {
        if (!InNotification.GetParams()
            || !InNotification.GetParams()->contains(MSG_PROGRESS_TOKEN)) {
            OnErrorInternal(runtime_error("Received a progress notification without progressToken: "
                                          + InNotification.GetMethod()));
            return;
        }

        int64_t ProgressToken = (*InNotification.GetParams())[MSG_PROGRESS_TOKEN];

        ProgressCallback Handler;
        {
            lock_guard<mutex> lock(m_HandlersMutex);
            auto it = m_ProgressHandlers.find(ProgressToken);
            if (it != m_ProgressHandlers.end()) { Handler = it->second; }
        }

        if (!Handler) {
            OnErrorInternal(runtime_error("Received a progress notification for an unknown token: "
                                          + to_string(ProgressToken)));
            return;
        }

        auto ResponseHandler = m_ResponseHandlers.find(ProgressToken);
        auto TimeoutIt = m_TimeoutInfo.find(ProgressToken);

        if (TimeoutIt != m_TimeoutInfo.end() && ResponseHandler != m_ResponseHandlers.end()
            && TimeoutIt->second.ResetTimeoutOnProgress) {
            try {
                ResetTimeout(ProgressToken);
            } catch (const ErrorBase& InError) {
                ResponseHandler->second(InError);
                return;
            }
        }

        // Extract the params excluding progressToken (equivalent to TypeScript destructuring)
        Progress ProgressData;
        ProgressData.ProgressToken = ProgressToken;

        // Extract the params excluding progressToken (equivalent to TypeScript destructuring)
        if (InNotification.GetParams() && InNotification.GetParams()->contains(MSG_DATA)) {
            ProgressData.Data = (*InNotification.GetParams())[MSG_DATA];
        }

        Handler(ProgressData);
    }

    void SetupTimeout(int64_t InRequestID, int64_t InTimeout,
                      const optional<int64_t>& InMaxTotalTimeout, function<void()> InOnTimeout,
                      bool InResetTimeoutOnProgress = false) {
        TimeoutInfo Info{.TimeoutID = static_cast<uint64_t>(InRequestID),
                         .StartTime = chrono::steady_clock::now(),
                         .Timeout = InTimeout,
                         .MaxTotalTimeout = InMaxTotalTimeout,
                         .ResetTimeoutOnProgress = InResetTimeoutOnProgress,
                         .OnTimeout = move(InOnTimeout)};

        lock_guard<mutex> Lock(m_HandlersMutex);
        m_TimeoutInfo[InRequestID] = move(Info);

        // TODO: Fix External Ref: Timer/Timeout mechanism - Set up actual timer that calls
        // onTimeout after timeout milliseconds
    }

    bool ResetTimeout(int64_t InRequestID) {
        lock_guard<mutex> Lock(m_HandlersMutex);
        auto It = m_TimeoutInfo.find(InRequestID);
        if (It == m_TimeoutInfo.end()) return false;

        auto& Info = It->second;
        auto TotalElapsed = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now()
                                                                        - Info.StartTime)
                                .count();

        if (Info.MaxTotalTimeout && TotalElapsed >= *Info.MaxTotalTimeout) {
            m_TimeoutInfo.erase(It);
            throw ErrorBase(
                ErrorCode::RequestTimeout, "Maximum total timeout exceeded",
                JSON{{"maxTotalTimeout", *Info.MaxTotalTimeout}, {"totalElapsed", TotalElapsed}});
        }

        // TODO: Fix External Ref: Timer/Timeout mechanism - Reset actual timer
        return true;
    }

    void CleanupTimeout(int64_t InRequestID) {
        lock_guard<mutex> Lock(m_HandlersMutex);
        auto It = m_TimeoutInfo.find(InRequestID);
        if (It != m_TimeoutInfo.end()) {
            // TODO: Fix External Ref: Timer/Timeout mechanism - Cancel actual timer
            m_TimeoutInfo.erase(It);
        }
    }

    void OnCloseInternal() {
        unordered_map<int64_t, function<void(const variant<ResponseBase, ErrorBase>&)>>
            ResponseHandlers;

        {
            lock_guard<mutex> Lock(m_HandlersMutex);
            ResponseHandlers = move(m_ResponseHandlers);
            m_ResponseHandlers.clear();
            m_ProgressHandlers.clear();
        }

        m_Transport.reset();

        if (m_OnClose) { m_OnClose(); }

        ErrorBase Error(ErrorCode::ConnectionClosed, "Connection closed");
        for (const auto& [ID, Handler] : ResponseHandlers) { Handler(Error); }
    }

    void OnErrorInternal(const exception& InError) {
        if (m_OnError) { m_OnError(InError); }
    }

    void OnNotification(const NotificationBase& InNotification) {
        function<future<void>(const NotificationBase&)> Handler;

        {
            lock_guard<mutex> Lock(m_HandlersMutex);
            auto It = m_NotificationHandlers.find(InNotification.Method);
            if (It != m_NotificationHandlers.end()) { Handler = It->second; }
        }

        if (!handler && FallbackNotificationHandler) {
            // Convert NotificationBase to Notification type
            Notification Notification;
            Notification.Method = InNotification.Method;
            Notification.Params = InNotification.Params;

            Handler = [this, Notification](const NotificationBase&) -> future<void> {
                return FallbackNotificationHandler(Notification);
            };
        }

        // Ignore notifications not being subscribed to.
        if (!Handler) { return; }

        // Execute handler asynchronously and catch errors
        async(launch::async, [this, Handler, InNotification]() {
            try {
                auto Future = Handler(InNotification);
                Future.wait();
            } catch (const exception& InError) {
                OnErrorInternal(runtime_error("Uncaught error in notification handler: "
                                              + string(InError.what())));
            }
        });
    }

    void OnRequest(const RequestBase& InRequest, const optional<AuthInfo>& InAuthInfo = nullopt) {
        function<future<SendResultT>(const RequestBase&,
                                     const RequestHandlerExtra<SendRequestT, SendNotificationT>&)>
            Handler;

        {
            lock_guard<mutex> Lock(m_HandlersMutex);
            auto It = m_RequestHandlers.find(InRequest.Method);
            if (It != m_RequestHandlers.end()) { Handler = It->second; }
        }

        if (!Handler && FallbackRequestHandler) {
            // Convert and use fallback handler
            Request Request;
            Request.Method = InRequest.Method;
            Request.Params = InRequest.Params;

            Handler = [this, Request](const RequestBase&,
                                      const RequestHandlerExtra<SendRequestT, SendNotificationT>&)
                -> future<SendResultT> { return FallbackRequestHandler(Request); };
        }

        if (!Handler) {
            // Send method not found error
            ErrorBase ErrorResponse;
            ErrorResponse.JSON_RPC = MSG_JSON_RPC_VERSION;
            ErrorResponse.ID = InRequest.ID;
            ErrorResponse.Error.Code = static_cast<int32_t>(ErrorCode::MethodNotFound);
            ErrorResponse.Error.Message = "Method not found";

            if (m_Transport) {
                // TODO: Fix External Ref: Transport - Send error response via transport
                // m_Transport->Send(ErrorResponse);
            }
            return;
        }

        // TODO: Fix External Ref: AbortSignal - Create proper AbortSignal
        AbortSignal AbortSignal;

        {
            lock_guard<mutex> Lock(m_HandlersMutex);
            m_RequestHandlerAbortControllers[InRequest.ID] = AbortSignal;
        }

        RequestHandlerExtra<SendRequestT, SendNotificationT> Extra;
        Extra.Signal = AbortSignal;
        Extra.SessionID = m_Transport ? m_Transport->SessionID : nullopt;
        Extra.AuthInfo = InAuthInfo;
        Extra.Meta = (InRequest.Params && InRequest.Params->contains(MSG_META))
                         ? optional<RequestMeta>{RequestMeta{}} // Basic extraction
                         : nullopt;
        Extra.RequestID = InRequest.ID;

        Extra.SendNotification = [this, RequestID = InRequest.ID](
                                     const SendNotificationT& InNotification) -> future<void> {
            return Notification(InNotification, NotificationOptions{.RelatedRequestID = RequestID});
        };

        Extra.SendRequest = [this, RequestID = InRequest.ID]<typename ResultT, typename SchemaT>(
                                const SendRequestT& InRequest, const SchemaT& InSchema,
                                const optional<RequestOptions>& InOptions) -> future<ResultT> {
            RequestOptions Options = InOptions.value_or(RequestOptions{});
            Options.RelatedRequestID = RequestID;
            return Request<ResultT>(InRequest, Options);
        };

        // Execute handler asynchronously
        async(launch::async, [this, Handler, InRequest, Extra]() {
            try {
                auto ResultFuture = Handler(InRequest, Extra);
                auto Result = ResultFuture.get();

                // TODO: Fix External Ref: AbortSignal - Check if request was aborted
                if (!Extra.Signal.IsAborted()) {
                    if (m_Transport) {
                        ResponseBase Response;
                        Response.JSON_RPC = MSG_JSON_RPC_VERSION;
                        Response.ID = InRequest.ID;
                        Response.Result = JSON{}; // Convert SendResultT to JSON

                        // TODO: Fix External Ref: Transport - Send response via transport
                        m_Transport->Send(Response);
                    }
                }
            } catch (const exception& InError) {
                // TODO: Fix External Ref: AbortSignal - Check if request was aborted
                if (!Extra.Signal.IsAborted()) {
                    // Send error response
                    ErrorBase ErrorResponse;
                    ErrorResponse.JSON_RPC = MSG_JSON_RPC_VERSION;
                    ErrorResponse.ID = InRequest.ID;
                    ErrorResponse.Error.Code = static_cast<int32_t>(ErrorCode::InternalError);
                    ErrorResponse.Error.Message = InError.what();

                    if (m_Transport) { m_Transport->Send(ErrorResponse); }
                }
            }

            // Cleanup
            {
                lock_guard<mutex> Lock(m_HandlersMutex);
                m_RequestHandlerAbortControllers.erase(InRequest.ID);
            }
        });
    }

    void OnResponse(const variant<ResponseBase, ErrorBase>& InResponse) {
        RequestID ResponseID;

        if (holds_alternative<ResponseBase>(InResponse)) {
            ResponseID = get<ResponseBase>(InResponse).ID;
        } else {
            ResponseID = get<ErrorBase>(InResponse).ID;
        }

        // Convert RequestID to int64_t for message correlation
        int64_t RequestID = 0;
        if (holds_alternative<int64_t>(ResponseID)) {
            RequestID = get<int64_t>(ResponseID);
        } else if (holds_alternative<string>(ResponseID)) {
            // Handle string IDs by parsing to int
            try {
                RequestID = stoll(get<string>(ResponseID));
            } catch (const exception&) {
                OnErrorInternal(runtime_error("Cannot correlate response with string ID: "
                                              + get<string>(ResponseID)));
                return;
            }
        }

        function<void(const variant<ResponseBase, McpError>&)> Handler;

        {
            lock_guard<mutex> Lock(m_HandlersMutex);
            auto It = m_ResponseHandlers.find(RequestID);
            if (It != m_ResponseHandlers.end()) {
                Handler = It->second;
                m_ResponseHandlers.erase(It);
                m_ProgressHandlers.erase(RequestID);
            }
        }

        if (!Handler) {
            OnErrorInternal(runtime_error("Received a response for an unknown message ID"));
            return;
        }

        CleanupTimeout(RequestID);

        if (holds_alternative<ResponseBase>(InResponse)) {
            Handler(get<ResponseBase>(InResponse));
        } else {
            const auto& ErrorResp = get<ErrorBase>(InResponse);
            McpError Error(static_cast<ErrorCode>(ErrorResp.Error.Code), ErrorResp.Error.Message,
                           ErrorResp.Error.Data);
            Handler(Error);
        }
    }

    // TODO: End @HalcyonOmega From direct translation - is this needed?

  private:
    ProtocolConfig m_Config;
    ProtocolCallbacks m_Callbacks;
    shared_ptr<ITransport> m_Transport;

    RequestID m_NextRequestID;
    queue<function<void(const ResponseBase&)>> m_PendingRequests;
    mutex m_PendingRequestsMutex;
};

// Content Type Concepts
template <typename T>
concept MCPContent = requires(T t) {
    { t.GetType() } -> convertible_to<string>;
    { t.GetData() } -> convertible_to<string>;
};

template <typename T>
concept MCPHandler = requires(T t) { is_invocable_v<T>; };

// Practical MCP Implementation Helper Class
// This class provides implementation-specific functionality not defined in the MCP spec
// but commonly needed for real-world usage
class PracticalMCP {
  public:
    virtual ~PracticalMCP() = default;

    // Connection Management (implementation details)
    virtual void SetConnectionTimeout(chrono::milliseconds timeout) = 0;
    virtual chrono::milliseconds GetConnectionTimeout() const = 0;
    virtual void SetRetryPolicy(int maxRetries, chrono::milliseconds retryDelay) = 0;

    // Session State Management (practical concerns)
    virtual void SaveState(const string& statePath) = 0;
    virtual bool LoadState(const string& statePath) = 0;
    virtual void ClearState() = 0;

    // Performance & Monitoring
    virtual void EnableMetrics(bool enable) = 0;
    virtual unordered_map<string, double> GetMetrics() const = 0;
    virtual void SetMaxConcurrentRequests(size_t maxRequests) = 0;
    virtual size_t GetActiveRequestCount() const = 0;

    // Security & Validation
    virtual void SetRequestValidator(function<bool(const RequestBase&)> validator) = 0;
    virtual void SetResponseValidator(function<bool(const ResponseBase&)> validator) = 0;
    virtual void EnableRequestLogging(bool enable, const string& logPath = "") = 0;

    // Advanced Transport Configuration
    virtual void SetCustomHeaders(const unordered_map<string, string>& headers) = 0;
    virtual void SetCompressionEnabled(bool enable) = 0;
    virtual void SetKeepAliveSettings(bool enable, chrono::seconds interval) = 0;

    // Event System (for advanced monitoring)
    virtual void OnRequestSent(function<void(const RequestBase&)> callback) = 0;
    virtual void OnResponseReceived(function<void(const ResponseBase&)> callback) = 0;
    virtual void OnNotificationReceived(function<void(const NotificationBase&)> callback) = 0;
    virtual void OnConnectionStateChanged(function<void(bool connected)> callback) = 0;

    // Batching Support (JSON-RPC batch operations)
    virtual void EnableBatching(bool enable, size_t maxBatchSize = 10) = 0;
    virtual void FlushBatch() = 0;

    // Resource Management
    virtual void SetResourceCacheSize(size_t maxCacheSize) = 0;
    virtual void ClearResourceCache() = 0;

    // Development/Debug Features
    virtual void SetDebugMode(bool enable) = 0;
    virtual string DumpInternalState() const = 0;
    virtual void InjectTestFailure(const string& methodName, const string& errorType) = 0;
};

// Utility Functions
namespace Utilities {
// Protocol version comparison
int CompareProtocolVersions(const string& version1, const string& version2);
bool IsVersionCompatible(const string& clientVersion, const string& serverVersion);
} // namespace Utilities

MCP_NAMESPACE_END