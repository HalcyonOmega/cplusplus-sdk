#pragma once

#include "Auth/Types/Auth.h"
#include "Communication/Transport/Transport.h"
#include "Core.h"
#include "Core/Messages/NotificationBase.h"
#include "Core/Messages/RequestBase.h"
#include "Core/Messages/ResponseBase.h"

MCP_NAMESPACE_BEGIN

// TODO: @HalcyonOmega Cleanup this file

// The default request timeout, in milliseconds.
static constexpr const int64_t DEFAULT_REQUEST_TIMEOUT_MSEC = 60000;

// Callback for progress notifications.
using ProgressCallback = function<void(const Progress&)>;

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

// Options that can be given per request.
struct RequestOptions : public TransportSendOptions {
    // If set, requests progress notifications from the remote end (if supported). When progress
    // notifications are received, this callback will be invoked.
    optional<ProgressCallback> OnProgress;

    // Can be used to cancel an in-flight request. This will cause an AbortError to be raised from
    // request().
    optional<AbortSignal> Signal;

    // A timeout (in milliseconds) for this request. If exceeded, an MCP_Error with code
    // `RequestTimeout` will be raised from request().
    //
    // If not specified, `DEFAULT_REQUEST_TIMEOUT_MSEC` will be used as the timeout.
    optional<int64_t> Timeout;

    // If true, receiving a progress notification will reset the request timeout.
    // This is useful for long-running operations that send periodic progress updates.
    // Default: false
    optional<bool> ResetTimeoutOnProgress;

    // Maximum total time (in milliseconds) to wait for a response.
    // If exceeded, an MCP_Error with code `RequestTimeout` will be raised, regardless of progress
    // notifications. If not specified, there is no maximum total timeout.
    optional<int64_t> MaxTotalTimeout;
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

// Implements MCP protocol framing on top of a pluggable transport, including features like
// request/response linking, notifications, and progress.
template <typename SendRequestT, typename SendNotificationT, typename SendResultT> class Protocol {
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

    explicit Protocol(const optional<ProtocolOptions>& InOptions = nullopt) : m_Options(InOptions) {
        // Set up default handlers for cancelled notifications and progress notifications
        SetNotificationHandler(
            MTHD_NOTIFICATION_CANCELLED,
            [this](const NotificationBase& InNotification) -> future<void> {
                // Extract RequestID and reason from notification params
                if (InNotification.Params && InNotification.Params->contains(MSG_REQUEST_ID)) {
                    RequestID RequestID = (*InNotification.Params)[MSG_REQUEST_ID];

                    lock_guard<mutex> lock(m_HandlersMutex);
                    auto it = m_RequestHandlerAbortControllers.find(RequestID);
                    if (it != m_RequestHandlerAbortControllers.end()) {
                        // TODO: Fix External Ref: AbortSignal - Signal abort with reason
                        string Reason = InNotification.Params->value("reason", "Request cancelled");
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
        if (InNotification.Params->contains(MSG_DATA)) {
            ProgressData.Data = (*InNotification.Params)[MSG_DATA];
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

  public:
    // Attaches to the given transport, starts it, and starts listening for messages.
    // The Protocol object assumes ownership of the Transport, replacing any callbacks that have
    // already been set, and expects that it is the only user of the Transport instance going
    // forward.
    future<void> Connect(const shared_ptr<Transport>& InTransport) {
        m_Transport = InTransport;

        // Set up transport callbacks
        m_Transport->OnClose = [this]() { OnCloseInternal(); };
        m_Transport->OnError = [this](const exception& InError) { OnErrorInternal(InError); };
        m_Transport->OnMessage = [this](const JSON& InMessage,
                                        const optional<AuthInfo>& InAuthInfo) {
            // Parse JSON message and route to appropriate handler
            if (IsJSONRPCResponse(InMessage) || IsJSONRPCError(InMessage)) {
                if (IsJSONRPCResponse(InMessage)) {
                    ResponseBase Response;
                    Response.JSON_RPC = InMessage[MSG_JSON_RPC];
                    Response.ID = InMessage[MSG_ID];
                    Response.Result = InMessage[MSG_RESULT];
                    OnResponse(response);
                } else {
                    ErrorBase ErrorResponse;
                    ErrorResponse.JSON_RPC = InMessage[MSG_JSON_RPC];
                    ErrorResponse.ID = InMessage[MSG_ID];
                    ErrorResponse.Error.Code = InMessage[MSG_ERROR][MSG_CODE];
                    ErrorResponse.Error.Message = InMessage[MSG_ERROR][MSG_MESSAGE];
                    if (InMessage[MSG_ERROR].contains(MSG_DATA)) {
                        ErrorResponse.Error.Data = InMessage[MSG_ERROR][MSG_DATA];
                    }
                    OnResponse(error);
                }
            } else if (IsJSONRPCRequest(InMessage)) {
                RequestBase Request;
                Request.JSON_RPC = InMessage[MSG_JSON_RPC];
                Request.ID = InMessage[MSG_ID];
                Request.Method = InMessage[MSG_METHOD];
                if (InMessage.contains(MSG_PARAMS)) { Request.Params = InMessage[MSG_PARAMS]; }
                OnRequest(Request, InAuthInfo);
            } else if (IsJSONRPCNotification(InMessage)) {
                NotificationBase Notification;
                Notification.JSON_RPC = InMessage[MSG_JSON_RPC];
                Notification.Method = InMessage[MSG_METHOD];
                if (InMessage.contains(MSG_PARAMS)) { Notification.Params = InMessage[MSG_PARAMS]; }
                OnNotification(Notification);
            } else {
                OnErrorInternal(runtime_error("Unknown message type: " + InMessage.dump()));
            }
        };

        // Start transport
        return m_Transport->Start();
    }

    const shared_ptr<Transport>& GetTransport() const {
        return m_Transport;
    }

    // Closes the connection.
    future<void> Close() {
        if (m_Transport) { return m_Transport->Close(); }
        promise<void> Promise;
        Promise.set_value();
        return Promise.get_future();
    }

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

    // Asserts that a request handler has not already been set for the given method, in preparation
    // for a new one being automatically installed.
    void AssertCanSetRequestHandler(const string& InMethod) {
        lock_guard<mutex> Lock(m_HandlersMutex);
        if (m_RequestHandlers.find(InMethod) != m_RequestHandlers.end()) {
            throw runtime_error("A request handler for " + InMethod
                                + " already exists, which would be overridden");
        }
    }

    // Removes the notification handler for the given method.
    void RemoveNotificationHandler(const string& InMethod) {
        lock_guard<mutex> Lock(m_HandlersMutex);
        m_NotificationHandlers.erase(InMethod);
    }
};

template <typename T> T MergeCapabilities(const T& InBase, const T& InAdditional) {
    T Result = InBase;

    // Simple capability merging - merge JSON fields
    // This assumes T has a member that can be JSON-merged
    // The exact implementation would depend on the specific capability structure
    // TODO: Fix External Ref: Capabilities - Implement proper capability structure merging

    return Result;
}

MCP_NAMESPACE_END
