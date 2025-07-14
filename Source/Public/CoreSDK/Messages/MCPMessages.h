#pragma once

#include <optional>
#include <string>
#include <utility>
#include <variant>

#include "CoreSDK/Common/BaseTypes.h"
#include "CoreSDK/Common/Capabilities.h"
#include "CoreSDK/Common/Content.h"
#include "CoreSDK/Common/Implementation.h"
#include "CoreSDK/Common/Logging.h"
#include "CoreSDK/Common/Roles.h"
#include "CoreSDK/Features/CompletionBase.h"
#include "CoreSDK/Features/PromptBase.h"
#include "CoreSDK/Features/ResourceBase.h"
#include "CoreSDK/Features/RootBase.h"
#include "CoreSDK/Features/SamplingBase.h"
#include "CoreSDK/Features/ToolBase.h"
#include "CoreSDK/Messages/NotificationBase.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "CoreSDK/Messages/ResponseBase.h"
#include "JSONProxy.h"

MCP_NAMESPACE_BEGIN

struct EmptyResponse : ResponseBase
{
	explicit EmptyResponse(const RequestID& InRequest) : ResponseBase(InRequest) {};
};

// InitializeRequest {
//   MSG_DESCRIPTION: "This request is sent from the client to the server when it first connects, asking it to begin
//   initialization.",
//                   MSG_PROPERTIES: {
//         MSG_METHOD: {MSG_CONST: MTHD_INITIALIZE, MSG_TYPE: MSG_STRING},
//         MSG_PARAMS: {
//           MSG_PROPERTIES: {
//             MSG_CAPABILITIES: {"$ref": "#/definitions/ClientCapabilities"},
//             MSG_CLIENT_INFO: {"$ref": "#/definitions/Implementation"},
//             MSG_PROTOCOL_VERSION: {
//               MSG_DESCRIPTION: "The latest version of the Model Context Protocol that the client supports. The client
//               MAY decide to support older versions as well.",
//               MSG_TYPE: MSG_STRING
//             }
//           },
//           MSG_REQUIRED: [ MSG_CAPABILITIES, MSG_CLIENT_INFO, MSG_PROTOCOL_VERSION ], MSG_TYPE: MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED: [ MSG_METHOD, MSG_PARAMS ],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * This request is sent from the client to the server when it first connects,
 * asking it to begin initialization.
 */
struct InitializeRequest : RequestBase
{
	struct Params : RequestParams
	{
		std::string ProtocolVersion;	 // The latest version of the Model Context Protocol
										 // that the client supports. The client MAY decide to
										 // support older versions as well.
		ClientCapabilities Capabilities; // The capabilities of the client.
		Implementation ClientInfo;		 // The implementation of the client.

		JKEY(PROTOCOLVERSIONKEY, ProtocolVersion, "protocolVersion")
		JKEY(CAPABILITIESKEY, Capabilities, "capabilities")
		JKEY(CLIENTINFOKEY, ClientInfo, "clientInfo")

		DEFINE_TYPE_JSON_DERIVED(InitializeRequest::Params,
			RequestParams,
			PROTOCOLVERSIONKEY,
			CAPABILITIESKEY,
			CLIENTINFOKEY)

		explicit Params(std::string InProtocolVersion,
			ClientCapabilities InCapabilities,
			Implementation InClientInfo,
			const std::optional<RequestParamsMeta>& InMeta = std::nullopt)
			: RequestParams(InMeta),
			  ProtocolVersion(std::move(InProtocolVersion)),
			  Capabilities(std::move(InCapabilities)),
			  ClientInfo(std::move(InClientInfo))
		{}
	};

	explicit InitializeRequest(const InitializeRequest::Params& InParams) : RequestBase("initialize", InParams) {}
};

// InitializeResult {
//   MSG_DESCRIPTION: "After receiving an initialize request from the client, "
//                   "the server sends this response.",
//    MSG_PROPERTIES: {
//         MSG_META: {
//           MSG_ADDITIONAL_PROPERTIES: {},
//           MSG_DESCRIPTION: "This result property is reserved by the protocol
//           to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           MSG_TYPE: MSG_OBJECT
//         },
//         MSG_CAPABILITIES: {"$ref": "#/definitions/ServerCapabilities"},
//         MSG_INSTRUCTIONS: {
//           MSG_DESCRIPTION:
//               "Instructions describing how to use the server and its
//               features.\n\nThis can be used by clients to improve the LLM's
//               understanding of available tools, resources, etc. It can be
//               thought of like a \"hint\" to the model. For example, this
//               information MAY be added to the system prompt.",
//           MSG_TYPE: MSG_STRING
//         },
//         MSG_PROTOCOL_VERSION: {
//           MSG_DESCRIPTION: "The version of the Model Context Protocol that
//           the server wants to use. This may not match the version that the client requested. If the client cannot
//           support this version, it MUST disconnect.",
//           MSG_TYPE: MSG_STRING
//         },
//         MSG_SERVER_INFO: {"$ref": "#/definitions/Implementation"}
//       },
//         MSG_REQUIRED: [ MSG_CAPABILITIES, MSG_PROTOCOL_VERSION, MSG_SERVER_INFO ],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * After receiving an initialize request from the client, the server sends this response.
 */
struct InitializeResponse : ResponseBase
{
	struct Result : ResultParams
	{
		std::string ProtocolVersion;							 // The version of the Model Context Protocol that the
																 // server wants to use. This may not match the version
																 // that the client requested. If the client cannot
																 // support this version, it MUST disconnect.
		ServerCapabilities Capabilities;						 // The capabilities of the server.
		Implementation ServerInfo;								 // The implementation of the server.
		std::optional<std::string> Instructions{ std::nullopt }; // Instructions describing how to use the server
																 // and its features. This can be used by clients to
																 // improve the LLM's understanding of available
																 // tools, resources, etc. It can be thought of like
																 // a "hint" to the model. For example, this
																 // information MAY be added to the system prompt.

		JKEY(PROTOCOLVERSIONKEY, ProtocolVersion, "protocolVersion")
		JKEY(CAPABILITIESKEY, Capabilities, "capabilities")
		JKEY(SERVERINFOKEY, ServerInfo, "serverInfo")
		JKEY(INSTRUCTIONSKEY, Instructions, "instructions")

		DEFINE_TYPE_JSON_DERIVED(InitializeResponse::Result,
			ResultParams,
			PROTOCOLVERSIONKEY,
			CAPABILITIESKEY,
			SERVERINFOKEY,
			INSTRUCTIONSKEY)

		explicit Result(std::string InProtocolVersion,
			ServerCapabilities InCapabilities,
			Implementation InServerInfo,
			const std::optional<std::string>& InInstructions = std::nullopt,
			const std::optional<JSONData>& InMeta = std::nullopt)
			: ResultParams(InMeta),
			  ProtocolVersion(std::move(InProtocolVersion)),
			  Capabilities(std::move(InCapabilities)),
			  ServerInfo(std::move(InServerInfo)),
			  Instructions(InInstructions)
		{}
	};

	explicit InitializeResponse(const RequestID& InRequestID, const InitializeResponse::Result& InResult)
		: ResponseBase(InRequestID, InResult)
	{}
};

// InitializedNotification {
//   MSG_DESCRIPTION: "This notification is sent from the client to the "
//                   "server after initialization has finished.",
//     MSG_PROPERTIES: {
//         MSG_METHOD: {MSG_CONST: MTHD_NOTIFICATIONS_INITIALIZED, MSG_TYPE:
//         MSG_STRING}, MSG_PARAMS: {
//           MSG_ADDITIONAL_PROPERTIES: {},
//           MSG_PROPERTIES: {
//             MSG_META: {
//               MSG_ADDITIONAL_PROPERTIES: {},
//               MSG_DESCRIPTION: "This parameter name is reserved by MCP to "
//                               "allow clients and servers to attach "
//                               "additional metadata to their notifications.",
//               MSG_TYPE: MSG_OBJECT
//             }
//           },
//           MSG_TYPE: MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED: [MSG_METHOD],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * This notification is sent from the client to the server after initialization has finished.
 */
struct InitializedNotification : NotificationBase
{
	InitializedNotification() : NotificationBase("notifications/initialized") {}
};

// PingRequest {
//   MSG_DESCRIPTION: "A ping, issued by either the server or the client, to check that the other party is still alive.
//   The receiver must promptly respond, or else may be disconnected.",
//         MSG_PROPERTIES: {
//         MSG_METHOD: {MSG_CONST: MTHD_PING, MSG_TYPE: MSG_STRING},
//         MSG_PARAMS: {
//           MSG_ADDITIONAL_PROPERTIES: {},
//           MSG_PROPERTIES: {
//             MSG_META: {
//               MSG_PROPERTIES: {
//                 MSG_PROGRESS_TOKEN: {
//                   "$ref": "#/definitions/ProgressToken",
//                   MSG_DESCRIPTION:
//                       "If specified, the caller is requesting out-of-band progress notifications for this request (as
//                       represented by notifications/progress). The value of this parameter is an opaque token that
//                       will be attached to any subsequent notifications. The receiver is not obligated to provide
//                       these notifications."
//                 }
//               },
//               MSG_TYPE: MSG_OBJECT
//             }
//           },
//           MSG_TYPE: MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED: [MSG_METHOD],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * A ping, issued by either the server or the client, to check that the other
 * party is still alive. The receiver must promptly respond, or else it may be
 * disconnected.
 */
