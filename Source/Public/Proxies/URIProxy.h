#pragma once

#include <Poco/URI.h>

#include "../CoreSDK/Common/Macros.h"
#include "URITemplate.h"

template <> struct std::hash<Poco::URI>
{
	std::size_t operator()(const Poco::URI& InURI) const noexcept { return std::hash<string>{}(InURI.toString()); }
};

MCP_NAMESPACE_BEGIN

// TODO: @HalcyonOmega create URI, URIFile, & URITemplate classes
using URI = Poco::URI;
using URIFile = Poco::URI;

// TODO: @HalcyonOmega Implement proper URL class
struct URL
{
	std::string Href;
	std::string Origin;

	explicit URL(const std::string& InURLString) : Href(InURLString)
	{
		// TODO: Proper URL parsing
		Origin = InURLString; // Simplified
	}

	URL(const std::string& InRelative, const URL& InBase)
	{
		// TODO: Proper relative URL resolution
		Href = InBase.Href + "/" + InRelative;
		Origin = InBase.Origin;
	}
};

MCP_NAMESPACE_END