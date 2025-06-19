#pragma once

#include <string>
#include <string_view>
#include <variant>

#include "Macros.h"

using std::string;
using std::string_view;
using std::variant;

MCP_NAMESPACE_BEGIN

// RequestID {
//   MSG_DESCRIPTION : "A uniquely identifying ID for a request in JSON-RPC.",
//                   MSG_TYPE : [ MSG_STRING, MSG_INTEGER ]
// };

// A uniquely identifying ID for a request in JSON-RPC.
struct RequestID {
  private:
    // TODO: @HalcyonOmega Is LongLong the right type or should it be double?
    variant<string, int, long long> m_RequestID;

  public:
    // Constructors
    RequestID(string StringID) : m_RequestID(std::move(StringID)) {}
    RequestID(int IntID) : m_RequestID(IntID) {}
    RequestID(long long LongLongID) : m_RequestID(LongLongID) {}

    // Direct Getters
    [[nodiscard]] string_view ToString() const;
};

MCP_NAMESPACE_END