struct PingRequest : RequestBase
{
	PingRequest() : RequestBase("ping") {}
};

struct PingResponse : ResponseBase
{
	explicit PingResponse(const RequestID& InRequestID) : ResponseBase(InRequestID) {}
};

// ListToolsRequest {
//   MSG_DESCRIPTION: "Sent from the client to request a list of tools the server has.",
//         MSG_PROPERTIES: {
//           MSG_METHOD: {MSG_CONST: MTHD_TOOLS_LIST, MSG_TYPE: MSG_STRING},
//           MSG_PARAMS: {
//             MSG_PROPERTIES: {
//               MSG_CURSOR: {
//                 MSG_DESCRIPTION: "An opaque token representing the current pagination position.If provided, the
//                 server should return results starting after this cursor.",
//                 MSG_TYPE: MSG_STRING
//               }
//             },
//             MSG_TYPE: MSG_OBJECT
//           }
//         },
//                        MSG_REQUIRED: [MSG_METHOD],
//                                     MSG_TYPE: MSG_OBJECT
// };

/**
 * Sent from the client to request a list of tools the server has.
 */
struct ListToolsRequest : RequestBase
{
	explicit ListToolsRequest(const PaginatedRequestParams& InParams) : RequestBase("tools/list", InParams) {}
};

// ListToolsResult {
//   MSG_DESCRIPTION
//     : "The server's response to a tools/list request from the client.",
//         MSG_PROPERTIES
//     : {
//         MSG_META: {
//           MSG_ADDITIONAL_PROPERTIES: {},
//           MSG_DESCRIPTION: "This result property is reserved by the protocol
//           to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           MSG_TYPE: MSG_OBJECT
//         },
//         MSG_NEXT_CURSOR: {
//           MSG_DESCRIPTION: "An opaque token representing the pagination "
//                           "position after the last returned result.\nIf "
//                           "present, there may be more results available.",
//           MSG_TYPE: MSG_STRING
//         },
//         MSG_TOOLS: {MSG_ITEMS: {"$ref": "#/definitions/Tool"}, MSG_TYPE:
//         MSG_ARRAY}
//       },
//         MSG_REQUIRED: [MSG_TOOLS],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * The server's response to a tools/list request from the client.
 */
struct ListToolsResponse : ResponseBase
{
	struct Result : PaginatedResultParams
	{
		std::vector<Tool> Tools;

		JKEY(TOOLSKEY, Tools, "tools")

		DEFINE_TYPE_JSON_DERIVED(ListToolsResponse::Result, PaginatedResultParams, TOOLSKEY)

		explicit Result(const std::vector<Tool>& InTools,
			const std::optional<std::string>& InNextCursor = std::nullopt,
			const std::optional<JSONData>& InMeta = std::nullopt)
			: PaginatedResultParams(InNextCursor, InMeta),
			  Tools(InTools)
		{}
	};

	explicit ListToolsResponse(const RequestID& InRequestID, const ListToolsResponse::Result& InResult)
		: ResponseBase(InRequestID, InResult)
	{}
};

// CallToolRequest {
//   MSG_DESCRIPTION: "Used by the client to invoke a tool provided by the
//   server.",
//       MSG_PROPERTIES: {
//         MSG_METHOD: {MSG_CONST: MTHD_TOOLS_CALL, MSG_TYPE: MSG_STRING},
//         MSG_PARAMS: {
//           MSG_PROPERTIES: {
//             MSG_ARGUMENTS: {MSG_ADDITIONAL_PROPERTIES: {}, MSG_TYPE:
//             MSG_OBJECT}, MSG_NAME: {MSG_TYPE: MSG_STRING}
//           },
//           MSG_REQUIRED: [MSG_NAME],
//           MSG_TYPE: MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED: [ MSG_METHOD, MSG_PARAMS ],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * Used by the client to invoke a tool provided by the server.
 */
struct CallToolRequest : RequestBase
{
	struct Params : RequestParams
	{
		std::string Name;
		std::optional<std::unordered_map<std::string, JSONData>> Arguments{ std::nullopt };

		JKEY(NAMEKEY, Name, "name")
		JKEY(ARGUMENTSKEY, Arguments, "arguments")

		DEFINE_TYPE_JSON_DERIVED(CallToolRequest::Params, RequestParams, NAMEKEY, ARGUMENTSKEY)

		explicit Params(const std::string& InName,
			const std::optional<std::unordered_map<std::string, JSONData>>& InArguments = std::nullopt,
			const std::optional<RequestParamsMeta>& InMeta = std::nullopt)
			: RequestParams(InMeta),
			  Name(InName),
			  Arguments(InArguments)
		{}
	};

	explicit CallToolRequest(const CallToolRequest::Params& InParams) : RequestBase("tools/call", InParams) {}
};

// CallToolResult {
//   MSG_DESCRIPTION: "The server's response to a tool call.
//   Any errors that originate from the tool SHOULD be reported inside the result object, with `isError` set to true,
//   _not_ as an MCP protocol-level error response. Otherwise, the LLM would not be able to see that an error
//   occurred and self-correct.
//   However, any errors in _finding_ the tool, an error indicating that the server does not support tool calls, or any
//   other exceptional conditions,should be reported as an MCP error response.",
//   MSG_PROPERTIES: {
//         MSG_META: {
//           MSG_ADDITIONAL_PROPERTIES: {},
//           MSG_DESCRIPTION: "This result property is reserved by the protocol to allow clients and servers to attach
//           additional metadata to their responses.",
//           MSG_TYPE: MSG_OBJECT
//         },
//         MSG_CONTENT: {
//           MSG_ITEMS: {
//             "anyOf": [
//               {"$ref": "#/definitions/TextContent"},
//               {"$ref": "#/definitions/ImageContent"},
//               {"$ref": "#/definitions/AudioContent"},
//               {"$ref": "#/definitions/EmbeddedResource"}
//             ]
//           },
//           MSG_TYPE: MSG_ARRAY
//         },
//         MSG_IS_ERROR: {
//           MSG_DESCRIPTION:
//               "Whether the tool call ended in an error.\n\nIf not set, this
//               is assumed to be false (the call was successful).",
//           MSG_TYPE: MSG_BOOLEAN
//         }
//       },
//         MSG_REQUIRED: [MSG_CONTENT],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * The server's response to a tool call.
 * Any errors that originate from the tool SHOULD be reported inside the result
 * object, with `isError` set to true, _not_ as an MCP protocol-level error
 * response. Otherwise, the LLM would not be able to see that an error occurred
 * and self-correct. However, any errors in _finding_ the tool, an error
 * indicating that the server does not support tool calls, or any other
 * exceptional conditions, should be reported as an MCP error response.
 */
struct CallToolResponse : ResponseBase
{
	struct Result : ResultParams
	{
		std::vector<MCP::Content> Content;
		std::optional<bool> IsError{ std::nullopt };

		JKEY(CONTENTKEY, Content, "content")
		JKEY(ISERRORKEY, IsError, "isError")

		DEFINE_TYPE_JSON_DERIVED(CallToolResponse::Result, ResultParams, CONTENTKEY, ISERRORKEY)

		explicit Result(const std::vector<MCP::Content>& InContent,
			const std::optional<bool>& InIsError = std::nullopt,
			const std::optional<JSONData>& InMeta = std::nullopt)
			: ResultParams(InMeta),
			  Content(InContent),
			  IsError(InIsError)
		{}
	};

	explicit CallToolResponse(const RequestID& InRequestID, const CallToolResponse::Result& InResult)
		: ResponseBase(InRequestID, InResult)
	{}
};

// ToolListChangedNotification {
//   MSG_DESCRIPTION: "An optional notification from the server to the client, informing it that the list of tools it
//   offers has changed. This may be issued by servers without any previous subscription from the client.",
//         MSG_PROPERTIES: {
//         MSG_METHOD:
//             {MSG_CONST: MTHD_NOTIFICATIONS_TOOLS_LIST_CHANGED, MSG_TYPE:
//             MSG_STRING},
//         MSG_PARAMS: {
//           MSG_ADDITIONAL_PROPERTIES: {},
//           MSG_PROPERTIES: {
//             MSG_META: {
//               MSG_ADDITIONAL_PROPERTIES: {},
//               MSG_DESCRIPTION: "This parameter name is reserved by MCP to allow clients and servers to attach
//               additional metadata to their notifications.",
//               MSG_TYPE: MSG_OBJECT
//             }
//           },
//           MSG_TYPE: MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED: [MSG_METHOD],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * An optional notification from the server to the client, informing it that the
 * list of tools it offers has changed. Servers may issue this without
 * any previous subscription from the client.
 */
struct ToolListChangedNotification : NotificationBase
{
	ToolListChangedNotification() : NotificationBase("notifications/tools/list_changed") {}
};

// ListPromptsRequest {
//   MSG_DESCRIPTION: "Sent from the client to request a list of prompts and "
//                   "prompt templates the server has.",
//                   MSG_PROPERTIES
//     : {
//         MSG_METHOD: {MSG_CONST: MTHD_PROMPTS_LIST, MSG_TYPE: MSG_STRING},
//         MSG_PARAMS: {
//           MSG_PROPERTIES: {
//             MSG_CURSOR: {
//               MSG_DESCRIPTION:
//                   "An opaque token representing the current pagination "
//                   "position.\nIf provided, the server should return "
//                   "results starting after this cursor.",
//               MSG_TYPE: MSG_STRING
//             }
//           },
//           MSG_TYPE: MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED: [MSG_METHOD],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * Sent from the client to request a list of prompts and prompt templates the
 * server has.
 */
