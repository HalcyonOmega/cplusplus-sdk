#include "Auth/Middleware/AllowedMethods.h"

#include "Core.h"

// TODO: Fix External Ref: MethodNotAllowedError

MCP_NAMESPACE_BEGIN

RequestHandler AllowedMethods(const vector<string>& AllowedMethods) {
    return [AllowedMethods](HTTP_Request& Req, HTTP_Response& Res, NextFunction Next) {
        // Note: Assuming HTTP_Request has Method property - needs to be defined in shared header
        // TODO: Add HTTP_Request::Method property definition in shared middleware types

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
        string AllowHeader = MSG_NULL;
        for (size_t i = 0; i < AllowedMethods.size(); ++i) {
            if (i > 0) { AllowHeader += ", "; }
            AllowHeader += AllowedMethods[i];
        }

        // Set response exactly like original: res.status(HTTPStatus::MethodNotAllowed).set('Allow',
        // ...).json(error.toResponseObject())
        Res.SetStatus(HTTPStatus::MethodNotAllowed);
        Res.SetHeader("Allow", AllowHeader);
        Res.SetJSON(Error.ToResponseObject());
    };
}

MCP_NAMESPACE_END