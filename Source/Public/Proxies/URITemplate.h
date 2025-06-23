#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "../CoreSDK/Common/Macros.h"

MCP_NAMESPACE_BEGIN

// Proper RFC 3986 percent-encoding implementation
std::string EncodeURI(const std::string& InValue);
std::string EncodeURIComponent(const std::string& InValue);

using Variables =
    std::unordered_map<std::string, std::variant<std::string, std::vector<std::string>>>;

class URITemplate {
  public:
    /**
     * Returns true if the given string contains any URI template expressions.
     * A template expression is a sequence of characters enclosed in curly
     * braces, like {foo} or {?bar}.
     */
    static bool IsTemplate(const std::string& InString);
    std::vector<std::string> GetVariableNames() const;
    explicit URITemplate(const std::string& InTemplateStr);
    std::string ToString() const;
    std::string Expand(const Variables& InVariables) const;
    Variables Match(const std::string& InURI) const;

  private:
    struct TemplatePart {
        std::string m_Name;
        std::string m_OperatorChar;
        std::vector<std::string> m_Names;
        bool m_Exploded;
    };

    using Part = std::variant<std::string, TemplatePart>;

    std::string m_Template;
    std::vector<Part> m_Parts;

    static void ValidateLength(const std::string& InString, size_t InMax,
                               const std::string& InContext);
    std::vector<Part> Parse(const std::string& InTemplateStr);
    std::string GetOperator(const std::string& InExpression) const;
    std::vector<std::string> GetNames(const std::string& InExpression) const;
    std::string EncodeValue(const std::string& InValue, const std::string& InOperatorChar) const;
    std::string ExpandPart(const TemplatePart& InPart, const Variables& InVariables) const;
    std::string EscapeRegExp(const std::string& InString) const;
    std::vector<std::pair<std::string, std::string>> PartToRegExp(const TemplatePart& InPart) const;
};

MCP_NAMESPACE_END