struct ListPromptsRequest : RequestBase
{
	explicit ListPromptsRequest(const PaginatedRequestParams& InParams) : RequestBase("prompts/list", InParams) {}
};

// ListPromptsResult {
//   MSG_DESCRIPTION
//     : "The server's response to a prompts/list request from the client.",
//         MSG_PROPERTIES
//     : {
//         MSG_META: {
//           MSG_ADDITIONAL_PROPERTIES: {},
//           MSG_DESCRIPTION: "This result property is reserved by the protocol
//           to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           MSG_TYPE: MSG_OBJECT
//         },
//         MSG_NEXT_CURSOR: {
//           MSG_DESCRIPTION: "An opaque token representing the pagination "
//                           "position after the last returned result.\nIf "
//                           "present, there may be more results available.",
//           MSG_TYPE: MSG_STRING
//         },
//         MSG_PROMPTS:
//             {MSG_ITEMS: {"$ref": "#/definitions/Prompt"}, MSG_TYPE:
//             MSG_ARRAY}
//       },
//         MSG_REQUIRED: [MSG_PROMPTS],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * The server's response to a prompts/list request from the client.
 */
struct ListPromptsResponse : ResponseBase
{
	struct Result : PaginatedResultParams
	{
		std::vector<Prompt> Prompts;

		JKEY(PROMPTSKEY, Prompts, "prompts")

		DEFINE_TYPE_JSON_DERIVED(ListPromptsResponse::Result, PaginatedResultParams, PROMPTSKEY)

		explicit Result(const std::vector<Prompt>& InPrompts,
			const std::optional<std::string>& InNextCursor = std::nullopt,
			const std::optional<std::string>& InMeta = std::nullopt)
			: PaginatedResultParams(InNextCursor, InMeta),
			  Prompts(InPrompts)
		{}
	};

	explicit ListPromptsResponse(const RequestID& InRequestID, const ListPromptsResponse::Result& InResult)
		: ResponseBase(InRequestID, InResult)
	{}
};

// GetPromptRequest {
//   MSG_DESCRIPTION: "Used by the client to get a prompt provided by the server.",
//                   MSG_PROPERTIES: {
//         MSG_METHOD: {MSG_CONST: MTHD_PROMPTS_GET, MSG_TYPE: MSG_STRING},
//         MSG_PARAMS: {
//           MSG_PROPERTIES: {
//             MSG_ARGUMENTS: {
//               MSG_ADDITIONAL_PROPERTIES: {MSG_TYPE: MSG_STRING},
//               MSG_DESCRIPTION: "Arguments to use for templating the
//               prompt.", MSG_TYPE: MSG_OBJECT
//             },
//             MSG_NAME: {
//               MSG_DESCRIPTION: "The name of the prompt or prompt template.",
//               MSG_TYPE: MSG_STRING
//             }
//           },
//           MSG_REQUIRED: [MSG_NAME],
//           MSG_TYPE: MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED: [ MSG_METHOD, MSG_PARAMS ],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * Used by the client to get a prompt provided by the server.
 */
struct GetPromptRequest : RequestBase
{
	struct Params : RequestParams
	{
		std::string Name; // The name of the prompt or prompt template.
		std::optional<std::vector<PromptArgument>> Arguments{
			std::nullopt
		}; // Arguments to use for templating the prompt.

		JKEY(NAMEKEY, Name, "name")
		JKEY(ARGUMENTSKEY, Arguments, "arguments")

		DEFINE_TYPE_JSON_DERIVED(GetPromptRequest::Params, RequestParams, NAMEKEY, ARGUMENTSKEY)

		explicit Params(const std::string& InName,
			const std::optional<std::vector<PromptArgument>>& InArguments = std::nullopt,
			const std::optional<RequestParamsMeta>& InMeta = std::nullopt)
			: RequestParams(InMeta),
			  Name(InName),
			  Arguments(InArguments)
		{}
	};

	explicit GetPromptRequest(const GetPromptRequest::Params& InParams) : RequestBase("prompts/get", InParams) {}
};

// GetPromptResult {
//   MSG_DESCRIPTION: "The server's response to a prompts/get request from the client.",
//         MSG_PROPERTIES: {
//         MSG_META: {
//           MSG_ADDITIONAL_PROPERTIES: {},
//           MSG_DESCRIPTION: "This result property is reserved by the protocol to allow clients and servers to attach
//           additional metadata to their responses.", MSG_TYPE: MSG_OBJECT
//         },
//         MSG_DESCRIPTION: {
//           MSG_DESCRIPTION: "An optional description for the prompt.",
//           MSG_TYPE: MSG_STRING
//         },
//         "messages": {
//           MSG_ITEMS: {"$ref": "#/definitions/PromptMessage"},
//           MSG_TYPE: MSG_ARRAY
//         }
//       },
//         MSG_REQUIRED: ["messages"],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * The server's response to a prompts/get request from the client.
 */
struct GetPromptResponse : ResponseBase
{
	struct Result : ResultParams
	{
		std::vector<PromptMessage> Messages;					// A list of prompt messages.
		std::optional<std::string> Description{ std::nullopt }; // An optional description for the prompt.

		JKEY(DESCRIPTIONKEY, Description, "description")
		JKEY(MESSAGESKEY, Messages, "messages")

		DEFINE_TYPE_JSON_DERIVED(GetPromptResponse::Result, ResultParams, DESCRIPTIONKEY, MESSAGESKEY)

		explicit Result(const std::vector<PromptMessage>& InMessages,
			const std::optional<std::string>& InDescription = std::nullopt,
			const std::optional<JSONData>& InMeta = std::nullopt)
			: ResultParams(InMeta),
			  Messages(InMessages),
			  Description(InDescription)
		{}
	};

	explicit GetPromptResponse(const RequestID& InRequestID, const GetPromptResponse::Result& InResult)
		: ResponseBase(InRequestID, InResult)
	{}
};

// PromptListChangedNotification {
//   MSG_DESCRIPTION: "An optional notification from the server to the client, informing it that the list of prompts it
//   offers has changed. This may be issued by servers without any previous subscription from the client.",
//     MSG_PROPERTIES: {
//         MSG_METHOD:
//             {MSG_CONST: MTHD_NOTIFICATIONS_PROMPTS_LIST_CHANGED, MSG_TYPE:
//             MSG_STRING},
//         MSG_PARAMS: {
//           MSG_ADDITIONAL_PROPERTIES: {},
//           MSG_PROPERTIES: {
//             MSG_META: {
//               MSG_ADDITIONAL_PROPERTIES: {},
//               MSG_DESCRIPTION: "This parameter name is reserved by MCP to allow clients and servers to attach
//               additional metadata to their notifications.", MSG_TYPE: MSG_OBJECT
//             }
//           },
//           MSG_TYPE: MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED: [MSG_METHOD],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * An optional notification from the server to the client, informing it that the
 * list of prompts it offers has changed. Servers may issue this without
 * any previous subscription from the client.
 */
struct PromptListChangedNotification : NotificationBase
{
	PromptListChangedNotification() : NotificationBase("notifications/prompts/list_changed") {}
};

// ListResourcesRequest {
//   MSG_DESCRIPTION: "Sent from the client to request a list of resources the server has.",
//         MSG_PROPERTIES: {
//           MSG_METHOD: {MSG_CONST: MTHD_RESOURCES_LIST, MSG_TYPE:
//           MSG_STRING}, MSG_PARAMS: {
//             MSG_PROPERTIES: {
//               MSG_CURSOR: {
//                 MSG_DESCRIPTION: "An opaque token representing the current pagination position.If provided, the
//                 server should return results starting after this cursor.",
//                 MSG_TYPE: MSG_STRING
//               }
//             },
//             MSG_TYPE: MSG_OBJECT
//           }
//         },
//                        MSG_REQUIRED: [MSG_METHOD],
//                                     MSG_TYPE: MSG_OBJECT
// };

/**
 * Sent from the client to request a list of resources the server has.
 */
struct ListResourcesRequest : RequestBase
{
	explicit ListResourcesRequest(const PaginatedRequestParams& InParams) : RequestBase("resources/list", InParams) {}
};

// ListResourcesResult {
//   MSG_DESCRIPTION: "The server's response to a resources/list request from the client.",
//         MSG_PROPERTIES: {
//         MSG_META: {
//           MSG_ADDITIONAL_PROPERTIES: {},
//           MSG_DESCRIPTION: "This result property is reserved by the protocol
//           to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           MSG_TYPE: MSG_OBJECT
//         },
//         MSG_NEXT_CURSOR: {
//           MSG_DESCRIPTION: "An opaque token representing the pagination "
//                           "position after the last returned result.\nIf "
//                           "present, there may be more results available.",
//           MSG_TYPE: MSG_STRING
//         },
//         MSG_RESOURCES:
//             {MSG_ITEMS: {"$ref": "#/definitions/Resource"}, MSG_TYPE:
//             MSG_ARRAY}
//       },
//         MSG_REQUIRED: [MSG_RESOURCES],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * The server's response to a resources/list request from the client.
 */
