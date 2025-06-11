#include "Session.h" // Only direct include needed for public headers from this .cpp file

#include "Core/Constants/ErrorConstants.h"
#include "Utilities/ThirdParty/UUID/UUIDLayer.h"

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

void Session::Initialize(std::function<void(const std::optional<ErrorMessage>&)> callback) {
    if (m_State != SessionState::Uninitialized) {
        if (callback) {
            // TODO: Define proper error codes/messages in ErrorConstants.h or similar
            callback(ErrorMessage(Errors::ConnectionClosed,
                                  "Session already initialized or initializing.", std::nullopt));
        }
        return;
    }

    m_InitializeCallback = callback;
    m_State = SessionState::Initializing;

    InitializeRequest request;
    request.params.protocolVersion = MCP_LATEST_PROTOCOL_VERSION; // From Constants.h via Core.h
    request.params.capabilities = m_ClientCapabilities;
    request.params.clientInfo = m_ClientInfo;

    // Assign a unique ID to the request object itself if the struct supports it,
    // or manage it externally for JSON-RPC packaging.
    // The InitializeRequest struct in InitializeSchemas.h inherits from Request, which has an 'id'
    // member.
    request.id = GenerateUUID();

    // Store this request ID to match with the response
    // TODO: Implement a proper pending requests map:
    // m_PendingRequests[request.id.to_string()] = m_InitializeCallback; // Assuming RequestID can
    // be converted to string for map key

    // Serialize the request to JSON
    // This assumes InitializeRequest and its members will have to_json support
    // or we construct the JSON manually. For now, let's assume direct construction
    // as an example, and to highlight where to_json methods are needed.

    JSON jsonRequest;
    jsonRequest[MSG_JSON_RPC] = MSG_JSON_RPC_VERSION;    // "2.0"
    jsonRequest[MSG_ID] = request.id.get<std::string>(); // Assuming RequestID is variant<string,
                                                         // int> and we use string here
    jsonRequest[MSG_METHOD] = request.method;            // Should be "initialize" (MTHD_INITIALIZE)

    JSON paramsJson;
    paramsJson[MSG_PROTOCOL_VERSION] = request.params.protocolVersion;

    // Manual serialization for ClientCapabilities (example - ideally ClientCapabilities has
    // to_json)
    JSON clientCapabilitiesJson;
    if (request.params.capabilities.roots) {
        JSON rootsJson;
        if (request.params.capabilities.roots->listChanged) {
            rootsJson[MSG_LIST_CHANGED] = *request.params.capabilities.roots->listChanged;
        }
        clientCapabilitiesJson[MSG_ROOTS] = rootsJson; // MSG_ROOTS needed in Constants.h
    }
    if (request.params.capabilities.sampling) {
        // Sampling is optional<JSON> in ClientSchemas.h, use it if present
        clientCapabilitiesJson[MSG_SAMPLING] =
            *request.params.capabilities.sampling; // MSG_SAMPLING needed
    }
    // Add other capabilities if present...
    paramsJson[MSG_CAPABILITIES] = clientCapabilitiesJson;

    JSON clientInfoJson;
    clientInfoJson[MSG_NAME] = request.params.clientInfo.name;
    clientInfoJson[MSG_VERSION] = request.params.clientInfo.version;
    paramsJson[MSG_CLIENT_INFO] = clientInfoJson;

    jsonRequest[MSG_PARAMS] = paramsJson;

    if (m_Transport) {
        m_Transport->Send(jsonRequest.dump());
    } else {
        if (m_InitializeCallback) {
            m_State = SessionState::Error;
            // TODO: Define proper error codes/messages
            m_InitializeCallback(ErrorMessage(Errors::RequestTimeout, "Transport not available."));
        }
    }
}

// TODO: Implement Session::Initialize, Session::Shutdown, and other methods

MCP_NAMESPACE_END