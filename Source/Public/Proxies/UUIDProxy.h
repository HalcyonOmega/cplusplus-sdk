#pragma once

#include <Poco/UUID.h>
#include <Poco/UUIDGenerator.h>

#include <string>

#include "CoreSDK/Common/Macros.h"

MCP_NAMESPACE_BEGIN

/**
 * @brief Generates a globally unique and cryptographically secure UUID.
 *
 * This function creates a Version 4 UUID to be used as a UUID,
 * satisfying the requirements for uniqueness and security.
 *
 * @return std::string The generated UUID.
 */
inline std::string GenerateUUID() {
    // Get the default UUID generator.
    Poco::UUIDGenerator& Generator = Poco::UUIDGenerator::defaultGenerator();

    // Create a cryptographically secure random UUID (Version 4).
    Poco::UUID UUID = Generator.createRandom();

    // Return the UUID as a string.
    return UUID.toString();
}

MCP_NAMESPACE_END
