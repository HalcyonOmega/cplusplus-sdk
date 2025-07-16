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

MCP_NAMESPACE_END