#pragma once

#include <random> // Required for std::mt19937

#include "Core.h"
#include "UUID.h"

MCP_NAMESPACE_BEGIN

// Add a static random engine for the UUID generator
static std::mt19937& GetRandomEngine() {
    static std::random_device RandomDevice;
    static std::mt19937 Engine(RandomDevice());
    return Engine;
}

string GenerateUUID() {
    // Pass the engine to the constructor
    uuids::uuid_random_generator Generator(GetRandomEngine());
    return uuids::to_string(Generator());
}

MCP_NAMESPACE_END
