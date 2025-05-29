#pragma once

#include "../Core/Common.hpp"
#include "../Core/Types/JSON_RPC.hpp"
#include "../Transport/Transport.hpp"

namespace MCP {

// TODO: Fix External Ref: ZodType, ZodObject, ZodLiteral - Schema validation library
// TODO: Fix External Ref: AuthInfo - Authentication information types
// TODO: Fix External Ref: Various MCP types (Request, Notification, Result, etc.)

// Forward declarations for external types that need proper implementation
struct AuthInfo;
struct Request;
struct Notification;
struct Result;
struct ClientCapabilities;
struct ServerCapabilities;
struct Progress;
struct RequestMeta;

/**
 * Callback for progress notifications.
 */
using ProgressCallback = function<void(const Progress&)>;

/**
 * Additional initialization options.
 */
struct ProtocolOptions {
  /**
   * Whether to restrict emitted requests to only those that the remote side has indicated that they can handle, through their advertised capabilities.
   *
   * Note that this DOES NOT affect checking of _local_ side capabilities, as it is considered a logic error to mis-specify those.
   *
   * Currently this defaults to false, for backwards compatibility with SDK versions that did not advertise capabilities correctly. In future, this will default to true.
   */
    optional<bool> EnforceStrictCapabilities;
};

/**
 * The default request timeout, in milliseconds.
 */
constexpr int64_t DEFAULT_REQUEST_TIMEOUT_MSEC = 60000;

/**
 * Options that can be given per request.
 */
struct RequestOptions : public Transport::TransportSendOptions {
    /**
     * If set, requests progress notifications from the remote end (if supported). When progress notifications are received, this callback will be invoked.
     */
    optional<ProgressCallback> OnProgress;

    /**
     * Can be used to cancel an in-flight request. This will cause an AbortError to be raised from request().
     */
    optional<AbortSignal> Signal;

    /**
     * A timeout (in milliseconds) for this request. If exceeded, an McpError with code `RequestTimeout` will be raised from request().
     *
     * If not specified, `DEFAULT_REQUEST_TIMEOUT_MSEC` will be used as the timeout.
     */
    optional<int64_t> Timeout;

    /**
     * If true, receiving a progress notification will reset the request timeout.
     * This is useful for long-running operations that send periodic progress updates.
     * Default: false
     */
    optional<bool> ResetTimeoutOnProgress;

    /**
     * Maximum total time (in milliseconds) to wait for a response.
     * If exceeded, an McpError with code `RequestTimeout` will be raised, regardless of progress notifications.
     * If not specified, there is no maximum total timeout.
     */
    optional<int64_t> MaxTotalTimeout;

    /**
     * May be used to indicate to the transport which incoming request to associate this outgoing request with.
     */
    optional<RequestID> RelatedRequestID;

    /**
     * Resumption token for transport-level message resumption.
     */
    optional<string> ResumptionToken;

    /**
     * Callback for when a resumption token is provided.
     */
    optional<function<void(const string&)>> OnResumptionToken;
};

/**
 * Options that can be given per notification.
 */
struct NotificationOptions {
    /**
     * May be used to indicate to the transport which incoming request to associate this outgoing notification with.
     */
    optional<RequestID> RelatedRequestID;
};

/**
 * Extra data given to request handlers.
 */
template<typename SendRequestT, typename SendNotificationT>
struct RequestHandlerExtra {
    /**
     * An abort signal used to communicate if the request was cancelled from the sender's side.
     */
    AbortSignal Signal;

    /**
     * Information about a validated access token, provided to request handlers.
     */
    optional<AuthInfo> AuthInfo;

    /**
     * The session ID from the transport, if available.
     */
    optional<string> SessionID;

    /**
     * Metadata from the original request.
     */
    optional<RequestMeta> Meta;

    /**
     * The JSON-RPC ID of the request being handled.
     * This can be useful for tracking or logging purposes.
     */
    RequestID RequestID;

    /**
     * Sends a notification that relates to the current request being handled.
     *
     * This is used by certain transports to correctly associate related messages.
     */
    function<future<void>(const SendNotificationT&)> SendNotification;

