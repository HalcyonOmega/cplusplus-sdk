#pragma once

#include <optional>
#include <string_view>

#include "CoreSDK/Common/Macros.h"
#include "CoreSDK/Messages/RequestBase.h"
#include "CoreSDK/Messages/ResponseBase.h"

MCP_NAMESPACE_BEGIN

/**
 * Primitive schema definition for boolean fields.
 */
struct BooleanSchema
{
	const std::string_view Type{ "boolean" };
	std::optional<std::string> Title;
	std::optional<std::string> Description;
	std::optional<bool> Default;

	JSON_KEY(TYPEKEY, Type, "type")
	JSON_KEY(TITLEKEY, Title, "title")
	JSON_KEY(DESCRIPTIONKEY, Description, "description")
	JSON_KEY(DEFAULTKEY, Default, "default")

	DEFINE_TYPE_JSON(BooleanSchema, TYPEKEY, TITLEKEY, DESCRIPTIONKEY, DEFAULTKEY)
};

enum class EStringFormat
{
	Email,
	URI,
	Date,
	DateTime
};

DEFINE_ENUM_JSON(EStringFormat,
	{ EStringFormat::Email, "email" },
	{ EStringFormat::URI, "uri" },
	{ EStringFormat::Date, "date" },
	{ EStringFormat::DateTime, "date-time" })

/**
 * Primitive schema definition for string fields.
 */
struct StringSchema
{
	const std::string_view Type{ "string" };
	std::optional<std::string> Title;
	std::optional<std::string> Description;
	std::optional<uint32_t> MinLength;
	std::optional<uint32_t> MaxLength;
	std::optional<EStringFormat> Format;

	JSON_KEY(TYPEKEY, Type, "type")
	JSON_KEY(TITLEKEY, Title, "title")
	JSON_KEY(DESCRIPTIONKEY, Description, "description")
	JSON_KEY(MINLENGTHKEY, MinLength, "minLength")
	JSON_KEY(MAXLENGTHKEY, MaxLength, "maxLength")
	JSON_KEY(FORMATKEY, Format, "format")

	DEFINE_TYPE_JSON(StringSchema, TYPEKEY, TITLEKEY, DESCRIPTIONKEY, MINLENGTHKEY, MAXLENGTHKEY, FORMATKEY)
};

enum class ENumberType
{
	Number,
	Integer
};

DEFINE_ENUM_JSON(ENumberType, { ENumberType::Number, "number" }, { ENumberType::Integer, "integer" })

/**
 * Primitive schema definition for number fields.
 */
struct NumberSchema
{
	ENumberType Type;
	std::optional<std::string> Title;
	std::optional<std::string> Description;
	std::optional<double> Minimum;
	std::optional<double> Maximum;

	JSON_KEY(TYPEKEY, Type, "type")
	JSON_KEY(TITLEKEY, Title, "title")
	JSON_KEY(DESCRIPTIONKEY, Description, "description")
	JSON_KEY(MINIMUMKEY, Minimum, "minimum")
	JSON_KEY(MAXIMUMKEY, Maximum, "maximum")

	DEFINE_TYPE_JSON(NumberSchema, TYPEKEY, TITLEKEY, DESCRIPTIONKEY, MINIMUMKEY, MAXIMUMKEY)
};

/**
 * Primitive schema definition for enum fields.
 */
struct EnumSchema
{
	const std::string_view Type{ "string" };
	std::optional<std::string> Title;
	std::optional<std::string> Description;
	std::vector<std::string> Enum;
	std::optional<std::vector<std::string>> EnumNames;

	JSON_KEY(TYPEKEY, Type, "type")
	JSON_KEY(TITLEKEY, Title, "title")
	JSON_KEY(DESCRIPTIONKEY, Description, "description")
	JSON_KEY(ENUMKEY, Enum, "enum")
	JSON_KEY(ENUMNAMESKEY, EnumNames, "enumNames")

	DEFINE_TYPE_JSON(EnumSchema, TYPEKEY, TITLEKEY, DESCRIPTIONKEY, ENUMKEY, ENUMNAMESKEY)
};

/**
 * Union of all primitive schema definitions.
 */
using PrimitiveSchemaDefinitions = std::variant<BooleanSchema, StringSchema, NumberSchema, EnumSchema>;

/**
 * A request from the server to elicit user input via the client.
 * The client should present the message and form fields to the user.
 */
struct ElicitRequest : RequestBase
{
	struct Params : RequestParams
	{
		/**
		 * The message to present to the user.
		 */
		std::string Message;

		/**
		 * The schema for the requested user input.
		 */
		struct FRequestedSchema
		{
			const std::string_view Type{ "object" };
			std::unordered_map<std::string, PrimitiveSchemaDefinitions> Properties;
			std::optional<std::vector<std::string>> Required;

			JSON_KEY(TYPEKEY, Type, "type")
			JSON_KEY(PROPERTIESKEY, Properties, "properties")
			JSON_KEY(REQUIREDKEY, Required, "required")

			DEFINE_TYPE_JSON(FRequestedSchema, TYPEKEY, PROPERTIESKEY, REQUIREDKEY)
		} RequestedSchema;

		JSON_KEY(MESSAGEKEY, Message, "message")
		JSON_KEY(REQUESTEDSCHEMAKEY, requestedSchema, "requestedSchema")

		DEFINE_TYPE_JSON_DERIVED(ElicitRequest::Params, RequestParams, MESSAGEKEY, REQUESTEDSCHEMAKEY)

		explicit Params(const std::string& InMessage,
			const FRequestedSchema& InRequestedSchema,
			const std::optional<RequestParamsMeta>& InMeta = std::nullopt)
			: RequestParams(InMeta),
			  Message(InMessage),
			  RequestedSchema(InRequestedSchema)
		{}
	};

	explicit ElicitRequest(const ElicitRequest::Params& InParams) : RequestBase("elicitation/create", InParams) {}
};

enum class EElicitationAction
{
	Accept,
	Decline,
	Cancel
};

DEFINE_ENUM_JSON(EElicitationAction,
	{ EElicitationAction::Accept, "accept" },
	{ EElicitationAction::Decline, "decline" },
	{ EElicitationAction::Cancel, "cancel" })

/**
 * The client's response to an elicitation/create request from the server.
 */
struct ElicitResponse : ResponseBase
{
	struct Result : ResultParams
	{
		/**
		 * The user's response action.
		 */
		EElicitationAction Action;

		/**
		 * The collected user input content (only present if the action is "accept").
		 */
		std::optional<std::unordered_map<std::string, JSONData>> Content{ std::nullopt };

		JSON_KEY(ACTIONKEY, Action, "action")
		JSON_KEY(CONTENTKEY, Content, "content")

		DEFINE_TYPE_JSON_DERIVED(ElicitResponse::Result, ResultParams, ACTIONKEY, CONTENTKEY)

		explicit Result(const EElicitationAction InAction,
			const std::optional<std::unordered_map<std::string, JSONData>>& InContent,
			const std::optional<JSONData>& InMeta = std::nullopt)
			: ResultParams(InMeta),
			  Action(InAction),
			  Content(InContent)
		{}
	};

	explicit ElicitResponse(const RequestID& InRequestID, const ElicitResponse::Result& InResult)
		: ResponseBase(InRequestID, InResult)
	{}
};

MCP_NAMESPACE_END