#pragma once

#include "Communication/Transport/ITransport.h"
#include "Core/Types/Initialization.h"
#include "Macros.h"
#include "NotificationBase.h"
#include "RequestBase.h"
#include "ResponseBase.h"
#include "Utilities/Async/MCPTask.h"

MCP_NAMESPACE_BEGIN

// TODO: @HalcyonOmega Begin Direct Translation Code
// The default request timeout, in milliseconds.
static constexpr const int64_t DEFAULT_REQUEST_TIMEOUT_MSEC = 60000;

// Extra data given to request handlers.
struct RequestHandlerExtra {
    // An abort signal used to communicate if the request was cancelled from the sender's side.
    AbortSignal Signal;

    // Information about a validated access token, provided to request handlers.
    optional<AuthInfo> AuthInfo;

    // The session ID from the transport, if available.
    optional<string> SessionID;

    // Metadata from the original request.
    optional<RequestBase::RequestParams::RequestParamsMeta> Meta;

    // The JSON-RPC ID of the request being handled.
    // This can be useful for tracking or logging purposes.
    RequestID RequestID;
};
// TODO: @HalcyonOmega End Direct Translation Code

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
    virtual string GetProtocolVersion() const = 0;
    virtual MCPTask_Void Connect() = 0;
    virtual MCPTask_Void Disconnect() = 0;

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

    void HandleIncomingMessage(const MessageBase& InMessage);
    virtual MCPTask<ResponseBase> HandleRequest(const RequestBase& InRequest) = 0;
    void HandleResponse(const ResponseBase& InResponse);
    virtual MCPTask_Void HandleNotification(const NotificationBase& InNotification) = 0;
    void RegisterPendingRequest(const string& InRequestID,
                                function<void(const ResponseBase&)> InHandler);

    /* Configuration */

    // Protocol configuration with sensible defaults
    struct Config {
        string ProtocolVersion = LATEST_PROTOCOL_VERSION;
        chrono::milliseconds RequestTimeout{30000};
        chrono::milliseconds DefaultTimeout{10000};
        bool AllowBatchRequests{true};
        size_t MaxConcurrentRequests{100};

        /**
         * Whether to restrict emitted requests to only those that the remote side has
         * indicated that they can handle, through their advertised capabilities.
         *
         * Note that this DOES NOT affect checking of _local_ side capabilities, as it is
         * considered a logic error to mis-specify those.
         *
         * Currently this defaults to false, for backwards compatibility with SDK versions
         * that did not advertise capabilities correctly. In future, this will default to
         * true.
         */
        optional<bool> EnforceStrictCapabilities = false;
    };

    void SetConfig(const Config& InConfig) {
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

  public:
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
    shared_ptr<ITransport> m_Transport;
    RequestID m_RequestRequestID{0};

    unordered_map<string,
                  function<future<ResponseBase>(const RequestBase&, const RequestHandlerExtra&)>>
        m_RequestHandlers;
    unordered_map<RequestID, AbortSignal> m_RequestHandlerAbortControllers;
    unordered_map<string, function<future<void>(const NotificationBase&)>> m_NotificationHandlers;
    unordered_map<int64_t, function<void(const variant<ResponseBase, ErrorBase>&)>>
        m_ResponseHandlers;
    unordered_map<int64_t, ProgressCallback> m_ProgressHandlers;

    mutex m_HandlersMutex;
    optional<Config> m_Options;

    // TODO: End @HalcyonOmega From direct translation - is this needed?

  private:
    Config m_Config;
    ProtocolCallbacks m_Callbacks;

    RequestID m_NextRequestID;
    queue<function<void(const ResponseBase&)>> m_PendingRequests;
    mutex m_PendingRequestsMutex;
};

MCP_NAMESPACE_END