    /**
     * Sends a request that relates to the current request being handled.
     *
     * This is used by certain transports to correctly associate related messages.
     */
    template<typename ResultT, typename SchemaT>
    function<future<ResultT>(const SendRequestT&, const SchemaT&, const optional<RequestOptions>&)> SendRequest;
};

/**
 * Information about a request's timeout state
 */
struct TimeoutInfo {
    // TODO: Fix External Ref: Timer/Timeout mechanism
    uint64_t TimeoutId;
    chrono::steady_clock::time_point StartTime;
    int64_t Timeout;
    optional<int64_t> MaxTotalTimeout;
    bool ResetTimeoutOnProgress;
    function<void()> OnTimeout;
};

/**
 * Implements MCP protocol framing on top of a pluggable transport, including
 * features like request/response linking, notifications, and progress.
 */
template<typename SendRequestT, typename SendNotificationT, typename SendResultT>
class Protocol {
private:
    shared_ptr<Transport> Transport_;
    atomic<int64_t> RequestMessageId_{0};

    unordered_map<string, function<future<SendResultT>(const JSON_RPC_Request&, const RequestHandlerExtra<SendRequestT, SendNotificationT>&)>> RequestHandlers_;
    unordered_map<RequestID, AbortSignal> RequestHandlerAbortControllers_;
    unordered_map<string, function<future<void>(const JSON_RPC_Notification&)>> NotificationHandlers_;
    unordered_map<int64_t, function<void(const variant<JSON_RPC_Response, McpError>&)>> ResponseHandlers_;
    unordered_map<int64_t, ProgressCallback> ProgressHandlers_;
    unordered_map<int64_t, TimeoutInfo> TimeoutInfo_;

    mutex HandlersMutex_;
    optional<ProtocolOptions> Options_;

public:
  /**
   * Callback for when the connection is closed for any reason.
   *
   * This is invoked when close() is called as well.
   */
    function<void()> OnClose;

  /**
   * Callback for when an error occurs.
   *
   * Note that errors are not necessarily fatal; they are used for reporting any kind of exceptional condition out of band.
   */
    function<void(const exception&)> OnError;

  /**
   * A handler to invoke for any request types that do not have their own handler installed.
   */
    function<future<SendResultT>(const Request&)> FallbackRequestHandler;

  /**
   * A handler to invoke for any notification types that do not have their own handler installed.
   */
    function<future<void>(const Notification&)> FallbackNotificationHandler;

    explicit Protocol(const optional<ProtocolOptions>& options = nullopt) : Options_(options) {
        // Set up default handlers for cancelled notifications and progress notifications
        SetNotificationHandler("notifications/cancelled", [this](const JSON_RPC_Notification& notification) -> future<void> {
            // Extract RequestID and reason from notification params
            if (notification.params && notification.params->contains("requestId")) {
                RequestID requestId = (*notification.params)["requestId"];

                lock_guard<mutex> lock(HandlersMutex_);
                auto it = RequestHandlerAbortControllers_.find(requestId);
                if (it != RequestHandlerAbortControllers_.end()) {
                    // TODO: Fix External Ref: AbortSignal - Signal abort with reason
                    string reason = notification.params->value("reason", "Request cancelled");
                    // it->second.abort(reason);
                }
            }
            promise<void> p;
            p.set_value();
            return p.get_future();
        });

        SetNotificationHandler("notifications/progress", [this](const JSON_RPC_Notification& notification) -> future<void> {
            OnProgress(notification);
            promise<void> p;
            p.set_value();
            return p.get_future();
        });

        // Automatic pong by default for ping requests
        SetRequestHandler("ping", [](const JSON_RPC_Request& request, const RequestHandlerExtra<SendRequestT, SendNotificationT>& extra) -> future<SendResultT> {
            // Return empty object as ping response
            promise<SendResultT> p;
            p.set_value(SendResultT{});
            return p.get_future();
        });
    }

private:
    void OnProgress(const JSON_RPC_Notification& notification) {
        if (!notification.params || !notification.params->contains("progressToken")) {
            OnErrorInternal(runtime_error("Received a progress notification without progressToken: " + notification.method));
            return;
        }

        int64_t progressToken = (*notification.params)["progressToken"];

        ProgressCallback handler;
        {
            lock_guard<mutex> lock(HandlersMutex_);
            auto it = ProgressHandlers_.find(progressToken);
            if (it != ProgressHandlers_.end()) {
                handler = it->second;
            }
        }

        if (!handler) {
            OnErrorInternal(runtime_error("Received a progress notification for an unknown token: " + to_string(progressToken)));
            return;
        }

        auto responseHandler = ResponseHandlers_.find(progressToken);
        auto timeoutIt = TimeoutInfo_.find(progressToken);

        if (timeoutIt != TimeoutInfo_.end() && responseHandler != ResponseHandlers_.end() && timeoutIt->second.ResetTimeoutOnProgress) {
            try {
                ResetTimeout(progressToken);
            } catch (const McpError& error) {
                responseHandler->second(error);
                return;
            }
        }

        // Extract the params excluding progressToken (equivalent to TypeScript destructuring)
        Progress progressData;
        progressData.progressToken = progressToken;

        // Extract the params excluding progressToken (equivalent to TypeScript destructuring)
        if (notification.params->contains("data")) {
            progressData.data = (*notification.params)["data"];
        }

        handler(progressData);
    }