struct ListResourcesResponse : ResponseBase
{
	struct Result : PaginatedResultParams
	{
		std::vector<Resource> Resources;

		JKEY(RESOURCESKEY, Resources, "resources")

		DEFINE_TYPE_JSON_DERIVED(ListResourcesResponse::Result, PaginatedResultParams, RESOURCESKEY)

		explicit Result(const std::vector<Resource>& InResources,
			const std::optional<std::string>& InNextCursor = std::nullopt,
			const std::optional<std::string>& InMeta = std::nullopt)
			: PaginatedResultParams(InNextCursor, InMeta),
			  Resources(InResources)
		{}
	};

	explicit ListResourcesResponse(const RequestID& InRequestID, const ListResourcesResponse::Result& InResult)
		: ResponseBase(InRequestID, InResult)
	{}
};

// ListResourceTemplatesRequest {
//   MSG_DESCRIPTION: "Sent from the client to request a list of resource "
//                   "templates the server has.",
//                   MSG_PROPERTIES: {
//         MSG_METHOD: {MSG_CONST: MTHD_RESOURCES_TEMPLATES_LIST, MSG_TYPE:
//         MSG_STRING}, MSG_PARAMS: {
//           MSG_PROPERTIES: {
//             MSG_CURSOR: {
//               MSG_DESCRIPTION:
//                   "An opaque token representing the current pagination "
//                   "position.\nIf provided, the server should return "
//                   "results starting after this cursor.",
//               MSG_TYPE: MSG_STRING
//             }
//           },
//           MSG_TYPE: MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED: [MSG_METHOD],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * Sent from the client to request a list of resource templates the server has.
 */
struct ListResourceTemplatesRequest : RequestBase
{
	explicit ListResourceTemplatesRequest(const PaginatedRequestParams& InParams)
		: RequestBase("resources/templates/list", InParams)
	{}
};

// ListResourceTemplatesResult {
//   MSG_DESCRIPTION: "The server's response to a resources/templates/list "
//                   "request from the client.",
//                   MSG_PROPERTIES
//      : {
//         MSG_META: {
//           MSG_ADDITIONAL_PROPERTIES: {},
//           MSG_DESCRIPTION: "This result property is reserved by the protocol
//           to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           MSG_TYPE: MSG_OBJECT
//         },
//         MSG_NEXT_CURSOR: {
//           MSG_DESCRIPTION: "An opaque token representing the pagination "
//                           "position after the last returned result.\nIf "
//                           "present, there may be more results available.",
//           MSG_TYPE: MSG_STRING
//         },
//         MSG_RESOURCE_TEMPLATES: {
//           MSG_ITEMS: {"$ref": "#/definitions/ResourceTemplate"},
//           MSG_TYPE: MSG_ARRAY
//         }
//       },
//         MSG_REQUIRED: [MSG_RESOURCE_TEMPLATES],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * The server's response to a resources/templates/list request from the client.
 */
struct ListResourceTemplatesResponse : ResponseBase
{
	struct Result : PaginatedResultParams
	{
		std::vector<ResourceTemplate> ResourceTemplates;

		JKEY(RESOURCE_TEMPLATESKEY, ResourceTemplates, "resourceTemplates")

		DEFINE_TYPE_JSON_DERIVED(ListResourceTemplatesResponse::Result, PaginatedResultParams, RESOURCE_TEMPLATESKEY)

		explicit Result(const std::vector<ResourceTemplate>& InResourceTemplates,
			const std::optional<std::string>& InNextCursor = std::nullopt,
			const std::optional<std::string>& InMeta = std::nullopt)
			: PaginatedResultParams(InNextCursor, InMeta),
			  ResourceTemplates(InResourceTemplates)
		{}
	};

	explicit ListResourceTemplatesResponse(const RequestID& InRequestID,
		const ListResourceTemplatesResponse::Result& InResult)
		: ResponseBase(InRequestID, InResult)
	{}
};

// ResourceUpdatedNotification {
//   MSG_DESCRIPTION: "A notification from the server to the client, informing it that a resource has changed and may
//   need to be read again. This should only be sent if the client previously sent a resources/subscribe request.",
//      MSG_PROPERTIES: {
//         MSG_METHOD:
//             {MSG_CONST: MTHD_NOTIFICATIONS_RESOURCES_UPDATED, MSG_TYPE:
//             MSG_STRING},
//         MSG_PARAMS: {
//           MSG_PROPERTIES: {
//             MSG_URI: {
//               MSG_DESCRIPTION: "The URI of the resource that has been updated. This might be a sub-resource of the
//               one that the client actually subscribed to.",
//               MSG_FORMAT: MSG_URI,
//               MSG_TYPE: MSG_STRING
//             }
//           },
//           MSG_REQUIRED: [MSG_URI],
//           MSG_TYPE: MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED: [ MSG_METHOD, MSG_PARAMS ],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * A notification from the server to the client, informing it that a resource
 * has changed and may need to be read again. This should only be sent if the
 * client previously sent a resources/subscribe request.
 */
struct ResourceUpdatedNotification : NotificationBase
{
	struct Params : NotificationParams
	{
		MCP::URI URI; // The URI of the resource that has been updated. This might be a
					  // sub-resource of the one that the client actually subscribed to.

		JKEY(URIKEY, URI, "uri")

		DEFINE_TYPE_JSON_DERIVED(ResourceUpdatedNotification::Params, NotificationParams, URIKEY)

		explicit Params(const MCP::URI& InURI, const std::optional<NotificationParamsMeta>& InMeta = std::nullopt)
			: NotificationParams(InMeta),
			  URI(InURI)
		{}
	};

	explicit ResourceUpdatedNotification(const Params& InParams)
		: NotificationBase("notifications/resources/updated", InParams)
	{}
};

// ReadResourceRequest {
//   MSG_DESCRIPTION: "Sent from the client to the server, to read a specific resource URI.",
//         MSG_PROPERTIES: {
//           MSG_METHOD: {MSG_CONST: MTHD_RESOURCES_READ, MSG_TYPE:
//           MSG_STRING}, MSG_PARAMS: {
//             MSG_PROPERTIES: {
//               MSG_URI: {
//                 MSG_DESCRIPTION: "The URI of the resource to read. The URI can use any protocol; it is up to the
//                 server how to interpret it.",
//                 MSG_FORMAT: MSG_URI,
//                 MSG_TYPE: MSG_STRING
//               }
//             },
//             MSG_REQUIRED: [MSG_URI],
//             MSG_TYPE: MSG_OBJECT
//           }
//         },
//                        MSG_REQUIRED: [ MSG_METHOD, MSG_PARAMS ],
//                                     MSG_TYPE: MSG_OBJECT
// };

/**
 * Sent from the client to the server to read a specific resource URI.
 */
struct ReadResourceRequest : RequestBase
{
	struct Params : RequestParams
	{
		MCP::URI URI; // The URI of the resource to read. The URI can use any
					  // protocol; it is up to the server how to interpret it.

		JKEY(URIKEY, URI, "uri")

		DEFINE_TYPE_JSON_DERIVED(ReadResourceRequest::Params, RequestParams, URIKEY)

		explicit Params(const MCP::URI& InURI, const std::optional<RequestParamsMeta>& InMeta = std::nullopt)
			: RequestParams(InMeta),
			  URI(InURI)
		{}
	};

	explicit ReadResourceRequest(const ReadResourceRequest::Params& InParams) : RequestBase("resources/read", InParams)
	{}
};

// ReadResourceResult {
//   MSG_DESCRIPTION
//      : "The server's response to a resources/read request from the client.",
//         MSG_PROPERTIES
//      : {
//         MSG_META: {
//           MSG_ADDITIONAL_PROPERTIES: {},
//           MSG_DESCRIPTION: "This result property is reserved by the protocol
//           to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           MSG_TYPE: MSG_OBJECT
//         },
//         MSG_CONTENTS: {
//           MSG_ITEMS: {
//             "anyOf": [
//               {"$ref": "#/definitions/TextResourceContents"},
//               {"$ref": "#/definitions/BlobResourceContents"}
//             ]
//           },
//           MSG_TYPE: MSG_ARRAY
//         }
//       },
//         MSG_REQUIRED: [MSG_CONTENTS],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * The server's response to a resources/read request from the client.
 */
struct ReadResourceResponse : ResponseBase
{
	struct Result : ResultParams
	{
		std::vector<std::variant<TextResourceContents, BlobResourceContents>> Contents;

		JKEY(CONTENTSKEY, Contents, "contents")

		DEFINE_TYPE_JSON_DERIVED(ReadResourceResponse::Result, ResultParams, CONTENTSKEY)

		explicit Result(const std::vector<std::variant<TextResourceContents, BlobResourceContents>>& InContents,
			const std::optional<JSONData>& InMeta = std::nullopt)
			: ResultParams(InMeta),
			  Contents(InContents)
		{}
	};

	explicit ReadResourceResponse(const RequestID& InRequestID, const ReadResourceResponse::Result& InResult)
		: ResponseBase(InRequestID, InResult)
	{}
};

