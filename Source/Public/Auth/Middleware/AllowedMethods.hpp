#pragma once

#include "Auth/Types/AuthErrors.h"
#include "Core.h"

// TODO: Fix External Ref: MethodNotAllowedError

MCP_NAMESPACE_BEGIN

// Forward declarations for HTTP types (to avoid duplication with other middleware)
struct HTTPRequest;
struct HTTPResponse;
using NextFunction = function<void()>;
using RequestHandler = function<void(HTTPRequest&, HTTPResponse&, NextFunction)>;

/**
 * Middleware to handle unsupported HTTP methods with a 405 Method Not Allowed response.
 *
 * @param AllowedMethods Array of allowed HTTP methods for this endpoint (e.g., {"GET", "POST"})
 * @returns Express middleware that returns a 405 error if method not in allowed list
 */
RequestHandler AllowedMethods(const vector<string>& AllowedMethods) {
    return [AllowedMethods](HTTPRequest& Req, HTTPResponse& Res, NextFunction Next) {
        // Note: Assuming HTTPRequest has Method property - needs to be defined in shared header
        // TODO: Add HTTPRequest::Method property definition in shared middleware types

        // Check if the request method is in the allowed methods list
        bool IsMethodAllowed = false;
        for (const auto& Method : AllowedMethods) {
            if (Method == Req.Method) {
                IsMethodAllowed = true;
                break;
            }
        }

        if (IsMethodAllowed) {
            Next();
            return;
        }

        // Create MethodNotAllowedError (exactly matching original TypeScript)
        const auto Error =
            MethodNotAllowedError("The method " + Req.Method + " is not allowed for this endpoint");

        // Create Allow header with comma-separated methods (matching allowedMethods.join(', '))
        string AllowHeader = "";
        for (size_t i = 0; i < AllowedMethods.size(); ++i) {
            if (i > 0) { AllowHeader += ", "; }
            AllowHeader += AllowedMethods[i];
        }

        // Set response exactly like original: res.status(405).set('Allow',
        // ...).json(error.toResponseObject())
        Res.SetStatus(405);
        Res.SetHeader("Allow", AllowHeader);
        Res.SetJSON(Error.ToResponseObject());
    };
}

MCP_NAMESPACE_END