#pragma once

#include <string>
#include <utility>

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"
#include "ProtocolInfo.h"

MCP_NAMESPACE_BEGIN

// Implementation {
//   MSG_DESCRIPTION: "Describes the name and version of an MCP implementation.",
//    MSG_PROPERTIES: {
//    MSG_NAME: {
//    MSG_TYPE: MSG_STRING
//    },
//    MSG_VERSION: {
//    MSG_TYPE: MSG_STRING
//    }
//    },
//         MSG_REQUIRED: [ MSG_NAME, MSG_VERSION ],
//                      MSG_TYPE: MSG_OBJECT
// };

// Describes the name and version of an MCP implementation.
struct Implementation
{
	std::string Name{};
	std::string Version{};

	EProtocolVersion ProtocolVersion{ EProtocolVersion::V2025_03_26 };

	JKEY(NAMEKEY, Name, "name")
	JKEY(VERSIONKEY, Version, "version")

	DEFINE_TYPE_JSON(Implementation, NAMEKEY, VERSIONKEY)

	Implementation() = default;
	Implementation(std::string InName,
		std::string InVersion,
		const std::optional<EProtocolVersion> InProtocolVersion = std::nullopt)
		: Name(std::move(InName)),
		  Version(std::move(InVersion)),
		  ProtocolVersion(InProtocolVersion.value_or(EProtocolVersion::V2025_03_26))
	{}
};

MCP_NAMESPACE_END