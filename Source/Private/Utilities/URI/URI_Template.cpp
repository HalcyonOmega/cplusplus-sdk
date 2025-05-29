#include "../../Utilities/URI/URI_Template.h"

namespace MCP {

// Proper RFC 3986 percent-encoding implementation
string EncodeURI(const string& value) {
    // RFC 3986: Encode everything except unreserved and reserved characters
    // Unreserved: A-Z a-z 0-9 - . _ ~
    // Reserved: : / ? # [ ] @ ! $ & ' ( ) * + , ; =
    string result;
    result.reserve(value.length() * 3); // Worst case: every char becomes %XX

    for (unsigned char c : value) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '.' || c == '_' || c == '~' ||  // unreserved
            c == ':' || c == '/' || c == '?' || c == '#' || c == '[' || c == ']' ||
            c == '@' || c == '!' || c == '$' || c == '&' || c == '\'' || c == '(' ||
            c == ')' || c == '*' || c == '+' || c == ',' || c == ';' || c == '=') { // reserved
            result += c;
        } else {
            result += '%';
            result += "0123456789ABCDEF"[c >> 4];
            result += "0123456789ABCDEF"[c & 0x0F];
        }
    }
    return result;
}

string EncodeURIComponent(const string& value) {
    // RFC 3986: Encode everything except unreserved characters
    // Unreserved: A-Z a-z 0-9 - . _ ~
    string result;
    result.reserve(value.length() * 3); // Worst case: every char becomes %XX

    for (unsigned char c : value) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '.' || c == '_' || c == '~') { // unreserved only
            result += c;
        } else {
            result += '%';
            result += "0123456789ABCDEF"[c >> 4];
            result += "0123456789ABCDEF"[c & 0x0F];
        }
    }
    return result;
}

// URI_Template method implementations

bool URI_Template::IsTemplate(const string& str) {
    // Look for any sequence of characters between curly braces
    // that isn't just whitespace
    regex pattern(R"(\{[^}\s]+\})");
    return regex_search(str, pattern);
}

vector<string> URI_Template::GetVariableNames() const {
    vector<string> names;
    for (const auto& part : parts_) {
        if (holds_alternative<TemplatePart>(part)) {
            const auto& templatePart = get<TemplatePart>(part);
            names.insert(names.end(), templatePart.names.begin(), templatePart.names.end());
        }
    }
    return names;
}

URI_Template::URI_Template(const string& templateStr) : template_(templateStr) {
    ValidateLength(templateStr, MAX_TEMPLATE_LENGTH, "Template");
    parts_ = Parse(templateStr);
}

string URI_Template::ToString() const {
    return template_;
}

string URI_Template::Expand(const Variables& variables) const {
    string result;
    bool hasQueryParam = false;

    for (const auto& part : parts_) {
        if (holds_alternative<string>(part)) {
            result += get<string>(part);
            continue;
        }

        const auto& templatePart = get<TemplatePart>(part);
        string expanded = ExpandPart(templatePart, variables);
        if (expanded.empty()) continue;

        // Convert ? to & if we already have a query parameter
        if ((templatePart.operatorChar == "?" || templatePart.operatorChar == "&") && hasQueryParam) {
            if (expanded[0] == '?') {
                expanded[0] = '&';
            }
            result += expanded;
        } else {
            result += expanded;
        }

        if (templatePart.operatorChar == "?" || templatePart.operatorChar == "&") {
            hasQueryParam = true;
        }
    }

    return result;
}

Variables URI_Template::Match(const string& uri) const {
    ValidateLength(uri, MAX_TEMPLATE_LENGTH, "URI");
    string pattern = "^";
    vector<pair<string, bool>> names; // name, exploded

    for (const auto& part : parts_) {
        if (holds_alternative<string>(part)) {
            pattern += EscapeRegExp(get<string>(part));
        } else {
            const auto& templatePart = get<TemplatePart>(part);
            auto patterns = PartToRegExp(templatePart);
            for (const auto& [partPattern, name] : patterns) {
                pattern += partPattern;
                names.emplace_back(name, templatePart.exploded);
            }
        }
    }

    pattern += "$";
    ValidateLength(pattern, MAX_REGEX_LENGTH, "Generated regex pattern");
    regex regexPattern(pattern);
    smatch match;

    if (!regex_match(uri, match, regexPattern)) {
        return {};
    }

    Variables result;
    for (size_t i = 0; i < names.size(); i++) {
        const auto& [name, exploded] = names[i];
        string value = match[i + 1].str();
        string cleanName = name;
        // Remove * from name if present
        if (!cleanName.empty() && cleanName.back() == '*') {
            cleanName.pop_back();
        }

        if (exploded && value.find(',') != string::npos) {
            vector<string> values;
            stringstream ss(value);
            string item;
            while (getline(ss, item, ',')) {
                values.push_back(item);
            }
            result[cleanName] = values;
        } else {
            result[cleanName] = value;
        }
    }

    return result;
}

