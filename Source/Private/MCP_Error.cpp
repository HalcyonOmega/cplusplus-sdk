#include "MCP_Error.h"

MCP_NAMESPACE_BEGIN

MCP_Error::MCP_Error(int code, const string& message, const optional<JSON>& data)
    : Code(code), Message(message), Data(data) {}

int MCP_Error::GetCode() const {
    return Code;
}

const string& MCP_Error::GetMessage() const {
    return Message;
}

const optional<JSON>& MCP_Error::GetData() const {
    return Data;
}

JSON MCP_Error::ToJSON() const {
    JSON json;
    json["code"] = Code;
    json["message"] = Message;
    if (Data) { json["data"] = *Data; }
    return json;
}

MCP_Error MCP_Error::FromJSON(const JSON& json) {
    if (!json.contains("code") || !json.contains("message")) {
        throw std::invalid_argument("Invalid error JSON: missing required fields");
    }

    optional<JSON> data;
    if (json.contains("data")) { data = json["data"]; }

    return MCP_Error(json["code"].get<int>(), json["message"].get<string>(), data);
}

MCP_NAMESPACE_END