// SubscribeRequest {
//   MSG_DESCRIPTION:
//       "Sent from the client to request resources/updated notifications from
//       "
//         "the server whenever a particular resource changes.",
//         MSG_PROPERTIES: {
//           MSG_METHOD: {MSG_CONST: MTHD_RESOURCES_SUBSCRIBE, MSG_TYPE:
//           MSG_STRING}, MSG_PARAMS: {
//             MSG_PROPERTIES: {
//               MSG_URI: {
//                 MSG_DESCRIPTION:
//                     "The URI of the resource to subscribe to. The URI can use
//                     any protocol; it is up to the server how to
//                     interpret it.",
//                 MSG_FORMAT: MSG_URI,
//                 MSG_TYPE: MSG_STRING
//               }
//             },
//             MSG_REQUIRED: [MSG_URI],
//             MSG_TYPE: MSG_OBJECT
//           }
//         },
//                        MSG_REQUIRED: [ MSG_METHOD, MSG_PARAMS ],
//                                     MSG_TYPE: MSG_OBJECT
// };

/**
 * Sent from the client to request resources/updated notifications from the
 * server whenever a particular resource changes.
 */
struct SubscribeRequest : RequestBase
{
	struct Params : RequestParams
	{
		MCP::URI URI; // The URI of the resource to subscribe to. The URI can use
					  // any protocol; it is up to the server how to interpret it.

		JKEY(URIKEY, URI, "uri")

		DEFINE_TYPE_JSON_DERIVED(SubscribeRequest::Params, RequestParams, URIKEY)

		explicit Params(const MCP::URI& InURI, const std::optional<RequestParamsMeta>& InMeta = std::nullopt)
			: RequestParams(InMeta),
			  URI(InURI)
		{}
	};

	explicit SubscribeRequest(const SubscribeRequest::Params& InParams) : RequestBase("resources/subscribe", InParams)
	{}
};

// UnsubscribeRequest {
//   MSG_DESCRIPTION: "Sent from the client to request cancellation of "
//                   "resources/updated notifications from the server. This "
//                   "should follow a previous resources/subscribe request.",
//         MSG_PROPERTIES: {
//         MSG_METHOD: {MSG_CONST: MTHD_RESOURCES_UNSUBSCRIBE, MSG_TYPE:
//         MSG_STRING}, MSG_PARAMS: {
//           MSG_PROPERTIES: {
//             MSG_URI: {
//               MSG_DESCRIPTION: "The URI of the resource to unsubscribe
//               from.", MSG_FORMAT: MSG_URI, MSG_TYPE: MSG_STRING
//             }
//           },
//           MSG_REQUIRED: [MSG_URI],
//           MSG_TYPE: MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED: [ MSG_METHOD, MSG_PARAMS ],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * Sent from the client to request cancellation of resources/updated
 * notifications from the server. This should follow a previous
 * resources/subscribe request.
 */
struct UnsubscribeRequest : RequestBase
{
	struct Params : RequestParams
	{
		MCP::URI URI; // The URI of the resource to unsubscribe from.

		JKEY(URIKEY, URI, "uri")

		DEFINE_TYPE_JSON_DERIVED(UnsubscribeRequest::Params, RequestParams, URIKEY)

		explicit Params(const MCP::URI& InURI, const std::optional<RequestParamsMeta>& InMeta = std::nullopt)
			: RequestParams(InMeta),
			  URI(InURI)
		{}
	};

	explicit UnsubscribeRequest(const UnsubscribeRequest::Params& InParams)
		: RequestBase("resources/unsubscribe", InParams)
	{}
};

// ResourceListChangedNotification {
//   MSG_DESCRIPTION: "An optional notification from the server to the client,
//   "
//                   "informing it that the list of resources it can read "
//                   "from has changed. This may be issued by servers without "
//                   "any previous subscription from the client.",
//                   MSG_PROPERTIES
//      : {
//         MSG_METHOD: {
//           MSG_CONST: MTHD_NOTIFICATIONS_RESOURCES_LIST_CHANGED,
//           MSG_TYPE: MSG_STRING
//         },
//         MSG_PARAMS: {
//           MSG_ADDITIONAL_PROPERTIES: {},
//           MSG_PROPERTIES: {
//             MSG_META: {
//               MSG_ADDITIONAL_PROPERTIES: {},
//               MSG_DESCRIPTION: "This parameter name is reserved by MCP to
//               allow "
//                               "clients and servers to attach additional "
//                               "metadata to their notifications.",
//               MSG_TYPE: MSG_OBJECT
//             }
//           },
//           MSG_TYPE: MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED: [MSG_METHOD],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * An optional notification from the server to the client, informing it that the
 * list of resources it can read from has changed. Servers may issue this
 * without any previous subscription from the client.
 */
struct ResourceListChangedNotification : NotificationBase
{
	ResourceListChangedNotification() : NotificationBase("notifications/resources/list_changed") {}
};

// CreateMessageRequest {
//   MSG_DESCRIPTION: "A request from the server to sample an LLM via the client. The
//       client has full discretion over which model to select. The client should also inform the user before beginning
//       sampling, to allow them to inspect the request (human in the loop) and decide whether to approve it.",
//         MSG_PROPERTIES: {
//         MSG_METHOD: {MSG_CONST: MTHD_SAMPLING_CREATE_MESSAGE, MSG_TYPE:
//         MSG_STRING}, MSG_PARAMS: {
//           MSG_PROPERTIES: {
//             "includeContext": {
//               MSG_DESCRIPTION: "A request to include context from one or more MCP servers (including the caller), to
//               be attached to the prompt. The client MAY ignore this request.",
//               MSG_ENUM: [ "allServers", "none", "thisServer" ],
//               MSG_TYPE: MSG_STRING
//             },
//             "maxTokens": {
//               MSG_DESCRIPTION: "The maximum number of tokens to sample, as requested by the server. The client MAY
//               choose to sample fewer tokens than requested.",
//               MSG_TYPE: MSG_INTEGER
//             },
//             "messages": {
//               MSG_ITEMS: {"$ref": "#/definitions/SamplingMessage"},
//               MSG_TYPE: MSG_ARRAY
//             },
//             "metadata": {
//               MSG_ADDITIONAL_PROPERTIES: true,
//               MSG_DESCRIPTION: "Optional metadata to pass through to the LLM provider. The format of this metadata is
//               provider-specific.",
//               MSG_PROPERTIES: {}, MSG_TYPE: MSG_OBJECT
//             },
//             "modelPreferences": {
//               "$ref": "#/definitions/ModelPreferences",
//               MSG_DESCRIPTION: "The server's preferences for which model to select. The client MAY ignore these
//               preferences."
//             },
//             "stopSequences": {MSG_ITEMS: {MSG_TYPE: MSG_STRING},
//             MSG_TYPE: MSG_ARRAY
//             },
//             "systemPrompt": {
//               MSG_DESCRIPTION: "An optional system prompt the server wants to use for sampling. The client MAY modify
//               or omit this prompt.",
//               MSG_TYPE: MSG_STRING
//             },
//             "temperature": {MSG_TYPE: MSG_NUMBER}
//           },
//           MSG_REQUIRED: [ "maxTokens", "messages" ],
//           MSG_TYPE: MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED: [ MSG_METHOD, MSG_PARAMS ],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * A request from the server to sample an LLM via the client. The client has
 * full discretion over which model to select. The client should also inform the
 * user before beginning sampling to allow them to inspect the request (human
 * in the loop) and decide whether to approve it.
 */
struct CreateMessageRequest : RequestBase
{
	struct Params : RequestParams
	{
		std::vector<SamplingMessage> Messages;
		int64_t MaxTokens; // The maximum number of tokens to sample, as requested
						   // by the server. The client MAY choose to sample fewer
						   // tokens than requested.
		std::optional<std::string> SystemPrompt{
			std::nullopt
		}; // An optional system prompt the server wants to use for
		   // sampling. The client MAY modify or omit this prompt.
		std::optional<IncludeContext> IncludeContext{
			std::nullopt
		}; // A request to include context from one or more
		   // MCP servers (including the caller), to be attached to
		   // the prompt. The client MAY ignore this request.
		std::optional<BoundedDouble> Temperature{ std::nullopt }; // The temperature to use for sampling.
		std::optional<std::vector<std::string>> StopSequences{
			std::nullopt
		}; // A list of sequences to stop sampling at.
		std::optional<ModelPreferences> ModelPreferences{
			std::nullopt
		}; // The server's preferences for which model to select.
		   // The client MAY ignore these preferences.
		std::optional<JSONData> Metadata{ std::nullopt }; // Optional metadata to pass through to the LLM provider.
														  // The format of this metadata is provider-specific.

		JKEY(MESSAGESKEY, Messages, "messages")
		JKEY(MAXTOKENSKEY, MaxTokens, "maxTokens")
		JKEY(SYSTEMPROMPTKEY, SystemPrompt, "systemPrompt")
		JKEY(INCLUDECONTEXTKEY, IncludeContext, "includeContext")
		JKEY(TEMPERATUREKEY, Temperature, "temperature")
		JKEY(STOPSEQUENCESKEY, StopSequences, "stopSequences")
		JKEY(MODELPREFSKEY, ModelPreferences, "modelPreferences")
		JKEY(METADATAKEY, Metadata, "metadata")

		DEFINE_TYPE_JSON_DERIVED(CreateMessageRequest::Params,
			RequestParams,
			MESSAGESKEY,
			MAXTOKENSKEY,
			SYSTEMPROMPTKEY,
			INCLUDECONTEXTKEY,
			TEMPERATUREKEY,
			STOPSEQUENCESKEY,
			MODELPREFSKEY,
			METADATAKEY)

