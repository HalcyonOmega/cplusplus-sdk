#pragma once

#include "Core.h"

// TODO: Fix External Ref: cors (CORS middleware)
// TODO: Fix External Ref: express-rate-limit (Rate limiting middleware)

MCP_NAMESPACE_BEGIN

// Client registration handler options
struct ClientRegistrationHandlerOptions {
    /**
     * A store used to save information about dynamically registered OAuth clients.
     */
    shared_ptr<OAuthRegisteredClientsStore> ClientsStore;

    /**
     * The number of seconds after which to expire issued client secrets, or 0 to prevent expiration
     * of client secrets (not recommended).
     *
     * If not set, defaults to 30 days.
     */
    optional<int> ClientSecretExpirySeconds;

    /**
     * Rate limiting configuration for the client registration endpoint.
     * Set to false to disable rate limiting for this endpoint.
     * Registration endpoints are particularly sensitive to abuse and should be rate limited.
     */
    optional<RateLimitOptions> RateLimit;
    bool RateLimitDisabled = false;
};

// Constants
constexpr int DEFAULT_CLIENT_SECRET_EXPIRY_SECONDS = 30 * 24 * 60 * 60; // 30 days

// Utility functions
string GenerateUUID();
string GenerateRandomBytes(int length);
int GetCurrentTimestamp();

// Main client registration handler function
RequestHandler ClientRegistrationHandler(const ClientRegistrationHandlerOptions& options);

MCP_NAMESPACE_END