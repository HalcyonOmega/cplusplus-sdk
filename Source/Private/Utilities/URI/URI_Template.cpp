#include "Utilities/URI/URI_Template.h"

#include "Constants.h"

MCP_NAMESPACE_BEGIN

// URI Template Constants
static constexpr const size_t MAX_TEMPLATE_LENGTH = 1000000; // 1MB
static constexpr const size_t MAX_VARIABLE_LENGTH = 1000000; // 1MB
static constexpr const size_t MAX_TEMPLATE_EXPRESSIONS = 10000;
static constexpr const size_t MAX_REGEX_LENGTH = 1000000; // 1MB

// Proper RFC 3986 percent-encoding implementation
string EncodeURI(const string& Value) {
    // RFC 3986: Encode everything except unreserved and reserved characters
    // Unreserved: A-Z a-z 0-9 - . _ ~
    // Reserved: : / ? # [ ] @ ! $ & ' ( ) * + , ; =
    string Result;
    Result.reserve(Value.length() * 3); // Worst case: every char becomes %XX

    for (unsigned char Char : Value) {
        if ((Char >= 'A' && Char <= 'Z') || (Char >= 'a' && Char <= 'z')
            || (Char >= '0' && Char <= '9') || Char == '-' || Char == '.' || Char == '_'
            || Char == '~' || // unreserved
            Char == ':' || Char == '/' || Char == '?' || Char == '#' || Char == '[' || Char == ']'
            || Char == '@' || Char == '!' || Char == '$' || Char == '&' || Char == '\''
            || Char == '(' || Char == ')' || Char == '*' || Char == '+' || Char == ','
            || Char == ';') { // reserved
            Result += Char;
        } else {
            Result += '%';
            Result += "0123456789ABCDEF"[Char >> 4];
            Result += "0123456789ABCDEF"[Char & 0x0F];
        }
    }
    return Result;
}

string EncodeURIComponent(const string& Value) {
    // RFC 3986: Encode everything except unreserved characters
    // Unreserved: A-Z a-z 0-9 - . _ ~
    string Result;
    Result.reserve(Value.length() * 3); // Worst case: every char becomes %XX

    for (unsigned char Char : Value) {
        if ((Char >= 'A' && Char <= 'Z') || (Char >= 'a' && Char <= 'z')
            || (Char >= '0' && Char <= '9') || Char == '-' || Char == '.' || Char == '_'
            || Char == '~') { // unreserved only
            Result += Char;
        } else {
            Result += '%';
            Result += "0123456789ABCDEF"[Char >> 4];
            Result += "0123456789ABCDEF"[Char & 0x0F];
        }
    }
    return Result;
}

// URI_Template method implementations

bool URI_Template::IsTemplate(const string& Str) {
    // Look for any sequence of characters between curly braces
    // that isn't just whitespace
    regex Pattern(R"(\{[^}\s]+\})");
    return regex_search(Str, Pattern);
}

vector<string> URI_Template::GetVariableNames() const {
    vector<string> Names;
    for (const auto& Part : m_Parts) {
        if (holds_alternative<TemplatePart>(Part)) {
            const auto& TemplatePart = get<TemplatePart>(Part);
            Names.insert(Names.end(), TemplatePart.Names.begin(), TemplatePart.Names.end());
        }
    }
    return Names;
}

URI_Template::URI_Template(const string& TemplateStr) : m_Template(TemplateStr) {
    ValidateLength(TemplateStr, MAX_TEMPLATE_LENGTH, "Template");
    m_Parts = Parse(TemplateStr);
}

string URI_Template::ToString() const {
    return m_Template;
}

string URI_Template::Expand(const Variables& Variables) const {
    string Result;
    bool HasQueryParam = false;

    for (const auto& Part : m_Parts) {
        if (holds_alternative<string>(Part)) {
            Result += get<string>(Part);
            continue;
        }

        const auto& TemplatePart = get<TemplatePart>(Part);
        string Expanded = ExpandPart(TemplatePart, Variables);
        if (Expanded.empty()) continue;

        // Convert ? to & if we already have a query parameter
        if ((TemplatePart.OperatorChar == "?" || TemplatePart.OperatorChar == "&")
            && HasQueryParam) {
            if (Expanded[0] == '?') { Expanded[0] = '&'; }
            Result += Expanded;
        } else {
            Result += Expanded;
        }

        if (TemplatePart.OperatorChar == "?" || TemplatePart.OperatorChar == "&") {
            HasQueryParam = true;
        }
    }

    return Result;
}