void URI_Template::ValidateLength(const string& str, size_t max, const string& context) {
    if (str.length() > max) {
        throw runtime_error(context + " exceeds maximum length of " + to_string(max) +
                          " characters (got " + to_string(str.length()) + ")");
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
            if (end == string::npos) {
                throw runtime_error("Unclosed template expression");
            }

            expressionCount++;
            if (expressionCount > MAX_TEMPLATE_EXPRESSIONS) {
                throw runtime_error("Template contains too many expressions (max " +
                                  to_string(MAX_TEMPLATE_EXPRESSIONS) + ")");
            }

            string expr = templateStr.substr(i + 1, end - i - 1);
            string operatorChar = GetOperator(expr);
            bool exploded = expr.find('*') != string::npos;
            vector<string> names = GetNames(expr);
            string name = names.empty() ? "" : names[0];

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

    if (!currentText.empty()) {
        parts.emplace_back(currentText);
    }

    return parts;
}

string URI_Template::GetOperator(const string& expr) const {
    vector<string> operators = {"+", "#", ".", "/", "?", "&"};
    for (const auto& op : operators) {
        if (expr.substr(0, op.length()) == op) {
            return op;
        }
    }
    return "";
}

vector<string> URI_Template::GetNames(const string& expr) const {
    string operatorChar = GetOperator(expr);
    string namesPart = expr.substr(operatorChar.length());

    vector<string> names;
    stringstream ss(namesPart);
    string name;

    while (getline(ss, name, ',')) {
        // Remove * and trim whitespace
        if (!name.empty() && name.back() == '*') {
            name.pop_back();
        }
        // Simple trim
        size_t start = name.find_first_not_of(" \t");
        size_t end = name.find_last_not_of(" \t");
        if (start != string::npos && end != string::npos) {
            name = name.substr(start, end - start + 1);
        }
        if (!name.empty()) {
            names.push_back(name);
        }
    }

    return names;
}

string URI_Template::EncodeValue(const string& value, const string& operatorChar) const {
    ValidateLength(value, MAX_VARIABLE_LENGTH, "Variable value");
    if (operatorChar == "+" || operatorChar == "#") {
        return EncodeURI(value);
    }
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
                encoded = "";
                for (size_t i = 0; i < encodedValues.size(); i++) {
                    if (i > 0) encoded += ",";
                    encoded += encodedValues[i];
                }
            } else {
                encoded = EncodeValue(get<string>(it->second), part.operatorChar);
            }
            pairs.push_back(name + "=" + encoded);
        }

        if (pairs.empty()) return "";
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
        if (values.empty()) return "";
        string result;
        for (size_t i = 0; i < values.size(); i++) {
            if (i > 0) result += ",";
            result += values[i];
        }
        return result;
    }

    auto it = variables.find(part.name);
    if (it == variables.end()) return "";

    vector<string> values;
    if (holds_alternative<vector<string>>(it->second)) {
        values = get<vector<string>>(it->second);
    } else {
        values.push_back(get<string>(it->second));
    }

    vector<string> encoded;
    for (const auto& v : values) {
        encoded.push_back(EncodeValue(v, part.operatorChar));
    }

    string result;
    for (size_t i = 0; i < encoded.size(); i++) {
        if (i > 0) result += ",";
        result += encoded[i];
    }

    if (part.operatorChar == "") {
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
        if (c == '.' || c == '*' || c == '+' || c == '?' || c == '^' ||
            c == '$' || c == '{' || c == '}' || c == '(' || c == ')' ||
            c == '|' || c == '[' || c == ']' || c == '\\') {
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

    if (part.operatorChar == "") {
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

} // namespace MCP 