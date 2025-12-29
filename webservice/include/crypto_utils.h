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
    if (!CryptBinaryToStringA(data, length, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, 
                              base64.data(), &base64Length)) {
        return "";
    }

    return std::string(base64.data(), base64Length - 1);
}

/**
 * Base64 decoding using Windows CryptoAPI
 */
inline std::vector<BYTE> base64Decode(const std::string& base64) {
    DWORD dataLength = 0;
    if (!CryptStringToBinaryA(base64.c_str(), 0, CRYPT_STRING_BASE64, 
                              nullptr, &dataLength, nullptr, nullptr)) {
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
