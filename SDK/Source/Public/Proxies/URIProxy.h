#pragma once

#include <Poco/URI.h>

#include "../CoreSDK/Common/Macros.h"

template <> struct std::hash<Poco::URI>
{
	std::size_t operator()(const Poco::URI& InURI) const noexcept { return std::hash<string>{}(InURI.toString()); }
};

MCP_NAMESPACE_BEGIN

// TODO: @HalcyonOmega create URI, URIFile, & URITemplate classes
using URI = Poco::URI;
using URIFile = Poco::URI;
using URITemplate = Poco::URI;

// TODO: @HalcyonOmega Implement proper URL class
struct URL
{
	std::string Href;
	std::string Origin;

	explicit URL(const std::string& InURLString) : Href(InURLString)
	{
		// TODO: Proper URL parsing
		Origin = InURLString; // Simplified
	}

	URL(const std::string& InRelative, const URL& InBase)
	{
		// TODO: Proper relative URL resolution
		Href = InBase.Href + "/" + InRelative;
		Origin = InBase.Origin;
	}
};

// Proper RFC 3986 percent-encoding implementation
// std::string EncodeURI(const std::string& InValue);
// std::string EncodeURIComponent(const std::string& InValue);
//
// using Variables = std::unordered_map<std::string, std::variant<std::string, std::vector<std::string>>>;
//
// class URITemplate
// {
// public:
// 	explicit URITemplate(const std::string& InTemplateStr);
// 	/**
// 	 * Returns true if the given string contains any URI template expressions.
// 	 * A template expression is a sequence of characters enclosed in curly
// 	 * braces, like {foo} or {?bar}.
// 	 */
// 	static bool IsTemplate(const std::string& InString);
// 	[[nodiscard]] std::vector<std::string> GetVariableNames() const;
// 	[[nodiscard]] std::string ToString() const;
// 	[[nodiscard]] std::string Expand(const Variables& InVariables) const;
// 	[[nodiscard]] Variables Match(const std::string& InURI) const;
//
// private:
// 	struct TemplatePart
// 	{
// 		std::string m_Name;
// 		std::string m_OperatorChar;
// 		std::vector<std::string> m_Names;
// 		bool m_Exploded;
// 	};
//
// 	using Part = std::variant<std::string, TemplatePart>;
//
// 	std::string m_Template;
// 	std::vector<Part> m_Parts;
//
// 	static void ValidateLength(const std::string& InString, size_t InMax, const std::string& InContext);
// 	std::vector<Part> Parse(const std::string& InTemplateStr);
// 	[[nodiscard]] std::string GetOperator(const std::string& InExpression) const;
// 	[[nodiscard]] std::vector<std::string> GetNames(const std::string& InExpression) const;
// 	[[nodiscard]] std::string EncodeValue(const std::string& InValue, const std::string& InOperatorChar) const;
// 	[[nodiscard]] std::string ExpandPart(const TemplatePart& InPart, const Variables& InVariables) const;
// 	[[nodiscard]] std::string EscapeRegExp(const std::string& InString) const;
// 	[[nodiscard]] std::vector<std::pair<std::string, std::string>> PartToRegExp(const TemplatePart& InPart) const;
// };

MCP_NAMESPACE_END