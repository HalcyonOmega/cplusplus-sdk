#pragma once

#include <Poco/URI.h>

#include "../CoreSDK/Common/Macros.h"
#include "URITemplate.h"

MCP_NAMESPACE_BEGIN

// TODO: @HalcyonOmega create URI, URIFile, & URITemplate classes
struct URI {
    std::string Value;
};

struct URIFile {
    static constexpr const char* URI_FILE_PREFIX = "file://";
    std::string Value = URI_FILE_PREFIX; // This *must* start with "file://" for now.
};

// TODO: @HalcyonOmega Implement proper URL class
class URL {
  public:
    std::string Href;
    std::string Origin;

    URL(const std::string& InURLString) : Href(InURLString) {
        // TODO: Proper URL parsing
        Origin = InURLString; // Simplified
    }

    URL(const std::string& InRelative, const URL& InBase) {
        // TODO: Proper relative URL resolution
        Href = InBase.Href + "/" + InRelative;
        Origin = InBase.Origin;
    }
};

MCP_NAMESPACE_END