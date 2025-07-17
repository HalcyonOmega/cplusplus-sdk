#pragma once

#include "CoreSDK/Common/Macros.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

enum class EProtocolVersion
{
	V2024_11_05,
	V2025_03_26,
	V2025_06_18,
};

DEFINE_ENUM_JSON(EProtocolVersion,
	{ EProtocolVersion::V2024_11_05, "2024-11-05" },
	{ EProtocolVersion::V2025_03_26, "2025-03-26" },
	{ EProtocolVersion::V2025_06_18, "2025-06-18" })

inline std::string ToString(const EProtocolVersion& InVersion)
{
	switch (InVersion)
	{
		case EProtocolVersion::V2024_11_05:
			return "2024-11-05";
		case EProtocolVersion::V2025_03_26:
			return "2025-03-26";
		case EProtocolVersion::V2025_06_18:
			return "2025-06-18";
		default:
			return "unknown";
	}
}
MCP_NAMESPACE_END