#pragma once

#include <windows.h>
#include <string>
#include <regex>

namespace ArhintSigner {
namespace Utils {

/**
 * Extract a field value from a DN (Distinguished Name) string
 * Example: "CN=John Doe, O=Company" -> extractDNField(dn, "CN") returns "John Doe"
 */
inline std::string extractDNField(const std::string& dn, const std::string& field) {
    std::regex pattern("\\b" + field + "=([^,+]+)");
    std::smatch match;
    if (std::regex_search(dn, match, pattern)) {
        std::string value = match[1].str();
        // Trim whitespace
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        return value;
    }
    return "";
}

/**
 * Convert Windows FILETIME to ISO 8601 string
 */
inline std::string fileTimeToISO(const FILETIME& ft) {
    SYSTEMTIME st;
    FileTimeToSystemTime(&ft, &st);
    
    char buffer[100];
    sprintf_s(buffer, "%04d-%02d-%02dT%02d:%02d:%02d.000Z",
              st.wYear, st.wMonth, st.wDay,
              st.wHour, st.wMinute, st.wSecond);
    return std::string(buffer);
}

/**
 * Convert Windows FILETIME to short date string (MM/DD/YYYY)
 */
inline std::string fileTimeToShortDate(const FILETIME& ft) {
    SYSTEMTIME st;
    FileTimeToSystemTime(&ft, &st);
    
    char buffer[50];
    sprintf_s(buffer, "%02d/%02d/%04d", st.wMonth, st.wDay, st.wYear);
    return std::string(buffer);
}

/**
 * Trim whitespace from both ends of a string
 */
inline std::string trim(const std::string& str) {
    std::string result = str;
    result.erase(0, result.find_first_not_of(" \t\r\n"));
    result.erase(result.find_last_not_of(" \t\r\n") + 1);
    return result;
}

} // namespace Utils
} // namespace ArhintSigner
