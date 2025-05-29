#pragma once

#include "../Core/Common.hpp"
#include "Transport.hpp"

namespace MCP::Transport {

// TODO: Fix External Ref: WebSocket implementation (platform-specific)
// TODO: Fix External Ref: URL parsing (consider std::string or custom URL class)
// TODO: Fix External Ref: JSON_RPC_Message validation schema

const string SUBPROTOCOL = "mcp";

/**
 * Client transport for WebSocket: this will connect to a server over the WebSocket protocol.
 */
class WebSocket_Client_Transport : public Transport {
private:
    // TODO: Fix External Ref: WebSocket - replace with actual WebSocket implementation
    optional<void*> Socket_; // Optional WebSocket instance (was _socket?: WebSocket)
    string URL_; // URL was not optional in TypeScript

    // Helper method to validate JSON-RPC message structure
    bool ValidateJSON_RPC_Message(const JSON& json) const {
        // Basic JSON-RPC validation equivalent to JSON_RPC_MessageSchema.parse()
        if (!json.contains("jsonrpc") || json["jsonrpc"] != "2.0") {
            return false;
        }

        // Check if it's a request, notification, response, or error
        bool hasId = json.contains("id");
        bool hasMethod = json.contains("method");
        bool hasResult = json.contains("result");
        bool hasError = json.contains("error");

        // Request: must have id and method
        if (hasId && hasMethod && !hasResult && !hasError) {
            return true;
        }

        // Notification: must have method but no id
        if (!hasId && hasMethod && !hasResult && !hasError) {
            return true;
        }

        // Response: must have id and result
        if (hasId && !hasMethod && hasResult && !hasError) {
            return true;
        }

        // Error response: must have id and error
        if (hasId && !hasMethod && !hasResult && hasError) {
            return true;
        }

        return false;
    }

    // Helper method to convert JSON to JSON_RPC_Message
    optional<JSON_RPC_Message> JSONToMessage(const JSON& json) const {
        // This would need to be implemented based on the actual JSON_RPC_Message variant structure
        // TODO: Implement actual conversion based on JSON_RPC_Message definition
        // For now, returning nullopt as placeholder
        return nullopt;
    }

    // Helper method to convert JSON_RPC_Message to JSON
    JSON MessageToJSON(const JSON_RPC_Message& message) const {
        // This would need to be implemented based on the actual JSON_RPC_Message variant structure
        // TODO: Implement actual conversion based on JSON_RPC_Message definition
        // For now, returning empty JSON as placeholder
        return JSON{};
    }

public:
    // Event handlers - optional callbacks matching TypeScript interface
    optional<function<void()>> OnClose;
    optional<function<void(const exception&)>> OnError;
    optional<function<void(const JSON_RPC_Message&, const optional<Auth::AuthInfo>&)>> OnMessage;

    // Session ID as required by Transport interface
    optional<string> SessionID;

    explicit WebSocket_Client_Transport(const string& Url) : URL_(Url), Socket_(nullopt) {}

    future<void> Start() override {
        if (Socket_.has_value()) {
            throw runtime_error(
                "WebSocket_Client_Transport already started! If using Client class, note that Connect() calls Start() automatically."
            );
        }

        // Create promise/future pair to match TypeScript Promise pattern
        auto promise_ptr = make_shared<promise<void>>();
        auto future_result = promise_ptr->get_future();

        // Run async to match TypeScript Promise behavior
        async(launch::async, [this, promise_ptr]() {
            try {
                // TODO: Fix External Ref: WebSocket - implement actual WebSocket connection
                // This would create a new WebSocket with URL_ and SUBPROTOCOL
                // Socket_ = /* new WebSocket instance */;

                // Setup error handler
                // TODO: Fix External Ref: WebSocket error event handling
                // socket.onerror = [this, promise_ptr](event) {
                //     auto error = /* extract error from event */;
                //     promise_ptr->set_exception(make_exception_ptr(error));
                //     if (OnError.has_value()) OnError.value()(error);
                // };

                // Setup open handler
                // TODO: Fix External Ref: WebSocket open event handling
                // socket.onopen = [promise_ptr]() {
                //     promise_ptr->set_value(); // Resolve the promise
                // };

                // Setup close handler
                // TODO: Fix External Ref: WebSocket close event handling
                // socket.onclose = [this]() {
                //     if (OnClose.has_value()) OnClose.value()();
                // };

                // Setup message handler with proper JSON validation
                // TODO: Fix External Ref: WebSocket message event handling
                // socket.onmessage = [this](event) {
                //     try {
                //         JSON parsed = JSON::parse(event.data);
                //
                //         // Validate JSON-RPC message structure (equivalent to JSON_RPC_MessageSchema.parse)
                //         if (!ValidateJSON_RPC_Message(parsed)) {
                //             if (OnError.has_value()) {
                //                 OnError.value()(runtime_error("Invalid JSON-RPC message format"));
                //             }
                //             return;
                //         }
                //
                //         // Convert to JSON_RPC_Message
                //         auto message = JSONToMessage(parsed);
                //         if (!message.has_value()) {
                //             if (OnError.has_value()) {
                //                 OnError.value()(runtime_error("Failed to convert JSON to JSON_RPC_Message"));
                //             }
                //             return;
                //         }
                //
                //         // Call message handler with message and optional auth info
                //         if (OnMessage.has_value()) {
                //             OnMessage.value()(message.value(), nullopt); // No auth info for basic WebSocket
                //         }
                //     } catch (const JSON::parse_error& error) {
                //         if (OnError.has_value()) {
                //             OnError.value()(runtime_error("JSON parse error: " + string(error.what())));
                //         }
                //     } catch (const exception& error) {
                //         if (OnError.has_value()) {
                //             OnError.value()(error);
                //         }
                //     }
                // };

                // For now, just set promise value since WebSocket is not implemented
                promise_ptr->set_value();
            } catch (const exception& error) {
                promise_ptr->set_exception(make_exception_ptr(error));
            }
        });

        return future_result;
    }

    future<void> Close() override {
        return async(launch::async, [this]() {
            // TODO: Fix External Ref: WebSocket - close the socket
            if (Socket_.has_value()) {
                // Socket_.value()->close();
                Socket_ = nullopt;
            }
        });
    }

    // Updated Send method to match Transport interface signature
    future<void> Send(const JSON_RPC_Message& Message, const optional<TransportSendOptions>& Options = nullopt) override {
        // Create promise/future pair to match TypeScript Promise pattern
        auto promise_ptr = make_shared<promise<void>>();
        auto future_result = promise_ptr->get_future();

        // Run async to match TypeScript Promise behavior
        async(launch::async, [this, Message, Options, promise_ptr]() {
            try {
                if (!Socket_.has_value()) {
                    throw runtime_error("Not connected");
                }

                // Convert JSON_RPC_Message to JSON string
                JSON serialized_message = MessageToJSON(Message);
                string message_str = serialized_message.dump();

                // TODO: Fix External Ref: WebSocket - send the serialized message
                // Socket_.value()->send(message_str);

                // For now, just resolve since WebSocket is not implemented
                promise_ptr->set_value();
            } catch (const exception& error) {
                promise_ptr->set_exception(make_exception_ptr(error));
            }
        });

        return future_result;
    }

    // Legacy Send method for backward compatibility (matching original signature)
    future<void> Send(const JSON_RPC_Message& Message) {
        return Send(Message, nullopt);
    }
};

} // namespace MCP::Transport
