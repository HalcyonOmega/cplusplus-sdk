#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// A notification which does not expect a response.
struct JSON_RPC_Notification : public Notification {
    string JSON_RPC = JSON_RPC_VERSION;
};

bool IsJSON_RPC_Notification(const JSON& value) {
    return value.is_object() && value.value(MSG_KEY_JSON_RPC, "") == JSON_RPC_VERSION
           && value.contains("method") && !value.contains(MSG_KEY_ID);
}

// A response to a request that indicates an error occurred.
struct JSON_RPC_Error {
    string JSON_RPC = JSON_RPC_VERSION;
    RequestID ID;
    struct {
        ErrorCode Code; // The error type that occurred.
        string Message; // A short description of the error. The message SHOULD be limited to a
                        // concise single sentence.
        optional<any>
            Data; // Additional information about the error. The value of this member is defined by
                  // the sender (e.g. detailed error information, nested errors etc.).
    } Error;
};

bool IsJSON_RPC_Error(const JSON& value) {
    return value.is_object() && value.value(MSG_KEY_JSON_RPC, "") == JSON_RPC_VERSION
           && value.contains(MSG_KEY_ID) && value.contains("error") && !value.contains("result");
}

// A request that expects a response.
struct JSON_RPC_Request : public Request {
    string JSON_RPC = JSON_RPC_VERSION;
    RequestID ID;
};
bool IsJSON_RPC_Request(const JSON& value) {
    return value.is_object() && value.value(MSG_KEY_JSON_RPC, "") == JSON_RPC_VERSION
           && value.contains(MSG_KEY_ID) && value.contains("method") && !value.contains("error")
           && !value.contains("result");
}

// A successful (non-error) response to a request.
struct JSON_RPC_Response {
    string JSON_RPC = JSON_RPC_VERSION;
    RequestID ID;
    Result Result;
};

bool IsJSON_RPC_Response(const JSON& value) {
    return value.is_object() && value.value(MSG_KEY_JSON_RPC, "") == JSON_RPC_VERSION
           && value.contains(MSG_KEY_ID) && value.contains("result") && !value.contains("error");
}

using JSON_RPC_Message =
    variant<JSON_RPC_Request, JSON_RPC_Notification, JSON_RPC_Response, JSON_RPC_Error, >;

} // namespace MCP
