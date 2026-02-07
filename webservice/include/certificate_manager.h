#pragma once

#include <windows.h>
#include <wincrypt.h>
#include <ncrypt.h>
#include <string>
#include <vector>
#include <sstream>
#include <regex>
#include <algorithm>
#include <iostream>
#include "crypto_utils.h"
#include "string_utils.h"
#include "json_utils.h"

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "ncrypt.lib")

namespace ArhintSigner {
namespace Certificate {

/**
 * Get certificate name as a string
 */
inline std::string getCertNameString(PCCERT_CONTEXT certContext, DWORD type) {
    DWORD size = CertGetNameStringA(certContext, type, 0, nullptr, nullptr, 0);
    if (size <= 1) return "";
    
    std::vector<char> name(size);
    CertGetNameStringA(certContext, type, 0, nullptr, name.data(), size);
    return std::string(name.data());
}

/**
 * List all valid certificates with private keys from the MY store
 */
inline std::string listCertificates() {
    std::ostringstream arrayBuilder;
    arrayBuilder << "[";
    bool firstCert = true;

    HCERTSTORE hStore = CertOpenSystemStoreA(0, "MY");
    if (!hStore) {
        std::cerr << "Failed to open certificate store" << std::endl;
        return "[]";
    }

    PCCERT_CONTEXT certContext = nullptr;
    SYSTEMTIME currentTime;
    GetSystemTime(&currentTime);
    FILETIME currentFileTime;
    SystemTimeToFileTime(&currentTime, &currentFileTime);

    while ((certContext = CertEnumCertificatesInStore(hStore, certContext)) != nullptr) {
        // Check if certificate has private key
        DWORD keySpec = 0;
        BOOL hasPrivateKey = FALSE;
        BOOL freeProvOrKey = FALSE;
        HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hCryptProvOrNCryptKey = 0;

        hasPrivateKey = CryptAcquireCertificatePrivateKey(
            certContext, 
            CRYPT_ACQUIRE_SILENT_FLAG | CRYPT_ACQUIRE_COMPARE_KEY_FLAG,
            nullptr, 
            &hCryptProvOrNCryptKey, 
            &keySpec, 
            &freeProvOrKey);

        // Release cryptographic provider/key if acquired
        if (freeProvOrKey && hCryptProvOrNCryptKey) {
            if (keySpec == CERT_NCRYPT_KEY_SPEC) {
                NCryptFreeObject(hCryptProvOrNCryptKey);
            } else {
                CryptReleaseContext(hCryptProvOrNCryptKey, 0);
            }
        }

        // Skip certificates without private keys or that are not currently valid
        if (!hasPrivateKey) continue;
        if (CompareFileTime(&certContext->pCertInfo->NotAfter, &currentFileTime) <= 0) continue;
        if (CompareFileTime(&certContext->pCertInfo->NotBefore, &currentFileTime) >= 0) continue;

        try {
            // Get subject and issuer
            std::string subject = getCertNameString(certContext, CERT_NAME_SIMPLE_DISPLAY_TYPE);
            std::string issuer = getCertNameString(certContext, CERT_NAME_SIMPLE_DISPLAY_TYPE);
            
            // Get full DN string for parsing
            DWORD subjectSize = CertNameToStrA(X509_ASN_ENCODING, &certContext->pCertInfo->Subject,
                                               CERT_X500_NAME_STR, nullptr, 0);
            std::vector<char> subjectDN(subjectSize);
            CertNameToStrA(X509_ASN_ENCODING, &certContext->pCertInfo->Subject,
                          CERT_X500_NAME_STR, subjectDN.data(), subjectSize);
            
            std::string subjectStr(subjectDN.data());
            
            // Parse subject for display name
            std::string displayName;
            std::string givenName = Utils::extractDNField(subjectStr, "G");
            std::string surname = Utils::extractDNField(subjectStr, "SN");
            
            if (!givenName.empty() && !surname.empty()) {
                displayName = givenName + " " + surname;
            } else {
                std::string cn = Utils::extractDNField(subjectStr, "CN");
                displayName = !cn.empty() ? cn : subject;
            }
            
            // Add organization if present
            std::string org = Utils::extractDNField(subjectStr, "O");
            if (!org.empty()) {
                displayName += " (" + org + ")";
            }

            // Get thumbprint
            BYTE thumbprint[20];
            DWORD thumbprintSize = sizeof(thumbprint);
            CertGetCertificateContextProperty(certContext, CERT_HASH_PROP_ID, 
                                             thumbprint, &thumbprintSize);
            
            char thumbprintHex[41];
            for (DWORD i = 0; i < thumbprintSize; i++) {
                sprintf_s(thumbprintHex + (i * 2), 3, "%02X", thumbprint[i]);
            }
            std::string thumbprintStr(thumbprintHex);

            // Get dates
            std::string notBefore = Utils::fileTimeToISO(certContext->pCertInfo->NotBefore);
            std::string notAfter = Utils::fileTimeToISO(certContext->pCertInfo->NotAfter);
            std::string expiry = Utils::fileTimeToShortDate(certContext->pCertInfo->NotAfter);

            // Encode certificate
            std::string certB64 = Crypto::base64Encode(certContext->pbCertEncoded, 
                                                       certContext->cbCertEncoded);

            // Build label
            std::string label = "Issued for: " + displayName + " | Issuer: " + issuer + 
                              " (expires " + expiry + ")";

            // Build JSON object for this certificate
            Json::Builder certJson;
            certJson.addString("label", label);
            certJson.addString("thumbprint", thumbprintStr);
            certJson.addString("subject", subjectStr);
            certJson.addString("issuer", issuer);
            certJson.addString("notBefore", notBefore);
            certJson.addString("notAfter", notAfter);
            certJson.addBool("hasPrivateKey", true);
            certJson.addString("cert", certB64);

            if (!firstCert) arrayBuilder << ",";
            arrayBuilder << certJson.toString();
            firstCert = false;
        }
        catch (...) {
            std::cerr << "Error processing certificate" << std::endl;
        }
    }

    CertCloseStore(hStore, 0);
    arrayBuilder << "]";
    return arrayBuilder.str();
}

/**
 * Sign a hash using a certificate identified by thumbprint
 */
inline std::string signHash(const std::string& hashB64Input, const std::string& thumbprintInput) {
    // Trim whitespace from inputs
    std::string hashB64 = Utils::trim(hashB64Input);
    std::string thumbprint = Utils::trim(thumbprintInput);
    
    // Validate inputs
    if (hashB64.empty()) {
        throw std::runtime_error("Hash is required and must be a string");
    }
    if (thumbprint.empty()) {
        throw std::runtime_error("Thumbprint is required and must be a string");
    }

    // Validate base64 format
    std::regex base64Regex("^[A-Za-z0-9+/]*={0,2}$");
    if (!std::regex_match(hashB64, base64Regex)) {
        std::string error = "Hash must be valid base64 encoded string. Received: '" + hashB64 + 
                          "' (length: " + std::to_string(hashB64.length()) + ")";
        std::cerr << error << std::endl;
        throw std::runtime_error(error);
    }

    // Decode hash
    std::vector<BYTE> hashBytes = Crypto::base64Decode(hashB64);
    if (hashBytes.empty()) {
        throw std::runtime_error("Invalid base64 hash - unable to decode");
    }
    
    // Validate hash length (SHA-256 = 32 bytes, SHA-1 = 20 bytes, SHA-512 = 64 bytes)
    if (hashBytes.size() != 32 && hashBytes.size() != 20 && hashBytes.size() != 64) {
        std::string error = "Invalid hash length: " + std::to_string(hashBytes.size()) + 
                          " bytes. Expected 20 (SHA-1), 32 (SHA-256), or 64 (SHA-512) bytes";
        throw std::runtime_error(error);
    }

    // Open certificate store
    HCERTSTORE hStore = CertOpenSystemStoreA(0, "MY");
    if (!hStore) {
        throw std::runtime_error("Failed to open certificate store");
    }

    // Find certificate by thumbprint
    CRYPT_HASH_BLOB hashBlob;
    BYTE thumbprintBytes[20];
    
    // Security: Validate thumbprint length before conversion
    if (thumbprint.length() != 40) {
        CertCloseStore(hStore, 0);
        throw std::runtime_error("Invalid thumbprint length (expected 40 hex characters)");
    }
    
    // Convert hex string to bytes with bounds checking
    for (size_t i = 0; i < 20; i++) {
        int result = sscanf_s(thumbprint.c_str() + (i * 2), "%2hhx", &thumbprintBytes[i]);
        if (result != 1) {
            CertCloseStore(hStore, 0);
            throw std::runtime_error("Invalid thumbprint format (must be hex)");
        }
    }
    
    hashBlob.cbData = 20;
    hashBlob.pbData = thumbprintBytes;

    PCCERT_CONTEXT certContext = CertFindCertificateInStore(
        hStore,
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        0,
        CERT_FIND_HASH,
        &hashBlob,
        nullptr);

    if (!certContext) {
        CertCloseStore(hStore, 0);
        throw std::runtime_error("Certificate not found");
    }

    // Get private key
    DWORD keySpec = 0;
    BOOL freeProvOrKey = FALSE;
    HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hCryptProvOrNCryptKey = 0;

    BOOL hasPrivateKey = CryptAcquireCertificatePrivateKey(
        certContext,
        CRYPT_ACQUIRE_SILENT_FLAG | CRYPT_ACQUIRE_PREFER_NCRYPT_KEY_FLAG,
        nullptr,
        &hCryptProvOrNCryptKey,
        &keySpec,
        &freeProvOrKey);

    if (!hasPrivateKey) {
        CertFreeCertificateContext(certContext);
        CertCloseStore(hStore, 0);
        throw std::runtime_error("Certificate has no private key");
    }
    
    std::cout << "Key spec: " << (keySpec == CERT_NCRYPT_KEY_SPEC ? "CNG" : "Legacy") 
              << " (" << keySpec << ")" << std::endl;

    std::string signatureB64;
    
    try {
        if (keySpec == CERT_NCRYPT_KEY_SPEC) {
            // Use CNG (Cryptography Next Generation) API
            std::cout << "Using CNG API for signing" << std::endl;
            BCRYPT_PKCS1_PADDING_INFO paddingInfo;
            paddingInfo.pszAlgId = BCRYPT_SHA256_ALGORITHM;

            DWORD signatureSize = 0;
            NTSTATUS status = NCryptSignHash(
                hCryptProvOrNCryptKey,
                &paddingInfo,
                hashBytes.data(),
                (DWORD)hashBytes.size(),
                nullptr,
                0,
                &signatureSize,
                BCRYPT_PAD_PKCS1);

            if (status != 0) {
                std::cerr << "NCryptSignHash (get size) failed with status: 0x" 
                         << std::hex << status << std::endl;
                if (status == 0x80090027) { // NTE_BAD_DATA
                    throw std::runtime_error("Invalid hash data - hash may be corrupted or wrong length for algorithm");
                }
                throw std::runtime_error("Failed to get signature size (status: 0x" + 
                                       std::to_string(status) + ")");
            }

            std::vector<BYTE> signature(signatureSize);
            status = NCryptSignHash(
                hCryptProvOrNCryptKey,
                &paddingInfo,
                hashBytes.data(),
                (DWORD)hashBytes.size(),
                signature.data(),
                signatureSize,
                &signatureSize,
                BCRYPT_PAD_PKCS1);

            if (status != 0) {
                std::cerr << "NCryptSignHash failed with status: 0x" 
                         << std::hex << status << std::endl;
                if (status == 0x80090027) { // NTE_BAD_DATA
                    throw std::runtime_error("Invalid hash data - hash may be corrupted or wrong length for algorithm");
                }
                throw std::runtime_error("Failed to sign hash (status: 0x" + 
                                       std::to_string(status) + ")");
            }

            signatureB64 = Crypto::base64Encode(signature.data(), signatureSize);
        }
        else {
            // Use legacy CryptoAPI
            std::cout << "Using legacy CryptoAPI for signing" << std::endl;
            HCRYPTHASH hHash;
            if (!CryptCreateHash(hCryptProvOrNCryptKey, CALG_SHA_256, 0, 0, &hHash)) {
                DWORD error = GetLastError();
                std::cerr << "CryptCreateHash failed with error: " << error << std::endl;
                throw std::runtime_error("Failed to create hash object. Error: " + 
                                       std::to_string(error));
            }

            if (!CryptSetHashParam(hHash, HP_HASHVAL, hashBytes.data(), 0)) {
                DWORD error = GetLastError();
                CryptDestroyHash(hHash);
                std::cerr << "CryptSetHashParam failed with error: " << error << std::endl;
                throw std::runtime_error("Failed to set hash value");
            }

            DWORD signatureSize = 0;
            if (!CryptSignHashA(hHash, keySpec, nullptr, 0, nullptr, &signatureSize)) {
                CryptDestroyHash(hHash);
                throw std::runtime_error("Failed to get signature size");
            }

            std::vector<BYTE> signature(signatureSize);
            if (!CryptSignHashA(hHash, keySpec, nullptr, 0, signature.data(), &signatureSize)) {
                CryptDestroyHash(hHash);
                throw std::runtime_error("Failed to sign hash");
            }

            CryptDestroyHash(hHash);

            // Reverse byte order (CryptoAPI returns little-endian)
            std::reverse(signature.begin(), signature.end());
            signatureB64 = Crypto::base64Encode(signature.data(), signatureSize);
        }
    }
    catch (...) {
        if (freeProvOrKey && hCryptProvOrNCryptKey) {
            if (keySpec == CERT_NCRYPT_KEY_SPEC) {
                NCryptFreeObject(hCryptProvOrNCryptKey);
            } else {
                CryptReleaseContext(hCryptProvOrNCryptKey, 0);
            }
        }
        CertFreeCertificateContext(certContext);
        CertCloseStore(hStore, 0);
        throw;
    }

    if (freeProvOrKey && hCryptProvOrNCryptKey) {
        if (keySpec == CERT_NCRYPT_KEY_SPEC) {
            NCryptFreeObject(hCryptProvOrNCryptKey);
        } else {
            CryptReleaseContext(hCryptProvOrNCryptKey, 0);
        }
    }

    CertFreeCertificateContext(certContext);
    CertCloseStore(hStore, 0);

    return signatureB64;
}

} // namespace Certificate
} // namespace ArhintSigner
