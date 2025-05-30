#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// Base metadata for notifications
struct NotificationMeta {
    Passthrough Additional;
};

struct BaseNotificationParams {
    // This parameter name is reserved by MCP to allow clients and servers to attach additional
    // metadata to their notifications.
    optional<NotificationMeta> _meta;
    Passthrough Additional;
};

struct Notification {
    string Method;
    optional<BaseNotificationParams> Params;
};

/* Cancellation */
/*
 * This notification can be sent by either side to indicate that it is cancelling a
 * previously-issued request.
 *
 * The request SHOULD still be in-flight, but due to communication latency, it is always possible
 * that this notification MAY arrive after the request has already finished.
 *
 * This notification indicates that the result will be unused, so any associated processing SHOULD
 * cease.
 *
 * A client MUST NOT attempt to cancel its `initialize` request.
 */
struct CancelledNotification : public Notification {
    CancelledNotificationParams Params;
    struct {
        BaseNotificationParams;
        RequestID RequestID; // The ID of the request to cancel. This MUST correspond to the ID of a
                             // request previously issued in the same direction.
        optional<string> Reason; // An optional string describing the reason for the cancellation.
                                 // This MAY be logged or presented to the user.
    } Params;

    CancelledNotification() {
        Method = "notifications/cancelled";
    }
};

// This notification is sent from the client to the server after initialization has finished.
struct InitializedNotification : public Notification {
    InitializedNotification() {
        Method = "notifications/initialized";
    }
};

const isInitializedNotification = (value : unknown)
    : value is InitializedNotification = > InitializedNotification.safeParse(value).success;

/* Progress notifications */
struct Progress {
    double Progress; // The progress thus far. This should increase every time progress is made,
                     // even if the total is unknown.
    optional<double>
        Total; // Total number of items to process (or total progress required), if known.
    optional<string> Message; // An optional message describing the current progress.
    Passthrough Additional;   // Additional properties.
};

// An out-of-band notification used to inform the receiver of a progress update for a long-running
// request.
struct ProgressNotification : public Notification {
    struct {
        BaseNotificationParams;
        Progress Progress;
        ProgressToken ProgressToken;
    } Params;

    ProgressNotification() {
        Method = "notifications/progress";
    }
};

// An optional notification from the server to the client, informing it that the list of resources
// it can read from has changed. This may be issued by servers without any previous subscription
// from the client.
struct ResourceListChangedNotification : public Notification {
    ResourceListChangedNotification() {
        Method = "notifications/resources/list_changed";
    }
};

// A notification from the server to the client, informing it that a resource has changed and may
// need to be read again. This should only be sent if the client previously sent a
// resources/subscribe request.
struct ResourceUpdatedNotification : public Notification {
    struct {
        BaseNotificationParams;
        string URI; // The URI of the resource that has been updated. This might be a sub-resource
                    // of the one that the client actually subscribed to.
    } Params;

    ResourceUpdatedNotification() {
        Method = "notifications/resources/updated";
    }
};

// An optional notification from the server to the client, informing it that the list of prompts it
// offers has changed. This may be issued by servers without any previous subscription from the
// client.
struct PromptListChangedNotification : public Notification {
    PromptListChangedNotification() {
        Method = "notifications/prompts/list_changed";
    }
};

// An optional notification from the server to the client, informing it that the list of tools it
// offers has changed. This may be issued by servers without any previous subscription from the
// client.
struct ToolListChangedNotification : public Notification {
    ToolListChangedNotification() {
        Method = "notifications/tools/list_changed";
    }
};

// Notification of a log message passed from server to client. If no logging/setLevel request has
// been sent from the client, the server MAY decide which messages to send automatically.
struct LoggingMessageNotification : public Notification {
    struct {
        BaseNotificationParams;
        LoggingLevel Level;      // The severity of this log message.
        optional<string> Logger; // An optional name of the logger issuing this message.
        any Data; // The data to be logged, such as a string message or an object. Any JSON
                  // serializable type is allowed here.
    } Params;

    LoggingMessageNotification() {
        Method = "notifications/message";
    }
};

// A notification from the client to the server, informing it that the list of roots has changed.
struct RootsListChangedNotification : public Notification {
    RootsListChangedNotification() {
        Method = "notifications/roots/list_changed";
    }
};

/* Client messages */
using ClientNotification = variant<CancelledNotification, ProgressNotification,
                                   InitializedNotification, RootsListChangedNotification>;

/* Server messages */
using ServerNotification =
    variant<CancelledNotification, ProgressNotification, LoggingMessageNotification,
            ResourceUpdatedNotification, ResourceListChangedNotification,
            ToolListChangedNotification, PromptListChangedNotification>;

MCP_NAMESPACE_END
