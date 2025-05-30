#pragma once

#include "Core.h"

// Constants
#include "Constants.h"

MCP_NAMESPACE_BEGIN

// Proper RFC 3986 percent-encoding implementation
string EncodeURI(const string& Value);

string EncodeURIComponent(const string& Value);

using Variables = unordered_map<string, std::variant<string, vector<string>>>;

class URI_Template {
  public:
    /**
     * Returns true if the given string contains any URI template expressions.
     * A template expression is a sequence of characters enclosed in curly
     * braces, like {foo} or {?bar}.
     */
    static bool IsTemplate(const string& Str);

    vector<string> GetVariableNames() const;

    explicit URI_Template(const string& TemplateStr);

    string ToString() const;

    string Expand(const Variables& variables) const;

    Variables Match(const string& uri) const;

  private:
    struct TemplatePart {
        string name;
        string operatorChar;
        vector<string> names;
        bool exploded;
    };

    using Part = variant<string, TemplatePart>;

    string template_;
    vector<Part> parts_;

    static void ValidateLength(const string& str, size_t max,
                               const string& context);

    vector<Part> Parse(const string& templateStr);

    string GetOperator(const string& expr) const;

    vector<string> GetNames(const string& expr) const;

    string EncodeValue(const string& value, const string& operatorChar) const;

    string ExpandPart(const TemplatePart& part,
                      const Variables& variables) const;

    string EscapeRegExp(const string& str) const;

    vector<std::pair<string, string>>
    PartToRegExp(const TemplatePart& part) const;
};

MCP_NAMESPACE_END
