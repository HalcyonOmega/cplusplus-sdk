// Common/shared schemas for Model Context Protocol
#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

struct EmptyResult {
  "$ref" : "#/definitions/Result"
};

struct Notification {
  "properties" : {
    "method" : {"type" : "string"},
    "params" : {
      "additionalProperties" : {},
      "properties" : {
        "_meta" : {
          "additionalProperties" : {},
          "description" :
              "This parameter name is reserved by MCP to allow clients and "
              "servers to attach additional metadata to their notifications.",
          "type" : "object"
        }
      },
      "type" : "object"
    }
  },
                 "required" : ["method"],
                              "type" : "object"
};

struct PaginatedRequest {
  "properties" : {
    "method" : {"type" : "string"},
    "params" : {
      "properties" : {
        "cursor" : {
          "description" :
              "An opaque token representing the current pagination "
              "position.\nIf provided, the server should return results "
              "starting after this cursor.",
          "type" : "string"
        }
      },
      "type" : "object"
    }
  },
                 "required" : ["method"],
                              "type" : "object"
};

struct PaginatedResult {
  "properties" : {
    "_meta" : {
      "additionalProperties" : {},
      "description" : "This result property is reserved by the protocol "
                      "to allow clients and servers to attach additional "
                      "metadata to their responses.",
      "type" : "object"
    },
    "nextCursor" : {
      "description" : "An opaque token representing the pagination "
                      "position after the last returned result.\nIf "
                      "present, there may be more results available.",
      "type" : "string"
    }
  },
                 "type" : "object"
};

struct PingRequest {
  "description" : "A ping, issued by either the server or the client, to "
                  "check that the other party is still alive. The receiver "
                  "must promptly respond, or else may be disconnected.",
                  "properties"
      : {
        "method" : {"const" : "ping", "type" : "string"},
        "params" : {
          "additionalProperties" : {},
          "properties" : {
            "_meta" : {
              "properties" : {
                "progressToken" : {
                  "$ref" : "#/definitions/ProgressToken",
                  "description" :
                      "If specified, the caller is requesting out-of-band "
                      "progress notifications for this request (as represented "
                      "by notifications/progress). The value of this parameter "
                      "is an opaque token that will be attached to any "
                      "subsequent notifications. The receiver is not obligated "
                      "to provide these notifications."
                }
              },
              "type" : "object"
            }
          },
          "type" : "object"
        }
      },
        "required" : ["method"],
                     "type" : "object"
};

struct ProgressNotification {
  "description" : "An out-of-band notification used to inform the receiver "
                  "of a progress update for a long-running request.",
                  "properties"
      : {
        "method" : {"const" : "notifications/progress", "type" : "string"},
        "params" : {
          "properties" : {
            "message" : {
              "description" :
                  "An optional message describing the current progress.",
              "type" : "string"
            },
            "progress" : {
              "description" :
                  "The progress thus far. This should increase every time "
                  "progress is made, even if the total is unknown.",
              "type" : "number"
            },
            "progressToken" : {
              "$ref" : "#/definitions/ProgressToken",
              "description" :
                  "The progress token which was given in the initial request, "
                  "used to associate this notification with the request that "
                  "is proceeding."
            },
            "total" : {
              "description" : "Total number of items to process (or total "
                              "progress required), if known.",
              "type" : "number"
            }
          },
          "required" : [ "progress", "progressToken" ],
          "type" : "object"
        }
      },
        "required" : [ "method", "params" ],
                     "type" : "object"
};

struct ProgressToken {
  "description" : "A progress token, used to associate progress "
                  "notifications with the original request.",
                  "type" : [ "string", "integer" ]
};

struct Request {
  "properties" : {
    "method" : {"type" : "string"},
    "params" : {
      "additionalProperties" : {},
      "properties" : {
        "_meta" : {
          "properties" : {
            "progressToken" : {
              "$ref" : "#/definitions/ProgressToken",
              "description" :
                  "If specified, the caller is requesting out-of-band "
                  "progress notifications for this request (as represented "
                  "by notifications/progress). The value of this parameter "
                  "is an opaque token that will be attached to any "
                  "subsequent notifications. The receiver is not obligated "
                  "to provide these notifications."
            }
          },
          "type" : "object"
        }
      },
      "type" : "object"
    }
  },
                 "required" : ["method"],
                              "type" : "object"
};

struct RequestId {
  "description" : "A uniquely identifying ID for a request in JSON-RPC.",
                  "type" : [ "string", "integer" ]
};

struct Result {
  "additionalProperties" : {},
                           "properties"
      : {
        "_meta" : {
          "additionalProperties" : {},
          "description" : "This result property is reserved by the protocol to "
                          "allow clients and servers to attach additional "
                          "metadata to their responses.",
          "type" : "object"
        }
      },
        "type" : "object"
};

struct Role {
  "description"
      : "The sender or recipient of messages and data in a conversation.",
        "enum" : [ "assistant", "user" ],
                 "type" : "string"
};

struct CancelledNotification {
  "description"
      : "This notification can be sent by either side to indicate that it is "
        "cancelling a previously-issued request.\n\nThe request SHOULD still "
        "be in-flight, but due to communication latency, it is always "
        "possible that this notification MAY arrive after the request has "
        "already finished.\n\nThis notification indicates that the result "
        "will be unused, so any associated processing SHOULD cease.\n\nA "
        "client MUST NOT attempt to cancel its `initialize` request.",
        "properties"
      : {
        "method" : {"const" : "notifications/cancelled", "type" : "string"},
        "params" : {
          "properties" : {
            "reason" : {
              "description" :
                  "An optional string describing the reason for the "
                  "cancellation. This MAY be logged or presented to the user.",
              "type" : "string"
            },
            "requestId" : {
              "$ref" : "#/definitions/RequestId",
              "description" :
                  "The ID of the request to cancel.\n\nThis MUST correspond to "
                  "the ID of a request previously issued in the same direction."
            }
          },
          "required" : ["requestId"],
          "type" : "object"
        }
      },
        "required" : [ "method", "params" ],
                     "type" : "object"
};

struct Cursor {
  "description" : "An opaque token used to represent a cursor for pagination.",
                  "type" : "string"
};

struct Implementation {
  "description" : "Describes the name and version of an MCP implementation.",
                  "properties"
      : {"name" : {"type" : "string"}, "version" : {"type" : "string"}},
        "required" : [ "name", "version" ],
                     "type" : "object"
};

MCP_NAMESPACE_END