    void SetupTimeout(int64_t messageId, int64_t timeout, const optional<int64_t>& maxTotalTimeout,
                     function<void()> onTimeout, bool resetTimeoutOnProgress = false) {
        TimeoutInfo info{
            .TimeoutId = static_cast<uint64_t>(messageId),
            .StartTime = chrono::steady_clock::now(),
            .Timeout = timeout,
            .MaxTotalTimeout = maxTotalTimeout,
            .ResetTimeoutOnProgress = resetTimeoutOnProgress,
            .OnTimeout = move(onTimeout)
        };

        lock_guard<mutex> lock(HandlersMutex_);
        TimeoutInfo_[messageId] = move(info);

        // TODO: Fix External Ref: Timer/Timeout mechanism - Set up actual timer that calls onTimeout after timeout milliseconds
    }

    bool ResetTimeout(int64_t messageId) {
        lock_guard<mutex> lock(HandlersMutex_);
        auto it = TimeoutInfo_.find(messageId);
        if (it == TimeoutInfo_.end()) return false;

        auto& info = it->second;
        auto totalElapsed = chrono::duration_cast<chrono::milliseconds>(
            chrono::steady_clock::now() - info.StartTime).count();

        if (info.MaxTotalTimeout && totalElapsed >= *info.MaxTotalTimeout) {
            TimeoutInfo_.erase(it);
            throw McpError(ErrorCode::RequestTimeout, "Maximum total timeout exceeded",
                          JSON{{"maxTotalTimeout", *info.MaxTotalTimeout}, {"totalElapsed", totalElapsed}});
        }

        // TODO: Fix External Ref: Timer/Timeout mechanism - Reset actual timer
        return true;
    }

    void CleanupTimeout(int64_t messageId) {
        lock_guard<mutex> lock(HandlersMutex_);
        auto it = TimeoutInfo_.find(messageId);
        if (it != TimeoutInfo_.end()) {
            // TODO: Fix External Ref: Timer/Timeout mechanism - Cancel actual timer
            TimeoutInfo_.erase(it);
        }
    }

    void OnCloseInternal() {
        unordered_map<int64_t, function<void(const variant<JSON_RPC_Response, McpError>&)>> responseHandlers;

        {
            lock_guard<mutex> lock(HandlersMutex_);
            responseHandlers = move(ResponseHandlers_);
            ResponseHandlers_.clear();
            ProgressHandlers_.clear();
        }

        Transport_.reset();

        if (OnClose) {
            OnClose();
        }

        McpError error(ErrorCode::ConnectionClosed, "Connection closed");
        for (const auto& [id, handler] : responseHandlers) {
            handler(error);
        }
    }

    void OnErrorInternal(const exception& error) {
        if (OnError) {
            OnError(error);
        }
    }

    void OnNotification(const JSON_RPC_Notification& notification) {
        function<future<void>(const JSON_RPC_Notification&)> handler;

        {
            lock_guard<mutex> lock(HandlersMutex_);
            auto it = NotificationHandlers_.find(notification.method);
            if (it != NotificationHandlers_.end()) {
                handler = it->second;
            }
        }

        if (!handler && FallbackNotificationHandler) {
            // Convert JSON_RPC_Notification to Notification type
            Notification notif;
            notif.method = notification.method;
            notif.params = notification.params;

            handler = [this, notif](const JSON_RPC_Notification&) -> future<void> {
                return FallbackNotificationHandler(notif);
            };
        }

        // Ignore notifications not being subscribed to.
        if (!handler) {
            return;
        }

        // Execute handler asynchronously and catch errors
        async(launch::async, [this, handler, notification]() {
            try {
                auto future = handler(notification);
                future.wait();
            } catch (const exception& e) {
                OnErrorInternal(runtime_error("Uncaught error in notification handler: " + string(e.what())));
            }
        });
    }