		explicit Params(const std::vector<SamplingMessage>& InMessages,
			const int64_t& InMaxTokens,
			const std::optional<std::string>& InSystemPrompt = std::nullopt,
			const std::optional<MCP::IncludeContext>& InIncludeContext = std::nullopt,
			const std::optional<double>& InTemperature = std::nullopt,
			const std::optional<std::vector<std::string>>& InStopSequences = std::nullopt,
			const std::optional<MCP::ModelPreferences>& InModelPreferences = std::nullopt,
			const std::optional<JSONData>& InMetadata = std::nullopt,
			const std::optional<RequestParamsMeta>& InMeta = std::nullopt)
			: RequestParams(InMeta),
			  Messages(InMessages),
			  MaxTokens(InMaxTokens),
			  SystemPrompt(InSystemPrompt),
			  IncludeContext(InIncludeContext),
			  Temperature(InitTemperature(InTemperature)),
			  StopSequences(InStopSequences),
			  ModelPreferences(InModelPreferences),
			  Metadata(InMetadata)
		{}

		// Utility
		static std::optional<BoundedDouble> InitTemperature(const std::optional<double>& CheckTemperature)
		{
			if (CheckTemperature)
			{
				return BoundedDouble{ CheckTemperature.value(), 0.0, 1.0, true };
			}
			return std::nullopt;
		}
	};

	explicit CreateMessageRequest(const CreateMessageRequest::Params& InParams)
		: RequestBase("sampling/createMessage", InParams)
	{}
};

// CreateMessageResult {
//   MSG_DESCRIPTION: "The client's response to a sampling/create_message request from the server. The client should
//   inform the user before returning the sampled message, to allow them to inspect the response (human in the loop)
//   and decide whether to allow the server to see it.",
//         MSG_PROPERTIES: {
//         MSG_META: {
//           MSG_ADDITIONAL_PROPERTIES: {},
//           MSG_DESCRIPTION: "This result property is reserved by the protocol to allow clients and servers to
//           attach additional metadata to their responses.", MSG_TYPE: MSG_OBJECT
//         },
//         MSG_CONTENT: {
//           "anyOf": [
//             {"$ref": "#/definitions/TextContent"},
//             {"$ref": "#/definitions/ImageContent"},
//             {"$ref": "#/definitions/AudioContent"}
//           ]
//         },
//         MSG_MODEL: {
//           MSG_DESCRIPTION: "The name of the model that generated the message.",
//           MSG_TYPE: MSG_STRING
//         },
//         MSG_ROLE: {"$ref": "#/definitions/Role"},
//         "stopReason": {
//           MSG_DESCRIPTION: "The reason why sampling stopped, if known.",
//           MSG_TYPE: MSG_STRING
//         }
//       },
//         MSG_REQUIRED: [ MSG_CONTENT, MSG_MODEL, MSG_ROLE ],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * The client's response to a sampling/create_message request from the server.
 * The client should inform the user before returning the sampled message to
 * allow them to inspect the response (human in the loop) and decide whether to
 * allow the server to see it.
 */
// TODO: Typescript extended from Result and SamplingMessage - How to convert
// properly?
struct CreateMessageResponse : ResponseBase
{
	struct Result : ResultParams
	{
		std::string Model;		// The name of the model that generated the message.
		MCP::Role ResponseRole; // The role of the response.
		std::variant<TextContent, ImageContent, AudioContent> ResponseContent; // The content of the response.
		std::optional<std::variant<MCP::StopReason, std::string>> StopReason{
			std::nullopt
		}; // The reason why sampling stopped, if known.

		JKEY(MODELKEY, Model, "model")
		JKEY(RESPONSEROLEKEY, ResponseRole, "role")
		JKEY(RESPONSECONTENTKEY, ResponseContent, "content")

		DEFINE_TYPE_JSON_DERIVED(CreateMessageResponse::Result,
			ResultParams,
			MODELKEY,
			RESPONSEROLEKEY,
			RESPONSECONTENTKEY)

		explicit Result(std::string  InModel,
			const Role& InResponseRole,
			const std::variant<TextContent, ImageContent, AudioContent>& InResponseContent,
			const std::optional<std::variant<MCP::StopReason, std::string>>& InStopReason = std::nullopt,
			const std::optional<JSONData>& InMeta = std::nullopt)
			: ResultParams(InMeta),
			  Model(std::move(InModel)),
			  ResponseRole(InResponseRole),
			  ResponseContent(InResponseContent),
			  StopReason(InStopReason)
		{}
	};

	explicit CreateMessageResponse(const RequestID& InRequestID, const CreateMessageResponse::Result& InResult)
		: ResponseBase(InRequestID, InResult)
	{}
};

// ListRootsRequest {
//     MSG_DESCRIPTION: "Sent from the server to request a list of root URIs from the
//         client. Roots allow servers to ask for specific directories or files to operate on. A common
//           example for roots is providing a set of repositories or directories a server should operate on.
//           This request is typically used when the server needs to understand the file system structure or access
//           specific locations that the client has permission to read from.",
//           MSG_PROPERTIES: {
//               MSG_METHOD: {MSG_CONST: MTHD_ROOTS_LIST, MSG_TYPE: MSG_STRING},
//               MSG_PARAMS: {
//                   MSG_ADDITIONAL_PROPERTIES: {},
//                   MSG_PROPERTIES: {
//                       MSG_META: {
//                           MSG_PROPERTIES: {
//                               MSG_PROGRESS_TOKEN: {
//                                   "$ref": "#/definitions/ProgressToken",
//                                   MSG_DESCRIPTION:
//                                       "If specified, the caller is requesting
//                                       out-of-band progress notifications for this request (as represented by
//                                       notifications/progress). The value of this parameter is an opaque token
//                                       that will be attached to any subsequent notifications. The receiver is not
//                                       obligated to provide these notifications."
//                               }
//                           },
//                           MSG_TYPE: MSG_OBJECT
//                       }
//                   },
//                   MSG_TYPE: MSG_OBJECT
//               }
//           },
//                          MSG_REQUIRED: [MSG_METHOD],
//                                       MSG_TYPE: MSG_OBJECT
// };

/**
 * Sent from the server to request a list of root URIs from the client. Roots
 * allow servers to ask for specific directories or files to operate on. A
 * common example for roots is providing a set of repositories or directories a
 * server should operate on. This request is typically used when the server
 * needs to understand the file system structure or access specific locations
 * that the client has permission to read from.
 */
struct ListRootsRequest : RequestBase
{
	explicit ListRootsRequest(const PaginatedRequestParams& InParams) : RequestBase("roots/list", InParams) {}
};

// ListRootsResult {
//     MSG_DESCRIPTION: "The client's response to a roots/list request from the
//     server.\nThis result
//     "
//                     "contains an array of Root objects, each representing a
//                     root directory\nor file that the server can operate
//                     on.", MSG_PROPERTIES
//        : {
//             MSG_META: {
//                 MSG_ADDITIONAL_PROPERTIES: {},
//                 MSG_DESCRIPTION: "This result property is reserved by the
//                 protocol to allow clients
//                 "
//                                "and servers to attach additional metadata to
//                                their responses.",
//                 MSG_TYPE: MSG_OBJECT
//             },
//             MSG_ROOTS: {MSG_ITEMS: {"$ref": "#/definitions/Root"}, MSG_TYPE:
//             MSG_ARRAY}
//         },
//           MSG_REQUIRED: [MSG_ROOTS],
//                        MSG_TYPE: MSG_OBJECT
// };

/**
 * The client's response to a roots/list request from the server. This result
 * contains an array of Root objects, each representing a root directory or file
 * that the server can operate on.
 */
struct ListRootsResponse : ResponseBase
{
	struct Result : PaginatedResultParams
	{
		std::vector<Root> Roots;

		JKEY(ROOTSKEY, Roots, "roots")

		DEFINE_TYPE_JSON_DERIVED(ListRootsResponse::Result, PaginatedResultParams, ROOTSKEY)

		explicit Result(const std::vector<Root>& InRoots,
			const std::optional<std::string>& InNextCursor = std::nullopt,
			const std::optional<JSONData>& InMeta = std::nullopt)
			: PaginatedResultParams(InNextCursor, InMeta),
			  Roots(InRoots)
		{}
	};

	explicit ListRootsResponse(const RequestID& InRequestID, const ListRootsResponse::Result& InResult)
		: ResponseBase(InRequestID, InResult)
	{}
};

// RootsListChangedNotification {
//     MSG_DESCRIPTION: "A notification from the client to the server,
//     informing it that the "
//                     "list of roots has changed.\nThis notification should be
//                     sent whenever the client adds, removes, or modifies
//                     any root.\nThe server should then request an updated
//                     list of roots using the ListRootsRequest.",
//                     MSG_PROPERTIES
//        : {
//             MSG_METHOD: {MSG_CONST: MTHD_NOTIFICATIONS_ROOTS_LIST_CHANGED,
//             MSG_TYPE: MSG_STRING}, MSG_PARAMS: {
//                 MSG_ADDITIONAL_PROPERTIES: {},
//                 MSG_PROPERTIES: {
//                     MSG_META: {
//                         MSG_ADDITIONAL_PROPERTIES: {},
//                         MSG_DESCRIPTION:
//                             "This parameter name is reserved by MCP to allow
//                             clients and servers to attach additional
//                             metadata to their notifications.",
//                         MSG_TYPE: MSG_OBJECT
//                     }
//                 },
//                 MSG_TYPE: MSG_OBJECT
//             }
//         },
//           MSG_REQUIRED: [MSG_METHOD],
//                        MSG_TYPE: MSG_OBJECT
// };

