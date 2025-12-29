#include <windows.h>
#include <wincrypt.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <regex>
#include <ctime>
#include <iomanip>
#include <map>
#include <algorithm>
#include <io.h>
#include <fcntl.h>

// Simple JSON builder for responses
class JsonBuilder {
private:
    std::ostringstream ss;
    bool first;

public:
    JsonBuilder() : first(true) {
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
        std::string result = ss.str() + "}";
        return result;
    }

private:
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
};

// Base64 encoding
std::string base64Encode(const BYTE* data, DWORD length) {
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

// Base64 decoding
std::vector<BYTE> base64Decode(const std::string& base64) {
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

// Extract value from DN string (e.g., "CN=John Doe" -> "John Doe")
std::string extractDNField(const std::string& dn, const std::string& field) {
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

// Convert FILETIME to ISO 8601 string
std::string fileTimeToISO(const FILETIME& ft) {
    SYSTEMTIME st;
    FileTimeToSystemTime(&ft, &st);
    
    char buffer[100];
    sprintf_s(buffer, "%04d-%02d-%02dT%02d:%02d:%02d.000Z",
              st.wYear, st.wMonth, st.wDay,
              st.wHour, st.wMinute, st.wSecond);
    return std::string(buffer);
}

// Convert FILETIME to short date string
std::string fileTimeToShortDate(const FILETIME& ft) {
    SYSTEMTIME st;
    FileTimeToSystemTime(&ft, &st);
    
    char buffer[50];
    sprintf_s(buffer, "%02d/%02d/%04d", st.wMonth, st.wDay, st.wYear);
    return std::string(buffer);
}

// Get certificate subject/issuer name as string
std::string getCertNameString(PCCERT_CONTEXT certContext, DWORD type) {
    DWORD size = CertGetNameStringA(certContext, type, 0, nullptr, nullptr, 0);
    if (size <= 1) return "";
    
    std::vector<char> name(size);
    CertGetNameStringA(certContext, type, 0, nullptr, name.data(), size);
    return std::string(name.data());
}

// List certificates
std::string listCertificates() {
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

        if (freeProvOrKey && hCryptProvOrNCryptKey) {
            if (keySpec == CERT_NCRYPT_KEY_SPEC) {
                NCryptFreeObject(hCryptProvOrNCryptKey);
            } else {
                CryptReleaseContext(hCryptProvOrNCryptKey, 0);
            }
        }

        // Check if certificate is currently valid
        if (!hasPrivateKey) continue;
        if (CompareFileTime(&certContext->pCertInfo->NotAfter, &currentFileTime) <= 0) continue;
        if (CompareFileTime(&certContext->pCertInfo->NotBefore, &currentFileTime) >= 0) continue;

        try {
            // Get subject and issuer
            std::string subject = getCertNameString(certContext, CERT_NAME_SIMPLE_DISPLAY_TYPE);
            std::string issuer = getCertNameString(certContext, CERT_NAME_SIMPLE_DISPLAY_TYPE);
            
            // Get full DN strings for parsing
            DWORD subjectSize = CertNameToStrA(X509_ASN_ENCODING, &certContext->pCertInfo->Subject,
                                               CERT_X500_NAME_STR, nullptr, 0);
            std::vector<char> subjectDN(subjectSize);
            CertNameToStrA(X509_ASN_ENCODING, &certContext->pCertInfo->Subject,
                          CERT_X500_NAME_STR, subjectDN.data(), subjectSize);
            
            std::string subjectStr(subjectDN.data());
            
            // Parse subject for display name
            std::string displayName;
            std::string givenName = extractDNField(subjectStr, "G");
            std::string surname = extractDNField(subjectStr, "SN");
            
            if (!givenName.empty() && !surname.empty()) {
                displayName = givenName + " " + surname;
            } else {
                std::string cn = extractDNField(subjectStr, "CN");
                if (!cn.empty()) {
                    displayName = cn;
                } else {
                    displayName = subject;
                }
            }
            
            // Add organization if present
            std::string org = extractDNField(subjectStr, "O");
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
            std::string notBefore = fileTimeToISO(certContext->pCertInfo->NotBefore);
            std::string notAfter = fileTimeToISO(certContext->pCertInfo->NotAfter);
            std::string expiry = fileTimeToShortDate(certContext->pCertInfo->NotAfter);

            // Encode certificate
            std::string certB64 = base64Encode(certContext->pbCertEncoded, 
                                              certContext->cbCertEncoded);

            // Build label
            std::string label = "Issued for: " + displayName + " | Issuer: " + issuer + 
                              " (expires " + expiry + ")";

            // Build JSON object for this certificate
            JsonBuilder certJson;
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

// Sign hash with certificate
std::string signHash(const std::string& hashB64, const std::string& thumbprint) {
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
        throw std::runtime_error("Hash must be valid base64 encoded string");
    }

    // Decode hash
    std::vector<BYTE> hashBytes = base64Decode(hashB64);
    if (hashBytes.empty()) {
        throw std::runtime_error("Invalid base64 hash");
    }

    // Open certificate store
    HCERTSTORE hStore = CertOpenSystemStoreA(0, "MY");
    if (!hStore) {
        throw std::runtime_error("Failed to open certificate store");
    }

    // Find certificate by thumbprint
    CRYPT_HASH_BLOB hashBlob;
    BYTE thumbprintBytes[20];
    
    // Convert hex string to bytes
    for (size_t i = 0; i < 20 && i * 2 < thumbprint.length(); i++) {
        sscanf_s(thumbprint.c_str() + (i * 2), "%2hhx", &thumbprintBytes[i]);
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
        CRYPT_ACQUIRE_SILENT_FLAG,
        nullptr,
        &hCryptProvOrNCryptKey,
        &keySpec,
        &freeProvOrKey);

    if (!hasPrivateKey) {
        CertFreeCertificateContext(certContext);
        CertCloseStore(hStore, 0);
        throw std::runtime_error("Certificate has no private key");
    }

    std::string signatureB64;
    
    try {
        if (keySpec == CERT_NCRYPT_KEY_SPEC) {
            // Use CNG (Cryptography Next Generation) API
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
                throw std::runtime_error("Failed to get signature size");
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
                throw std::runtime_error("Failed to sign hash");
            }

            signatureB64 = base64Encode(signature.data(), signatureSize);
        }
        else {
            // Use legacy CryptoAPI
            HCRYPTHASH hHash;
            if (!CryptCreateHash(hCryptProvOrNCryptKey, CALG_SHA_256, 0, 0, &hHash)) {
                throw std::runtime_error("Failed to create hash object");
            }

            if (!CryptSetHashParam(hHash, HP_HASHVAL, hashBytes.data(), 0)) {
                CryptDestroyHash(hHash);
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
            signatureB64 = base64Encode(signature.data(), signatureSize);
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

// Simple JSON parser (very basic - just enough for our needs)
std::map<std::string, std::string> parseJson(const std::string& json) {
    std::map<std::string, std::string> result;
    
    std::regex keyValueRegex("\"([^\"]+)\"\\s*:\\s*\"([^\"]+)\"");
    auto begin = std::sregex_iterator(json.begin(), json.end(), keyValueRegex);
    auto end = std::sregex_iterator();
    
    for (auto i = begin; i != end; ++i) {
        std::smatch match = *i;
        result[match[1].str()] = match[2].str();
    }
    
    return result;
}

// Read message from stdin (Chrome Native Messaging protocol)
bool readMessage(std::string& message) {
    // Read 4-byte length header
    uint32_t messageLength = 0;
    std::cin.read(reinterpret_cast<char*>(&messageLength), sizeof(messageLength));
    
    if (std::cin.eof() || std::cin.gcount() != sizeof(messageLength)) {
        return false;
    }
    
    if (messageLength <= 0 || messageLength > 10 * 1024 * 1024) {
        std::cerr << "Invalid message length: " << messageLength << std::endl;
        return false;
    }
    
    // Read message
    std::vector<char> buffer(messageLength);
    std::cin.read(buffer.data(), messageLength);
    
    if (std::cin.gcount() != messageLength) {
        std::cerr << "Incomplete message received" << std::endl;
        return false;
    }
    
    message = std::string(buffer.data(), messageLength);
    return true;
}

// Write message to stdout (Chrome Native Messaging protocol)
void writeMessage(const std::string& message) {
    uint32_t messageLength = static_cast<uint32_t>(message.length());
    
    std::cout.write(reinterpret_cast<const char*>(&messageLength), sizeof(messageLength));
    std::cout.write(message.c_str(), messageLength);
    std::cout.flush();
}

// Handle incoming message
void handleMessage(const std::string& messageJson) {
    std::cerr << "Received message: " << messageJson << std::endl;
    
    try {
        auto params = parseJson(messageJson);
        
        if (params.find("action") == params.end()) {
            throw std::runtime_error("No action specified");
        }
        
        std::string action = params["action"];
        
        if (action == "listCerts") {
            std::string certs = listCertificates();
            JsonBuilder response;
            response.addArray("result", certs);
            writeMessage(response.toString());
        }
        else if (action == "sign") {
            if (params.find("hash") == params.end() || params.find("thumbprint") == params.end()) {
                throw std::runtime_error("Missing required parameters for sign action");
            }
            
            std::string signature = signHash(params["hash"], params["thumbprint"]);
            JsonBuilder response;
            response.addString("result", signature);
            writeMessage(response.toString());
        }
        else {
            throw std::runtime_error("Unknown action: " + action);
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        JsonBuilder errorResponse;
        errorResponse.addString("error", ex.what());
        writeMessage(errorResponse.toString());
    }
}

int main() {
    // Set binary mode for stdin/stdout
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
    
    try {
        std::string message;
        while (readMessage(message)) {
            handleMessage(message);
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return 1;
    }
    
    return 0;
}