    void OnRequest(const JSON_RPC_Request& request, const optional<AuthInfo>& authInfo = nullopt) {
        function<future<SendResultT>(const JSON_RPC_Request&, const RequestHandlerExtra<SendRequestT, SendNotificationT>&)> handler;

        {
            lock_guard<mutex> lock(HandlersMutex_);
            auto it = RequestHandlers_.find(request.method);
            if (it != RequestHandlers_.end()) {
                handler = it->second;
            }
        }

        if (!handler && FallbackRequestHandler) {
            // Convert and use fallback handler
            Request req;
            req.method = request.method;
            req.params = request.params;

            handler = [this, req](const JSON_RPC_Request&, const RequestHandlerExtra<SendRequestT, SendNotificationT>&) -> future<SendResultT> {
                return FallbackRequestHandler(req);
            };
        }

        if (!handler) {
            // Send method not found error
            JSON_RPC_Error errorResponse;
            errorResponse.jsonrpc = "2.0";
            errorResponse.id = request.id;
            errorResponse.error.code = static_cast<int32_t>(ErrorCode::MethodNotFound);
            errorResponse.error.message = "Method not found";

            if (Transport_) {
                // TODO: Fix External Ref: Transport - Send error response via transport
                // Transport_->Send(errorResponse);
            }
            return;
        }

        // TODO: Fix External Ref: AbortSignal - Create proper AbortSignal
        AbortSignal abortSignal;

        {
            lock_guard<mutex> lock(HandlersMutex_);
            RequestHandlerAbortControllers_[request.id] = abortSignal;
        }

        RequestHandlerExtra<SendRequestT, SendNotificationT> extra;
        extra.Signal = abortSignal;
        extra.SessionID = Transport_ ? Transport_->SessionID : nullopt;
        extra.AuthInfo = authInfo;
        extra.Meta = (request.params && request.params->contains("_meta"))
                     ? optional<RequestMeta>{RequestMeta{}} // Basic extraction
                     : nullopt;
        extra.RequestID = request.id;

        extra.SendNotification = [this, requestId = request.id](const SendNotificationT& notification) -> future<void> {
            return Notification(notification, NotificationOptions{.RelatedRequestID = requestId});
        };

        extra.SendRequest = [this, requestId = request.id]<typename ResultT, typename SchemaT>(
            const SendRequestT& req, const SchemaT& schema, const optional<RequestOptions>& opts) -> future<ResultT> {
            RequestOptions options = opts.value_or(RequestOptions{});
            options.RelatedRequestID = requestId;
            return Request<ResultT>(req, options);
        };

        // Execute handler asynchronously
        async(launch::async, [this, handler, request, extra]() {
            try {
                auto resultFuture = handler(request, extra);
                auto result = resultFuture.get();

                // TODO: Fix External Ref: AbortSignal - Check if request was aborted
                // if (!extra.Signal.IsAborted()) {
                    if (Transport_) {
                        JSON_RPC_Response response;
                        response.jsonrpc = "2.0";
                        response.id = request.id;
                        response.result = JSON{}; // Convert SendResultT to JSON

                        // TODO: Fix External Ref: Transport - Send response via transport
                        // Transport_->Send(response);
                    }
                // }
            } catch (const exception& e) {
                // TODO: Fix External Ref: AbortSignal - Check if request was aborted
                // if (!extra.Signal.IsAborted()) {
                    // Send error response
                    JSON_RPC_Error errorResponse;
                    errorResponse.jsonrpc = "2.0";
                    errorResponse.id = request.id;
                    errorResponse.error.code = static_cast<int32_t>(ErrorCode::InternalError);
                    errorResponse.error.message = e.what();

                    if (Transport_) {
                        // TODO: Fix External Ref: Transport - Send error response via transport
                        // Transport_->Send(errorResponse);
                    }
                // }
            }

            // Cleanup
            {
                lock_guard<mutex> lock(HandlersMutex_);
                RequestHandlerAbortControllers_.erase(request.id);
            }
        });
    }

