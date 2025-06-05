#include "Session.h" // Only direct include needed for public headers from this .cpp file

MCP_NAMESPACE_BEGIN

Session::Session(std::shared_ptr<Transport> transport, const ClientCapabilities& clientCaps,
                 const Implementation& clientInfo)
    : m_Transport(transport), m_ClientCapabilities(clientCaps), m_ClientInfo(clientInfo),
      m_State(SessionState::Uninitialized) {
    if (m_Transport) {
        m_Transport->SetOnMessage([this](const std::string& message, const AuthInfo* authInfo) {
            // AuthInfo is defined in Transport.h, which is included by Session.h
            (void)authInfo;
            this->HandleTransportMessage(message);
        });
        // TODO: Set up other transport callbacks like SetOnError, SetOnClose using m_Transport
    }
}

Session::~Session() {
    if (m_State != SessionState::Shutdown && m_State != SessionState::Error) {
        // Consider logging a warning if shutdown was not explicit or perform automatic cleanup
    }
    // m_Transport (shared_ptr) is managed automatically
}

void Session::HandleTransportMessage(const std::string& message) {
    // Placeholder - to be implemented
    // Example: Parse message using JSON alias from Core.h (via Session.h)
    // auto json_message = JSON::parse(message, nullptr, false);
    // if (json_message.is_discarded()) { /* handle error */ return; }
    // ... logic to dispatch based on message type (request, response, notification) ...
    (void)message; // Suppress unused parameter warning
}

void Session::ProcessInitializeResult(const InitializeResult& result) {
    // Placeholder - to be implemented
    // Example:
    // m_ServerCapabilities = result.capabilities;
    // m_ServerInfo = result.serverInfo;
    // m_NegotiatedProtocolVersion = result.protocolVersion;
    // m_State = SessionState::Initialized;
    // SendInitializedNotification();
    // if (m_InitializeCallback) { m_InitializeCallback(std::nullopt); }
    (void)result; // Suppress unused parameter warning
}

void Session::SendInitializedNotification() {
    // Placeholder - to be implemented
    // Example:
    // InitializedNotification notification;
    // JSON json_notification = notification; // Requires to_json for InitializedNotification
    // if (m_Transport) { m_Transport->Send(json_notification.dump()); }
}

// TODO: Implement a more robust request ID generation mechanism (e.g., UUID)
// For now, a simple atomic counter will suffice for basic unique ID generation within a session.
static std::atomic<int> s_NextRequestID = 1;

std::string GenerateRequestID() {
    return std::to_string(s_NextRequestID++);
}

void Session::Initialize(std::function<void(const std::optional<MCP_Error>&)> callback) {
    if (m_State != SessionState::Uninitialized) {
        if (callback) {
            // TODO: Define proper error codes/messages in ErrorConstants.h or similar
            callback(
                MCP_Error(-32000, "Session already initialized or initializing.", std::nullopt));
        }
        return;
    }

    m_InitializeCallback = callback;
    m_State = SessionState::Initializing;

    InitializeRequest request;
    request.params.protocolVersion = LATEST_PROTOCOL_VERSION; // From Constants.h via Core.h
    request.params.capabilities = m_ClientCapabilities;
    request.params.clientInfo = m_ClientInfo;

    // Assign a unique ID to the request object itself if the struct supports it,
    // or manage it externally for JSON-RPC packaging.
    // The InitializeRequest struct in InitializeSchemas.h inherits from Request, which has an 'id'
    // member.
    request.id = GenerateRequestID();

    // Store this request ID to match with the response
    // TODO: Implement a proper pending requests map:
    // m_PendingRequests[request.id.to_string()] = m_InitializeCallback; // Assuming RequestID can
    // be converted to string for map key

    // Serialize the request to JSON
    // This assumes InitializeRequest and its members will have to_json support
    // or we construct the JSON manually. For now, let's assume direct construction
    // as an example, and to highlight where to_json methods are needed.

    JSON jsonRequest;
    jsonRequest[MSG_KEY_JSON_RPC] = MSG_KEY_JSON_RPC_VERSION; // "2.0"
    jsonRequest[MSG_KEY_ID] =
        request.id.get<std::string>(); // Assuming RequestID is variant<string, int> and we use
                                       // string here
    jsonRequest[MSG_KEY_METHOD] = request.method; // Should be "initialize" (MTHD_INITIALIZE)

    JSON paramsJson;
    paramsJson[MSG_KEY_PROTOCOL_VERSION] = request.params.protocolVersion;

    // Manual serialization for ClientCapabilities (example - ideally ClientCapabilities has
    // to_json)
    JSON clientCapabilitiesJson;
    if (request.params.capabilities.roots) {
        JSON rootsJson;
        if (request.params.capabilities.roots->listChanged) {
            rootsJson[MSG_KEY_LIST_CHANGED] = *request.params.capabilities.roots->listChanged;
        }
        clientCapabilitiesJson[MSG_KEY_ROOTS] = rootsJson; // MSG_KEY_ROOTS needed in Constants.h
    }
    if (request.params.capabilities.sampling) {
        // Sampling is optional<JSON> in ClientSchemas.h, use it if present
        clientCapabilitiesJson[MSG_KEY_SAMPLING] =
            *request.params.capabilities.sampling; // MSG_KEY_SAMPLING needed
    }
    // Add other capabilities if present...
    paramsJson[MSG_KEY_CAPABILITIES] = clientCapabilitiesJson;

    JSON clientInfoJson;
    clientInfoJson[MSG_KEY_NAME] = request.params.clientInfo.name;
    clientInfoJson[MSG_KEY_VERSION] = request.params.clientInfo.version;
    paramsJson[MSG_KEY_CLIENT_INFO] = clientInfoJson;

    jsonRequest[MSG_KEY_PARAMS] = paramsJson;

    if (m_Transport) {
        m_Transport->Send(jsonRequest.dump());
    } else {
        if (m_InitializeCallback) {
            m_State = SessionState::Error;
            // TODO: Define proper error codes/messages
            m_InitializeCallback(MCP_Error(-32001, "Transport not available.", std::nullopt));
        }
    }
}

// Implementations for Session::Initialize, Session::Shutdown, and other methods will follow.

MCP_NAMESPACE_END