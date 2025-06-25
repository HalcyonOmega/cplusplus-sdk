#pragma once

#include <optional>
#include <string>

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"
#include "URIProxy.h"

MCP_NAMESPACE_BEGIN

// Represents a root directory or file that the server can operate on.
struct Root {
    MCP::URIFile URI; // The URI identifying the root. This *must* start with file:// for now. This
                      // restriction may be relaxed in future versions of the protocol to allow
                      // other URI schemes.
    std::optional<std::string>
        Name; // An optional name for the root. This can be used to provide a human-readable
              // identifier for the root, which may be useful for display purposes or for
              // referencing the root in other parts of the application.

    JKEY(URIKEY, URI, "uri")
    JKEY(NAMEKEY, Name, "name")

    DEFINE_TYPE_JSON(Root, URIKEY, NAMEKEY)
};

template <typename T>
concept RootType = requires(T Type) {
    { Type.URI } -> std::same_as<MCP::URIFile>;
    { Type.Name } -> std::same_as<std::optional<std::string>>;
};

MCP_NAMESPACE_END