Variables URI_Template::Match(const string& Uri) const {
    ValidateLength(Uri, MAX_TEMPLATE_LENGTH, "URI");
    string Pattern = "^";
    vector<pair<string, bool>> Names; // name, exploded

    for (const auto& Part : m_Parts) {
        if (holds_alternative<string>(Part)) {
            Pattern += EscapeRegExp(get<string>(Part));
        } else {
            const auto& TemplatePart = get<TemplatePart>(Part);
            auto Patterns = PartToRegExp(TemplatePart);
            for (const auto& [PartPattern, Name] : Patterns) {
                Pattern += PartPattern;
                Names.emplace_back(Name, TemplatePart.Exploded);
            }
        }
    }

    Pattern += "$";
    ValidateLength(Pattern, MAX_REGEX_LENGTH, "Generated regex pattern");
    regex RegexPattern(Pattern);
    smatch Match;

    if (!regex_match(Uri, Match, RegexPattern)) { return {}; }

    Variables result;
    for (size_t i = 0; i < Names.size(); i++) {
        const auto& [name, exploded] = Names[i];
        string value = Match[i + 1].str();
        string cleanName = name;
        // Remove * from name if present
        if (!cleanName.empty() && cleanName.back() == '*') { cleanName.pop_back(); }

        if (exploded && value.find(',') != string::npos) {
            vector<string> values;
            stringstream ss(value);
            string item;
            while (getline(ss, item, ',')) { values.push_back(item); }
            result[cleanName] = values;
        } else {
            result[cleanName] = value;
        }
    }

    return result;
}

void URI_Template::ValidateLength(const string& str, size_t max, const string& context) {
    if (str.length() > max) {
        throw runtime_error(context + " exceeds maximum length of " + to_string(max)
                            + " characters (got " + to_string(str.length()) + ")");
    }
}

vector<URI_Template::Part> URI_Template::Parse(const string& templateStr) {
    vector<Part> parts;
    string currentText;
    size_t i = 0;
    size_t expressionCount = 0;

    while (i < templateStr.length()) {
        if (templateStr[i] == '{') {
            if (!currentText.empty()) {
                parts.emplace_back(currentText);
                currentText.clear();
            }
            size_t end = templateStr.find('}', i);
            if (end == string::npos) { throw runtime_error("Unclosed template expression"); }

            expressionCount++;
            if (expressionCount > MAX_TEMPLATE_EXPRESSIONS) {
                throw runtime_error("Template contains too many expressions (max "
                                    + to_string(MAX_TEMPLATE_EXPRESSIONS) + ")");
            }

            string expr = templateStr.substr(i + 1, end - i - 1);
            string operatorChar = GetOperator(expr);
            bool exploded = expr.find('*') != string::npos;
            vector<string> names = GetNames(expr);
            string name = names.empty() ? MSG_NULL : names[0];

            // Validate variable name length
            for (const auto& variableName : names) {
                ValidateLength(variableName, MAX_VARIABLE_LENGTH, "Variable name");
            }

            parts.emplace_back(TemplatePart{name, operatorChar, names, exploded});
            i = end + 1;
        } else {
            currentText += templateStr[i];
            i++;
        }
    }

    if (!currentText.empty()) { parts.emplace_back(currentText); }

    return parts;
}

string URI_Template::GetOperator(const string& expr) const {
    vector<string> operators = {"+", "#", ".", "/", "?", "&"};
    for (const auto& op : operators) {
        if (expr.substr(0, op.length()) == op) { return op; }
    }
    return MSG_NULL;
}

vector<string> URI_Template::GetNames(const string& expr) const {
    string operatorChar = GetOperator(expr);
    string namesPart = expr.substr(operatorChar.length());

    vector<string> names;
    stringstream ss(namesPart);
    string name;

    while (getline(ss, name, ',')) {
        // Remove * and trim whitespace
        if (!name.empty() && name.back() == '*') { name.pop_back(); }
        // Simple trim
        size_t start = name.find_first_not_of(" \t");
        size_t end = name.find_last_not_of(" \t");
        if (start != string::npos && end != string::npos) {
            name = name.substr(start, end - start + 1);
        }
        if (!name.empty()) { names.push_back(name); }
    }

    return names;
}

string URI_Template::EncodeValue(const string& value, const string& operatorChar) const {
    ValidateLength(value, MAX_VARIABLE_LENGTH, "Variable value");
    if (operatorChar == "+" || operatorChar == "#") { return EncodeURI(value); }
    return EncodeURIComponent(value);
}

