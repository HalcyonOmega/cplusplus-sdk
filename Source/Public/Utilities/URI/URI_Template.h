#pragma once

#include "Core.h"

MCP_NAMESPACE_BEGIN

// Proper RFC 3986 percent-encoding implementation
string EncodeURI(const string& InValue);

string EncodeURIComponent(const string& InValue);

using Variables = unordered_map<string, std::variant<string, vector<string>>>;

class URI_Template {
  public:
    /**
     * Returns true if the given string contains any URI template expressions.
     * A template expression is a sequence of characters enclosed in curly
     * braces, like {foo} or {?bar}.
     */
    static bool IsTemplate(const string& InString);

    vector<string> GetVariableNames() const;

    explicit URI_Template(const string& InTemplateStr);

    string ToString() const;

    string Expand(const Variables& InVariables) const;

    Variables Match(const string& InURI) const;

  private:
    struct TemplatePart {
        string m_Name;
        string m_OperatorChar;
        vector<string> m_Names;
        bool m_Exploded;
    };

    using Part = variant<string, TemplatePart>;

    string m_Template;
    vector<Part> m_Parts;

    static void ValidateLength(const string& InString, size_t InMax, const string& InContext);

    vector<Part> Parse(const string& InTemplateStr);

    string GetOperator(const string& InExpression) const;

    vector<string> GetNames(const string& InExpression) const;

    string EncodeValue(const string& InValue, const string& InOperatorChar) const;

    string ExpandPart(const TemplatePart& InPart, const Variables& InVariables) const;

    string EscapeRegExp(const string& InString) const;

    vector<std::pair<string, string>> PartToRegExp(const TemplatePart& InPart) const;
};

MCP_NAMESPACE_END