/**
 * A notification from the client to the server, informing it that the list of
 * roots has changed. This notification should be sent whenever the client adds,
 * removes, or modifies any root. The server should then request an updated list
 * of roots using the ListRootsRequest.
 */
struct RootsListChangedNotification : NotificationBase
{
	RootsListChangedNotification() : NotificationBase("notifications/roots/list_changed") {}
};

// SetLevelRequest {
//   MSG_DESCRIPTION: "A request from the client to the server, to enable or adjust logging.",
//         MSG_PROPERTIES: {
//           MSG_METHOD: {MSG_CONST: MTHD_LOGGING_SET_LEVEL, MSG_TYPE:
//           MSG_STRING}, MSG_PARAMS: {
//             MSG_PROPERTIES: {
//               MSG_LEVEL: {
//                 "$ref": "#/definitions/LoggingLevel",
//                 MSG_DESCRIPTION: "The level of logging that the client wants to receive from the server. The
//                 server should send all logs at this level and higher (i.e., more severe) to the client as
//                 notifications/message."
//               }
//             },
//             MSG_REQUIRED: [MSG_LEVEL],
//             MSG_TYPE: MSG_OBJECT
//           }
//         },
//                        MSG_REQUIRED: [ MSG_METHOD, MSG_PARAMS ],
//                                     MSG_TYPE: MSG_OBJECT
// };

/**
 * A request from the client to the server to enable or adjust logging.
 */
struct SetLevelRequest : RequestBase
{
	struct Params : RequestParams
	{
		LoggingLevel Level; // The level of logging that the client wants to receive
							// from the server. The server should send all logs at
							// this level and higher (i.e., more severe) to the
							// client as notifications/messages.

		JKEY(LEVELKEY, Level, "level")

		DEFINE_TYPE_JSON_DERIVED(SetLevelRequest::Params, RequestParams, LEVELKEY)

		explicit Params(const LoggingLevel& InLevel, const std::optional<RequestParamsMeta>& InMeta = std::nullopt)
			: RequestParams(InMeta),
			  Level(InLevel)
		{}
	};

	explicit SetLevelRequest(const SetLevelRequest::Params& InParams) : RequestBase("logging/setLevel", InParams) {}
};

// LoggingMessageNotification {
//   MSG_DESCRIPTION: "Notification of a log message passed from server to client. If no logging/setLevel request
//   has been sent from the client, the server MAY decide which messages to send automatically.",
//         MSG_PROPERTIES: {
//           MSG_METHOD: {
//           MSG_CONST: MTHD_NOTIFICATIONS_MESSAGE,
//           MSG_TYPE: MSG_STRING
//           },
//           MSG_PARAMS: {
//             MSG_PROPERTIES: {
//               MSG_DATA: {
//                 MSG_DESCRIPTION: "The data to be logged, such as a string message or an object. Any JSON
//                 serializable type is allowed here."
//               },
//               MSG_LEVEL: {
//                 "$ref": "#/definitions/LoggingLevel",
//                 MSG_DESCRIPTION: "The severity of this log message."
//               },
//               MSG_LOGGER: {
//                 MSG_DESCRIPTION: "An optional name of the logger issuing this message.",
//                 MSG_TYPE: MSG_STRING
//               }
//             },
//             MSG_REQUIRED: [ MSG_DATA, MSG_LEVEL ],
//             MSG_TYPE: MSG_OBJECT
//           }
//         },
//                        MSG_REQUIRED: [ MSG_METHOD, MSG_PARAMS ],
//                                     MSG_TYPE: MSG_OBJECT
// };

/**
 * Notification of a log message passed from server to client. If no
 * logging/setLevel request has been sent from the client, the server MAY decide
 * which messages to send automatically.
 */
struct LoggingMessageNotification : NotificationBase
{
	struct Params : NotificationParams
	{
		LoggingLevel Level;								   // The severity of this log message.
		JSONData Data;									   // The data to be logged, such as a string message or an
														   // object. Any JSON serializable type is allowed here.
		std::optional<std::string> Logger{ std::nullopt }; // An optional name of the logger issuing this message.

		JKEY(LEVELKEY, Level, "level")
		JKEY(LOGGERKEY, Logger, "logger")
		JKEY(DATAKEY, Data, "data")

		DEFINE_TYPE_JSON_DERIVED(LoggingMessageNotification::Params, NotificationParams, LEVELKEY, LOGGERKEY, DATAKEY)

		explicit Params(const LoggingLevel& InLevel,
			const JSONData& InData,
			const std::optional<std::string>& InLogger = std::nullopt,
			const std::optional<NotificationParamsMeta>& InMeta = std::nullopt)
			: NotificationParams(InMeta),
			  Level(InLevel),
			  Data(std::move(InData)),
			  Logger(InLogger)
		{}
	};

	explicit LoggingMessageNotification(const LoggingMessageNotification::Params& InParams)
		: NotificationBase("notifications/message", InParams)
	{}
};

// ProgressNotification {
//   MSG_DESCRIPTION: "An out-of-band notification used to inform the receiver of a progress update for a
//   long-running request.",
//   MSG_PROPERTIES: {
//         MSG_METHOD: {MSG_CONST: MTHD_NOTIFICATIONS_PROGRESS, MSG_TYPE:
//         MSG_STRING}, MSG_PARAMS: {
//           MSG_PROPERTIES: {
//             MSG_MESSAGE: {
//               MSG_DESCRIPTION:
//                   "An optional message describing the current progress.",
//               MSG_TYPE: MSG_STRING
//             },
//             MSG_PROGRESS: {
//               MSG_DESCRIPTION:
//                   "The progress thus far. This should increase every time "
//                   "progress is made, even if the total is unknown.",
//               MSG_TYPE: MSG_NUMBER
//             },
//             MSG_PROGRESS_TOKEN: {
//               "$ref": "#/definitions/ProgressToken",
//               MSG_DESCRIPTION:
//                   "The progress token which was given in the initial request,
//                   used to associate this notification with the request
//                   that is proceeding."
//             },
//             "total": {
//               MSG_DESCRIPTION: "Total number of items to process (or total progress required), if known.",
//               MSG_TYPE: MSG_NUMBER
//             }
//           },
//           MSG_REQUIRED: [ MSG_PROGRESS, MSG_PROGRESS_TOKEN ],
//           MSG_TYPE: MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED: [ MSG_METHOD, MSG_PARAMS ],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * An out-of-band notification used to inform the receiver of a progress update
 * for a long-running request.
 */
struct ProgressNotification : NotificationBase
{
	struct Params : NotificationParams
	{
		std::optional<std::string> Message{ std::nullopt }; // An optional message describing the current progress.
		MCP::ProgressToken ProgressToken;					// The progress token which was given in the initial
															// request, used to associate this notification with the
															// request that is proceeding.
		BoundedDouble Progress{ 0.0, 0.0, 1.0, true }; // Range from 0-1. The progress thus far. This should increase
													   // every time progress is made, even if the total is unknown.
		std::optional<int64_t> Total{ std::nullopt };  // Total number of items to process (or total
													   // progress required), if known.

		JKEY(MESSAGEKEY, Message, "message")
		JKEY(PROGRESSTOKENKEY, ProgressToken, "progressToken")
		JKEY(PROGRESSKEY, Progress, "progress")
		JKEY(TOTALKEY, Total, "total")

		DEFINE_TYPE_JSON_DERIVED(ProgressNotification::Params,
			NotificationParams,
			MESSAGEKEY,
			PROGRESSTOKENKEY,
			PROGRESSKEY,
			TOTALKEY)

		Params(const std::optional<std::string>& InMessage,
			const MCP::ProgressToken& InProgressToken,
			const double InProgress,
			const std::optional<int64_t>& InTotal,
			const std::optional<NotificationParamsMeta>& InMeta = std::nullopt)
			: NotificationParams(InMeta),
			  Message(InMessage),
			  ProgressToken(InProgressToken),
			  Progress(BoundedDouble{ InProgress, 0.0, 1.0, true }),
			  Total(InTotal)
		{}
	};

	explicit ProgressNotification(const ProgressNotification::Params& InParams)
		: NotificationBase("notifications/progress", InParams)
	{}
};