string URI_Template::ExpandPart(const TemplatePart& part, const Variables& variables) const {
    if (part.operatorChar == "?" || part.operatorChar == "&") {
        vector<string> pairs;
        for (const auto& name : part.names) {
            auto it = variables.find(name);
            if (it == variables.end()) continue;

            string encoded;
            if (holds_alternative<vector<string>>(it->second)) {
                const auto& values = get<vector<string>>(it->second);
                vector<string> encodedValues;
                for (const auto& v : values) {
                    encodedValues.push_back(EncodeValue(v, part.operatorChar));
                }
                encoded = MSG_NULL;
                for (size_t i = 0; i < encodedValues.size(); i++) {
                    if (i > 0) encoded += ",";
                    encoded += encodedValues[i];
                }
            } else {
                encoded = EncodeValue(get<string>(it->second), part.operatorChar);
            }
            pairs.push_back(name + "=" + encoded);
        }

        if (pairs.empty()) return MSG_NULL;
        string separator = part.operatorChar == "?" ? "?" : "&";
        string result = separator;
        for (size_t i = 0; i < pairs.size(); i++) {
            if (i > 0) result += "&";
            result += pairs[i];
        }
        return result;
    }

    if (part.names.size() > 1) {
        vector<string> values;
        for (const auto& name : part.names) {
            auto it = variables.find(name);
            if (it != variables.end()) {
                if (holds_alternative<vector<string>>(it->second)) {
                    const auto& vec = get<vector<string>>(it->second);
                    if (!vec.empty()) values.push_back(vec[0]);
                } else {
                    values.push_back(get<string>(it->second));
                }
            }
        }
        if (values.empty()) return MSG_NULL;
        string result;
        for (size_t i = 0; i < values.size(); i++) {
            if (i > 0) result += ",";
            result += values[i];
        }
        return result;
    }

    auto it = variables.find(part.name);
    if (it == variables.end()) return MSG_NULL;

    vector<string> values;
    if (holds_alternative<vector<string>>(it->second)) {
        values = get<vector<string>>(it->second);
    } else {
        values.push_back(get<string>(it->second));
    }

    vector<string> encoded;
    for (const auto& v : values) { encoded.push_back(EncodeValue(v, part.operatorChar)); }

    string result;
    for (size_t i = 0; i < encoded.size(); i++) {
        if (i > 0) result += ",";
        result += encoded[i];
    }

    if (part.operatorChar == MSG_NULL) {
        return result;
    } else if (part.operatorChar == "+") {
        return result;
    } else if (part.operatorChar == "#") {
        return "#" + result;
    } else if (part.operatorChar == ".") {
        string dotResult = ".";
        for (size_t i = 0; i < encoded.size(); i++) {
            if (i > 0) dotResult += ".";
            dotResult += encoded[i];
        }
        return dotResult;
    } else if (part.operatorChar == "/") {
        string slashResult = "/";
        for (size_t i = 0; i < encoded.size(); i++) {
            if (i > 0) slashResult += "/";
            slashResult += encoded[i];
        }
        return slashResult;
    } else {
        return result;
    }
}

string URI_Template::EscapeRegExp(const string& str) const {
    string result;
    for (char c : str) {
        if (c == '.' || c == '*' || c == '+' || c == '?' || c == '^' || c == '$' || c == '{'
            || c == '}' || c == '(' || c == ')' || c == '|' || c == '[' || c == ']' || c == '\\') {
            result += '\\';
        }
        result += c;
    }
    return result;
}

vector<pair<string, string>> URI_Template::PartToRegExp(const TemplatePart& part) const {
    vector<pair<string, string>> patterns;

    // Validate variable name length for matching
    for (const auto& name : part.names) {
        ValidateLength(name, MAX_VARIABLE_LENGTH, "Variable name");
    }

    if (part.operatorChar == "?" || part.operatorChar == "&") {
        for (size_t i = 0; i < part.names.size(); i++) {
            const string& name = part.names[i];
            string prefix = (i == 0) ? "\\" + part.operatorChar : "&";
            patterns.emplace_back(prefix + EscapeRegExp(name) + "=([^&]+)", name);
        }
        return patterns;
    }

    string pattern;
    const string& name = part.name;

    if (part.operatorChar == MSG_NULL) {
        pattern = part.exploded ? "([^/]+(?:,[^/]+)*)" : "([^/,]+)";
    } else if (part.operatorChar == "+" || part.operatorChar == "#") {
        pattern = "(.+)";
    } else if (part.operatorChar == ".") {
        pattern = "\\.([^/,]+)";
    } else if (part.operatorChar == "/") {
        pattern = "/" + (part.exploded ? "([^/]+(?:,[^/]+)*)" : "([^/,]+)");
    } else {
        pattern = "([^/]+)";
    }

    patterns.emplace_back(pattern, name);
    return patterns;
}

MCP_NAMESPACE_END