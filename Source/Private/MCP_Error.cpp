#include "MCP_Error.h"

MCP_NAMESPACE_BEGIN

Errors MCP_Error::GetCode() const {
    return m_Code;
}

const string& MCP_Error::GetMessage() const {
    return m_Message;
}

const optional<JSON>& MCP_Error::GetData() const {
    return m_Data;
}

JSON MCP_Error::ToJSON() const {
    JSON json;
    json["code"] = m_Code;
    json["message"] = m_Message;
    if (m_Data) { json["data"] = *m_Data; }
    return json;
}

MCP_Error MCP_Error::FromJSON(const JSON& json) {
    if (!json.contains("code") || !json.contains("message")) {
        throw std::invalid_argument("Invalid error JSON: missing required fields");
    }

    optional<JSON> data;
    if (json.contains("data")) { data = json["data"]; }

    return MCP_Error(json["code"].get<Errors>(), json["message"].get<string>(), data);
}

MCP_NAMESPACE_END