    void OnResponse(const variant<JSON_RPC_Response, JSON_RPC_Error>& response) {
        RequestID responseId;

        if (holds_alternative<JSON_RPC_Response>(response)) {
            responseId = get<JSON_RPC_Response>(response).id;
        } else {
            responseId = get<JSON_RPC_Error>(response).id;
        }

        // Convert RequestID to int64_t for message correlation
        int64_t messageId = 0;
        if (holds_alternative<int64_t>(responseId)) {
            messageId = get<int64_t>(responseId);
        } else if (holds_alternative<string>(responseId)) {
            // Handle string IDs by parsing to int
            try {
                messageId = stoll(get<string>(responseId));
            } catch (const exception&) {
                OnErrorInternal(runtime_error("Cannot correlate response with string ID: " + get<string>(responseId)));
                return;
            }
        }

        function<void(const variant<JSON_RPC_Response, McpError>&)> handler;

        {
            lock_guard<mutex> lock(HandlersMutex_);
            auto it = ResponseHandlers_.find(messageId);
            if (it != ResponseHandlers_.end()) {
                handler = it->second;
                ResponseHandlers_.erase(it);
                ProgressHandlers_.erase(messageId);
            }
        }

        if (!handler) {
            OnErrorInternal(runtime_error("Received a response for an unknown message ID"));
            return;
        }

        CleanupTimeout(messageId);

        if (holds_alternative<JSON_RPC_Response>(response)) {
            handler(get<JSON_RPC_Response>(response));
        } else {
            const auto& errorResp = get<JSON_RPC_Error>(response);
            McpError error(static_cast<ErrorCode>(errorResp.error.code),
                          errorResp.error.message, errorResp.error.data);
            handler(error);
        }
    }

public:
    /**
     * Attaches to the given transport, starts it, and starts listening for messages.
     *
     * The Protocol object assumes ownership of the Transport, replacing any callbacks that have already been set, and expects that it is the only user of the Transport instance going forward.
     */
    future<void> Connect(shared_ptr<Transport> transport) {
        Transport_ = transport;

        // Set up transport callbacks
        Transport_->OnClose = [this]() { OnCloseInternal(); };
        Transport_->OnError = [this](const exception& e) { OnErrorInternal(e); };
        Transport_->OnMessage = [this](const JSON& message, const optional<AuthInfo>& extra) {
            // Parse JSON message and route to appropriate handler
            if (IsJSONRPCResponse(message) || IsJSONRPCError(message)) {
                if (IsJSONRPCResponse(message)) {
                    JSON_RPC_Response response;
                    response.jsonrpc = message["jsonrpc"];
                    response.id = message["id"];
                    response.result = message["result"];
                    OnResponse(response);
                } else {
                    JSON_RPC_Error error;
                    error.jsonrpc = message["jsonrpc"];
                    error.id = message["id"];
                    error.error.code = message["error"]["code"];
                    error.error.message = message["error"]["message"];
                    if (message["error"].contains("data")) {
                        error.error.data = message["error"]["data"];
                    }
                    OnResponse(error);
                }
            } else if (IsJSONRPCRequest(message)) {
                JSON_RPC_Request request;
                request.jsonrpc = message["jsonrpc"];
                request.id = message["id"];
                request.method = message["method"];
                if (message.contains("params")) {
                    request.params = message["params"];
                }
                OnRequest(request, extra);
            } else if (IsJSONRPCNotification(message)) {
                JSON_RPC_Notification notification;
                notification.jsonrpc = message["jsonrpc"];
                notification.method = message["method"];
                if (message.contains("params")) {
                    notification.params = message["params"];
                }
                OnNotification(notification);
            } else {
                OnErrorInternal(runtime_error("Unknown message type: " + message.dump()));
            }
        };

        // Start transport
        return Transport_->Start();
    }

    shared_ptr<Transport> GetTransport() const {
        return Transport_;
  }

  /**
   * Closes the connection.
   */
    future<void> Close() {
        if (Transport_) {
            return Transport_->Close();
        }
        promise<void> p;
        p.set_value();
        return p.get_future();
    }

protected:
  /**
   * A method to check if a capability is supported by the remote side, for the given method to be called.
   *
   * This should be implemented by subclasses.
   */
    virtual void AssertCapabilityForMethod(const string& method) = 0;

