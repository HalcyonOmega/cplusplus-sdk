#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// Resource {
//   MSG_DESCRIPTION : "A known resource that the server is capable of reading.",
//                   MSG_PROPERTIES
//       : {
//         MSG_ANNOTATIONS : {
//           "$ref" : "#/definitions/Annotations",
//           MSG_DESCRIPTION : "Optional annotations for the client."
//         },
//         MSG_DESCRIPTION : {
//           MSG_DESCRIPTION :
//               "A description of what this resource represents.\n\nThis can be
//               " "used " "by clients to improve the LLM's understanding of
//               available " "resources. It can be thought of like a \"hint\" to
//               the model.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_MIME_TYPE : {
//           MSG_DESCRIPTION : "The MIME type of this resource, if known.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_NAME : {
//           MSG_DESCRIPTION :
//               "A human-readable name for this resource.\n\nThis can be "
//               "used by clients to populate UI elements.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_SIZE : {
//           MSG_DESCRIPTION :
//               "The size of the raw resource content, in bytes (i.e., before "
//               "base64 "
//               "encoding or any tokenization), if known.\n\nThis can be used
//               by " "Hosts to display file sizes and estimate context window
//               usage.",
//           MSG_TYPE : MSG_INTEGER
//         },
//         MSG_URI : {
//           MSG_DESCRIPTION : "The URI of this resource.",
//           MSG_FORMAT : MSG_URI,
//           MSG_TYPE : MSG_STRING
//         }
//       },
//         MSG_REQUIRED : [ MSG_NAME, MSG_URI ],
//                      MSG_TYPE : MSG_OBJECT
// };

// A known resource that the server is capable of reading.
struct Resource {
    URI URI;     // The URI of this resource.
    string Name; // A human-readable name for this resource. This can be used by clients to populate
                 // UI elements.
    optional<string> Description; // A description of what this resource represents. This can be
                                  // used by clients to improve the LLM's understanding of available
                                  // resources. It can be thought of like a "hint" to the model.
    optional<string> MIMEType;    // The MIME type of this resource, if known.
    optional<Annotations> Annotations; // Optional annotations for the client.
    optional<long long> Size; // The size of the raw resource content, in bytes (i.e., before base64
                              // encoding or any tokenization), if known. This can be used by Hosts
                              // to display file sizes and estimate context window usage.
};

// ResourceTemplate {
//   MSG_DESCRIPTION
//       : "A template description for resources available on the server.",
//         MSG_PROPERTIES
//       : {
//         MSG_ANNOTATIONS : {
//           "$ref" : "#/definitions/Annotations",
//           MSG_DESCRIPTION : "Optional annotations for the client."
//         },
//         MSG_DESCRIPTION : {
//           MSG_DESCRIPTION :
//               "A description of what this template is for.\n\nThis can be
//               used " "by clients to improve the LLM's understanding of
//               available " "resources. It can be thought of like a \"hint\" to
//               the model.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_MIME_TYPE : {
//           MSG_DESCRIPTION :
//               "The MIME type for all resources that match this template. This
//               " "should only be included if all resources matching this
//               template " "have the same type.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_NAME : {
//           MSG_DESCRIPTION : "A human-readable name for the type of resource
//           this "
//                           "template refers to.\n\nThis can be used by clients
//                           " "to populate UI elements.",
//           MSG_TYPE : MSG_STRING
//         },
//         MSG_URI_TEMPLATE : {
//           MSG_DESCRIPTION : "A URI template (according to RFC 6570) that can be
//           "
//                           "used to construct resource URIs.",
//           MSG_FORMAT : MSG_URITEMPLATE,
//           MSG_TYPE : MSG_STRING
//         }
//       },
//         MSG_REQUIRED : [ MSG_NAME, MSG_URI_TEMPLATE ],
//                      MSG_TYPE : MSG_OBJECT
// };

// A template description for resources available on the server.
struct ResourceTemplate {
    URITemplate URITemplate; // A URI template (according to RFC 6570) that can be used to construct
                             // resource URIs.
    string Name; // A human-readable name for the type of resource this template refers to. This can
                 // be used by clients to populate UI elements.
    optional<string> Description; // A description of what this template is for. This can be used by
                                  // clients to improve the LLM's understanding of available
                                  // resources. It can be thought of like a "hint" to the model.
    optional<string>
        MIMEType; // The MIME type for all resources that match this template. This should only be
                  // included if all resources matching this template have the same type.
    optional<Annotations> Annotations; // Optional annotations for the client.
};

MCP_NAMESPACE_END