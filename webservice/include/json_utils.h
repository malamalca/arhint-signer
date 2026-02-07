#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <map>
#include <regex>

namespace ArhintSigner {
namespace Json {

/**
 * Simple JSON builder for creating JSON responses
 */
class Builder {
private:
    std::ostringstream ss;
    bool first;

    std::string escapeJson(const std::string& s) {
        std::ostringstream o;
        for (char c : s) {
            switch (c) {
                case '"': o << "\\\""; break;
                case '\\': o << "\\\\"; break;
                case '\b': o << "\\b"; break;
                case '\f': o << "\\f"; break;
                case '\n': o << "\\n"; break;
                case '\r': o << "\\r"; break;
                case '\t': o << "\\t"; break;
                default:
                    if ('\x00' <= c && c <= '\x1f') {
                        o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                    } else {
                        o << c;
                    }
            }
        }
        return o.str();
    }

public:
    Builder() : first(true) {
        ss << "{";
    }

    void addString(const std::string& key, const std::string& value) {
        if (!first) ss << ",";
        ss << "\"" << escapeJson(key) << "\":\"" << escapeJson(value) << "\"";
        first = false;
    }

    void addBool(const std::string& key, bool value) {
        if (!first) ss << ",";
        ss << "\"" << escapeJson(key) << "\":" << (value ? "true" : "false");
        first = false;
    }

    void addArray(const std::string& key, const std::string& arrayContent) {
        if (!first) ss << ",";
        ss << "\"" << escapeJson(key) << "\":" << arrayContent;
        first = false;
    }

    void addObject(const std::string& key, const std::string& objectContent) {
        if (!first) ss << ",";
        ss << "\"" << escapeJson(key) << "\":" << objectContent;
        first = false;
    }

    std::string toString() {
        return ss.str() + "}";
    }
};

/**
 * Simple JSON parser (basic implementation for our needs)
 */
inline std::map<std::string, std::string> parse(const std::string& json) {
    std::map<std::string, std::string> result;
    
    // Security: Prevent regex DoS with input size limit
    if (json.length() > 10240) {
        throw std::runtime_error("JSON input too large (max 10KB)");
    }
    
    std::regex keyValueRegex("\"([^\"]+)\"\\s*:\\s*\"([^\"]+)\"");
    auto begin = std::sregex_iterator(json.begin(), json.end(), keyValueRegex);
    auto end = std::sregex_iterator();
    
    for (auto i = begin; i != end; ++i) {
        std::smatch match = *i;
        result[match[1].str()] = match[2].str();
    }
    
    return result;
}

} // namespace Json
} // namespace ArhintSigner