  /**
   * A method to check if a notification is supported by the local side, for the given method to be sent.
   *
   * This should be implemented by subclasses.
   */
    virtual void AssertNotificationCapability(const string& method) = 0;

  /**
   * A method to check if a request handler is supported by the local side, for the given method to be handled.
   *
   * This should be implemented by subclasses.
   */
    virtual void AssertRequestHandlerCapability(const string& method) = 0;

public:
  /**
   * Sends a request and wait for a response.
   *
     * Do not use this method to emit notifications! Use Notification() instead.
     */
    template<typename ResultT>
    future<ResultT> Request(const SendRequestT& request, const optional<RequestOptions>& options = nullopt) {
        if (!Transport_) {
            throw runtime_error("Not connected");
        }

        if (Options_ && Options_->EnforceStrictCapabilities.value_or(false)) {
            AssertCapabilityForMethod(request.method);
        }

        if (options && options->Signal && options->Signal->IsAborted()) {
            throw runtime_error("Request was aborted");
        }

        int64_t messageId = RequestMessageId_++;

        JSON_RPC_Request jsonRpcRequest;
        jsonRpcRequest.jsonrpc = "2.0";
        jsonRpcRequest.id = messageId;
        jsonRpcRequest.method = request.method;
        jsonRpcRequest.params = request.params;

        promise<ResultT> resultPromise;
        auto resultFuture = resultPromise.get_future();

        if (options && options->OnProgress) {
            lock_guard<mutex> lock(HandlersMutex_);
            ProgressHandlers_[messageId] = *options->OnProgress;

            if (!jsonRpcRequest.params) {
                jsonRpcRequest.params = JSON::object();
            }
            if (!jsonRpcRequest.params->contains("_meta")) {
                (*jsonRpcRequest.params)["_meta"] = JSON::object();
            }
            (*jsonRpcRequest.params)["_meta"]["progressToken"] = messageId;
        }

        auto cancel = [this, messageId](const string& reason) {
            {
                lock_guard<mutex> lock(HandlersMutex_);
                ResponseHandlers_.erase(messageId);
                ProgressHandlers_.erase(messageId);
            }
            CleanupTimeout(messageId);

            // Send cancellation notification
            if (Transport_) {
                JSON_RPC_Notification cancelNotification;
                cancelNotification.jsonrpc = "2.0";
                cancelNotification.method = "notifications/cancelled";
                cancelNotification.params = JSON::object();
                (*cancelNotification.params)["requestId"] = messageId;
                (*cancelNotification.params)["reason"] = reason;

                // TODO: Fix External Ref: Transport - Send cancellation notification
                // Transport_->Send(cancelNotification);
            }
        };

        {
            lock_guard<mutex> lock(HandlersMutex_);
            ResponseHandlers_[messageId] = [promise = move(resultPromise), cancel, options](const variant<JSON_RPC_Response, McpError>& response) mutable {
                // TODO: Fix External Ref: AbortSignal - Check if request was aborted
                // if (options && options->Signal && options->Signal->IsAborted()) {
                //     return;
                // }

                if (holds_alternative<McpError>(response)) {
                    promise.set_exception(make_exception_ptr(get<McpError>(response)));
                    return;
                }

                try {
                    const auto& jsonResponse = get<JSON_RPC_Response>(response);
                    // Parse response result as ResultT using simple JSON conversion
                    if constexpr (std::is_same_v<ResultT, JSON>) {
                        promise.set_value(jsonResponse.result);
                    } else {
                        // Simple JSON to type conversion
                        ResultT result = jsonResponse.result.get<ResultT>();
                        promise.set_value(result);
                    }
                } catch (const exception& e) {
                    promise.set_exception(current_exception());
                }
            };
        }

        // TODO: Fix External Ref: AbortSignal - Set up signal cancellation callback
        // if (options && options->Signal) {
        //     options->Signal->OnAbort([cancel]() { cancel("Request was cancelled"); });
        // }

        int64_t timeout = options && options->Timeout ? *options->Timeout : DEFAULT_REQUEST_TIMEOUT_MSEC;
        auto timeoutHandler = [cancel]() { cancel("Request timed out"); };

        SetupTimeout(messageId, timeout,
                    options ? options->MaxTotalTimeout : nullopt,
                    timeoutHandler,
                    options ? options->ResetTimeoutOnProgress.value_or(false) : false);

        // Send request via transport
        try {
            JSON requestJson = {
                {"jsonrpc", jsonRpcRequest.jsonrpc},
                {"id", jsonRpcRequest.id},
                {"method", jsonRpcRequest.method}
            };
            if (jsonRpcRequest.params) {
                requestJson["params"] = *jsonRpcRequest.params;
            }

            Transport::TransportSendOptions transportOptions;
            if (options) {
                transportOptions.RelatedRequestID = options->RelatedRequestID;
                transportOptions.ResumptionToken = options->ResumptionToken;
                transportOptions.OnResumptionToken = options->OnResumptionToken;
            }

            // TODO: Fix External Ref: Transport - Send request message
            // Transport_->Send(requestJson, transportOptions);
        } catch (const exception& e) {
            CleanupTimeout(messageId);
            throw;
        }

        return resultFuture;
    }

