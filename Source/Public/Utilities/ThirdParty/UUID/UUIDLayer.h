#pragma once

#include "Core.h"
#include "UUID.h"

MCP_NAMESPACE_BEGIN

string GenerateUUID() {
    return to_string(uuids::uuid_random_generator()());
}

string GenerateRandomBytes(int length) {
    return GenerateUUID();
}

int GetCurrentTimestamp() {
    return static_cast<int>(
        chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch())
            .count());
}
MCP_NAMESPACE_END
