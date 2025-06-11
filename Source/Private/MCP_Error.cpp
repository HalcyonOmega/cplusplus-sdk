#include "MCP_Error.h"

#include "Core/Constants/MessageConstants.h"

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
    json[MSG_CODE] = m_Code;
    json[MSG_MESSAGE] = m_Message;
    if (m_Data) { json[MSG_DATA] = *m_Data; }
    return json;
}

MCP_Error MCP_Error::FromJSON(const JSON& json) {
    if (!json.contains(MSG_CODE) || !json.contains(MSG_MESSAGE)) {
        throw std::invalid_argument("Invalid error JSON: missing required fields");
    }

    optional<JSON> data;
    if (json.contains(MSG_DATA)) { data = json[MSG_DATA]; }

    return MCP_Error(json[MSG_CODE].get<Errors>(), json[MSG_MESSAGE].get<string>(), data);
}

MCP_NAMESPACE_END