  /**
   * Emits a notification, which is a one-way message that does not expect a response.
   */
    future<void> Notification(const SendNotificationT& notification, const optional<NotificationOptions>& options = nullopt) {
        if (!Transport_) {
            throw runtime_error("Not connected");
        }

        AssertNotificationCapability(notification.method);

        JSON_RPC_Notification jsonRpcNotification;
        jsonRpcNotification.jsonrpc = "2.0";
        jsonRpcNotification.method = notification.method;
        jsonRpcNotification.params = notification.params;

        JSON notificationJson = {
            {"jsonrpc", jsonRpcNotification.jsonrpc},
            {"method", jsonRpcNotification.method}
        };
        if (jsonRpcNotification.params) {
            notificationJson["params"] = *jsonRpcNotification.params;
        }

        Transport::TransportSendOptions transportOptions;
        if (options) {
            transportOptions.RelatedRequestID = options->RelatedRequestID;
        }

        // TODO: Fix External Ref: Transport - Send notification via transport
        // return Transport_->Send(notificationJson, transportOptions);

        promise<void> p;
        p.set_value();
        return p.get_future();
    }

  /**
   * Registers a handler to invoke when this protocol object receives a request with the given method.
   *
   * Note that this will replace any previous request handler for the same method.
   */
    void SetRequestHandler(const string& method, function<future<SendResultT>(const JSON_RPC_Request&, const RequestHandlerExtra<SendRequestT, SendNotificationT>&)> handler) {
        AssertRequestHandlerCapability(method);

        lock_guard<mutex> lock(HandlersMutex_);
        RequestHandlers_[method] = move(handler);
    }

    /**
     * Registers a handler to invoke when this protocol object receives a notification with the given method.
     *
     * Note that this will replace any previous notification handler for the same method.
     */
    void SetNotificationHandler(const string& method, function<future<void>(const JSON_RPC_Notification&)> handler) {
        lock_guard<mutex> lock(HandlersMutex_);
        NotificationHandlers_[method] = move(handler);
    }

  /**
   * Removes the request handler for the given method.
   */
    void RemoveRequestHandler(const string& method) {
        lock_guard<mutex> lock(HandlersMutex_);
        RequestHandlers_.erase(method);
  }

  /**
   * Asserts that a request handler has not already been set for the given method, in preparation for a new one being automatically installed.
   */
    void AssertCanSetRequestHandler(const string& method) {
        lock_guard<mutex> lock(HandlersMutex_);
        if (RequestHandlers_.find(method) != RequestHandlers_.end()) {
            throw runtime_error("A request handler for " + method + " already exists, which would be overridden");
    }
  }

  /**
   * Removes the notification handler for the given method.
   */
    void RemoveNotificationHandler(const string& method) {
        lock_guard<mutex> lock(HandlersMutex_);
        NotificationHandlers_.erase(method);
    }
};

template<typename T>
T MergeCapabilities(const T& base, const T& additional) {
    T result = base;

    // Simple capability merging - merge JSON fields
    // This assumes T has a member that can be JSON-merged
    // The exact implementation would depend on the specific capability structure
    // TODO: Fix External Ref: Capabilities - Implement proper capability structure merging

    return result;
}

} // namespace MCP
