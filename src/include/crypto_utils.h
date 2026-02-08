#pragma once

#include <windows.h>
#include <wincrypt.h>
#include <string>
#include <vector>

namespace ArhintSigner {
namespace Crypto {

/**
 * Base64 encoding using Windows CryptoAPI
 */
inline std::string base64Encode(const BYTE* data, DWORD length) {
    DWORD base64Length = 0;
    if (!CryptBinaryToStringA(data, length, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, 
                              nullptr, &base64Length)) {
        return "";
    }

    std::vector<char> base64(base64Length);
    DWORD actualLength = base64Length;
    if (!CryptBinaryToStringA(data, length, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, 
                              base64.data(), &actualLength)) {
        return "";
    }

    // actualLength now contains the actual string length including null terminator
    // Return string excluding the null terminator
    if (actualLength > 0 && base64[actualLength - 1] == '\0') {
        return std::string(base64.data(), actualLength - 1);
    }
    return std::string(base64.data(), actualLength);
}

/**
 * Base64 decoding using Windows CryptoAPI
 */
inline std::vector<BYTE> base64Decode(const std::string& base64) {
    // Security: Limit input size to prevent DoS (max 1MB base64 = ~750KB decoded)
    if (base64.length() > 1048576) {
        return {};
    }
    
    DWORD dataLength = 0;
    if (!CryptStringToBinaryA(base64.c_str(), 0, CRYPT_STRING_BASE64, 
                              nullptr, &dataLength, nullptr, nullptr)) {
        return {};
    }
    
    // Security: Sanity check decoded size
    if (dataLength > 1048576) {
        return {};
    }

    std::vector<BYTE> data(dataLength);
    if (!CryptStringToBinaryA(base64.c_str(), 0, CRYPT_STRING_BASE64, 
                              data.data(), &dataLength, nullptr, nullptr)) {
        return {};
    }

    return data;
}

} // namespace Crypto
} // namespace ArhintSigner