// CancelledNotification {
//   MSG_DESCRIPTION: "This notification can be sent by either side to indicate that it is cancelling a previously
//   issued request. The request SHOULD still be in-flight, but due to communication latency, it is always possible
//   that this notification MAY arrive after the request has already finished. This notification indicates that the
//   result will be unused, so any associated processing SHOULD cease. A client MUST NOT attempt to cancel its
//   `initialize` request.",
//         MSG_PROPERTIES: {
//         MSG_METHOD: {
//				MSG_CONST: MTHD_NOTIFICATIONS_CANCELLED,
//				MSG_TYPE: MSG_STRING
//         },
//         MSG_PARAMS: {
//           MSG_PROPERTIES: {
//             "reason": {
//               MSG_DESCRIPTION: "An optional string describing the reason for the cancellation. This MAY be logged
//               or presented to the user.", MSG_TYPE: MSG_STRING
//             },
//             MSG_REQUEST_ID: {
//               "$ref": "#/definitions/RequestID",
//               MSG_DESCRIPTION: "The ID of the request to cancel.
//               This MUST correspond to the ID of a request previously issued in the same direction."
//             }
//           },
//           MSG_REQUIRED: [MSG_REQUEST_ID],
//           MSG_TYPE: MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED: [ MSG_METHOD, MSG_PARAMS ],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * This notification can be sent by either side to indicate that it is
 * cancelling a previously issued request. The request SHOULD still be
 * in-flight, but due to communication latency, it is always possible that this
 * notification MAY arrive after the request has already finished. This
 * notification indicates that the result will be unused, so any associated
 * processing SHOULD cease. A client MUST NOT attempt to cancel its `initialize`
 * request.
 */
struct CancelledNotification : NotificationBase
{
	struct Params : NotificationParams
	{
		RequestID CancelRequestID;						   // The ID of the request to cancel. This MUST
														   // correspond to the ID of a request previously
														   // issued in the same direction.
		std::optional<std::string> Reason{ std::nullopt }; // An optional string describing the reason for the
														   // cancellation. This MAY be logged or presented to the user.

		JKEY(CANCELREQUESTIDKEY, CancelRequestID, "requestId")
		JKEY(REASONKEY, Reason, "reason")

		DEFINE_TYPE_JSON_DERIVED(CancelledNotification::Params, NotificationParams, CANCELREQUESTIDKEY, REASONKEY)

		explicit Params(const RequestID& InCancelRequestID,
			const std::optional<std::string>& InReason = std::nullopt,
			const std::optional<NotificationParamsMeta>& InMeta = std::nullopt)
			: NotificationParams(InMeta),
			  CancelRequestID(InCancelRequestID),
			  Reason(InReason)
		{}
	};

	explicit CancelledNotification(const CancelledNotification::Params& InParams)
		: NotificationBase("notifications/cancelled", InParams)
	{}
};

// CompleteRequest {
//   MSG_DESCRIPTION: "A request from the client to the server, to ask fo completion options.",
//      MSG_PROPERTIES: {
//         MSG_METHOD: {MSG_CONST: MTHD_COMPLETION_COMPLETE, MSG_TYPE:
//         MSG_STRING}, MSG_PARAMS: {
//           MSG_PROPERTIES: {
//             MSG_ARGUMENT: {
//               MSG_DESCRIPTION: "The argument's information",
//               MSG_PROPERTIES: {
//                 MSG_NAME: {
//                   MSG_DESCRIPTION: "The name of the argument",
//                   MSG_TYPE: MSG_STRING
//                 },
//                 MSG_VALUE: {
//                   MSG_DESCRIPTION: "The value of the argument to use for "
//                                   "completion matching.",
//                   MSG_TYPE: MSG_STRING
//                 }
//               },
//               MSG_REQUIRED: [ MSG_NAME, MSG_VALUE ],
//               MSG_TYPE: MSG_OBJECT
//             },
//             MSG_REF: {
//               "anyOf": [
//                 {"$ref": "#/definitions/PromptReference"},
//                 {"$ref": "#/definitions/ResourceReference"}
//               ]
//             }
//           },
//           MSG_REQUIRED: [ MSG_ARGUMENT, MSG_REF ],
//           MSG_TYPE: MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED: [ MSG_METHOD, MSG_PARAMS ],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * A request from the client to the server to ask for completion options.
 */
struct CompleteRequest : RequestBase
{
	struct Params : RequestParams
	{
		std::variant<PromptReference, ResourceReference> Reference;

		struct CompleteArgument
		{
			std::string Name;  // The name of the argument
			std::string Value; // The value of the argument to use for completion matching.

			JKEY(NAMEKEY, Name, "name")
			JKEY(VALUEKEY, Value, "value")

			DEFINE_TYPE_JSON(CompleteRequest::Params::CompleteArgument, NAMEKEY, VALUEKEY)

			explicit CompleteArgument(const std::string& InName, const std::string& InValue)
				: Name(InName),
				  Value(InValue)
			{}
		} Argument;

		JKEY(REFERENCEKEY, Reference, "ref")
		JKEY(ARGUMENTKEY, Argument, "argument")

		DEFINE_TYPE_JSON_DERIVED(CompleteRequest::Params, RequestParams, REFERENCEKEY, ARGUMENTKEY)

		explicit Params(const std::variant<PromptReference, ResourceReference>& InReference,
			const CompleteArgument& InArgument,
			const std::optional<RequestParamsMeta>& InMeta = std::nullopt)
			: RequestParams(InMeta),
			  Reference(InReference),
			  Argument(InArgument)
		{}
	};

	explicit CompleteRequest(const CompleteRequest::Params& InParams) : RequestBase("completion/complete", InParams) {}
};

// CompleteResult {
//   MSG_DESCRIPTION: "The server's response to a completion/complete request",
//                   MSG_PROPERTIES
//      : {
//         MSG_META: {
//           MSG_ADDITIONAL_PROPERTIES: {},
//           MSG_DESCRIPTION: "This result property is reserved by the protocol
//           to "
//                           "allow clients and servers to attach additional "
//                           "metadata to their responses.",
//           MSG_TYPE: MSG_OBJECT
//         },
//         MSG_COMPLETION: {
//           MSG_PROPERTIES: {
//             MSG_HAS_MORE: {
//               MSG_DESCRIPTION:
//                   "Indicates whether there are additional completion options
//                   beyond those provided in the current response, even if
//                   the exact total is unknown.",
//               MSG_TYPE: MSG_BOOLEAN
//             },
//             MSG_TOTAL: {
//               MSG_DESCRIPTION:
//                   "The total number of completion options available. This can exceed the number of values
//                   actually sent in the response.",
//               MSG_TYPE: MSG_INTEGER
//             },
//             MSG_VALUES: {
//               MSG_DESCRIPTION:
//                   "An array of completion values. Must not exceed 100
//                   items.",
//               MSG_ITEMS: {MSG_TYPE: MSG_STRING},
//               MSG_TYPE: MSG_ARRAY
//             }
//           },
//           MSG_REQUIRED: [MSG_VALUES],
//           MSG_TYPE: MSG_OBJECT
//         }
//       },
//         MSG_REQUIRED: [MSG_COMPLETION],
//                      MSG_TYPE: MSG_OBJECT
// };

/**
 * The server's response to a completion/complete request
 */
struct CompleteResponse : ResponseBase
{
	struct Result : ResultParams
	{
		struct Completion
		{
			static constexpr size_t MAX_VALUES = 100;
			std::array<std::string, MAX_VALUES> Values;	  // An array of completion values. Must not exceed 100 items.
			std::optional<int64_t> Total{ std::nullopt }; // The total number of completion options available. This can
														  // exceed the number of values actually sent in the response.
			std::optional<bool> HasMore{ std::nullopt };  // Indicates whether there are additional completion
														  // options beyond those provided in the current response,
														  // even if the exact total is unknown.

			JKEY(VALUESKEY, Values, "values")
			JKEY(TOTALKEY, Total, "total")
			JKEY(HASMOREKEY, HasMore, "hasMore")

			DEFINE_TYPE_JSON(CompleteResponse::Result::Completion, VALUESKEY, TOTALKEY, HASMOREKEY)

			explicit Completion(const std::array<std::string, MAX_VALUES>& InValues,
				const std::optional<int64_t>& InTotal = std::nullopt,
				const std::optional<bool>& InHasMore = std::nullopt)
				: Values(InValues),
				  Total(InTotal),
				  HasMore(InHasMore)
			{}
		} CompletionData;

		JKEY(COMPLETIONDATAKEY, CompletionData, "completion")

		DEFINE_TYPE_JSON_DERIVED(CompleteResponse::Result, ResultParams, COMPLETIONDATAKEY)

		explicit Result(const Completion& InCompletionData, const std::optional<JSONData>& InMeta = std::nullopt)
			: ResultParams(InMeta),
			  CompletionData(InCompletionData)
		{}
	};

	explicit CompleteResponse(const RequestID& InRequestID, const CompleteResponse::Result& InResult)
		: ResponseBase(InRequestID, InResult)
	{}
};

// Union types for polymorphic handling
using AnyRequest = std::variant<InitializeRequest,
	PingRequest,
	ListToolsRequest,
	CallToolRequest,
	ListPromptsRequest,
	GetPromptRequest,
	ListResourcesRequest,
	ReadResourceRequest,
	SubscribeRequest,
	UnsubscribeRequest,
	CreateMessageRequest,
	ListRootsRequest,
	SetLevelRequest,
	CompleteRequest>;

using AnyResponse = std::variant<InitializeResponse,
	PingResponse,
	ListToolsResponse,
	CallToolResponse,
	ListPromptsResponse,
	GetPromptResponse,
	ListResourcesResponse,
	ReadResourceResponse,
	CreateMessageResponse,
	ListRootsResponse,
	CompleteResponse>;

using AnyNotification = std::variant<InitializedNotification,
	ProgressNotification,
	CancelledNotification,
	ResourceListChangedNotification,
	ResourceUpdatedNotification,
	PromptListChangedNotification,
	ToolListChangedNotification,
	RootsListChangedNotification,
	LoggingMessageNotification>;

MCP_NAMESPACE_END