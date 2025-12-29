#pragma once

#include <windows.h>
#include <http.h>
#include <string>
#include <iostream>

#pragma comment(lib, "httpapi.lib")

namespace ArhintSigner {
namespace Http {

/**
 * Send an HTTP response with optional CORS headers
 */
inline void sendResponse(HANDLE hReqQueue, HTTP_REQUEST_ID requestId, USHORT statusCode, 
                        const std::string& contentType, const std::string& body,
                        bool includeCors = true) {
    // Static CORS headers to ensure they persist during the HTTP API call
    static const char* corsOriginHeader = "Access-Control-Allow-Origin";
    static const char* corsOriginValue = "*";
    static const char* corsMethodsHeader = "Access-Control-Allow-Methods";
    static const char* corsMethodsValue = "GET, POST, OPTIONS";
    static const char* corsHeadersHeader = "Access-Control-Allow-Headers";
    static const char* corsHeadersValue = "Content-Type";
    
    HTTP_RESPONSE response;
    HTTP_DATA_CHUNK dataChunk;
    HTTP_UNKNOWN_HEADER unknownHeaders[3];
    
    ZeroMemory(&response, sizeof(response));
    ZeroMemory(&dataChunk, sizeof(dataChunk));
    
    // Set HTTP version
    response.Version.MajorVersion = 1;
    response.Version.MinorVersion = 1;
    
    response.StatusCode = statusCode;
    response.pReason = (statusCode == 200) ? "OK" : 
                       (statusCode == 400) ? "Bad Request" :
                       (statusCode == 404) ? "Not Found" :
                       (statusCode == 500) ? "Internal Server Error" : "Error";
    response.ReasonLength = (USHORT)strlen(response.pReason);

    // Add content-type header
    response.Headers.KnownHeaders[HttpHeaderContentType].pRawValue = contentType.c_str();
    response.Headers.KnownHeaders[HttpHeaderContentType].RawValueLength = (USHORT)contentType.length();

    // Add CORS headers
    if (includeCors) {
        ZeroMemory(unknownHeaders, sizeof(unknownHeaders));
        
        unknownHeaders[0].pName = corsOriginHeader;
        unknownHeaders[0].NameLength = (USHORT)strlen(corsOriginHeader);
        unknownHeaders[0].pRawValue = corsOriginValue;
        unknownHeaders[0].RawValueLength = (USHORT)strlen(corsOriginValue);

        unknownHeaders[1].pName = corsMethodsHeader;
        unknownHeaders[1].NameLength = (USHORT)strlen(corsMethodsHeader);
        unknownHeaders[1].pRawValue = corsMethodsValue;
        unknownHeaders[1].RawValueLength = (USHORT)strlen(corsMethodsValue);

        unknownHeaders[2].pName = corsHeadersHeader;
        unknownHeaders[2].NameLength = (USHORT)strlen(corsHeadersHeader);
        unknownHeaders[2].pRawValue = corsHeadersValue;
        unknownHeaders[2].RawValueLength = (USHORT)strlen(corsHeadersValue);

        response.Headers.pUnknownHeaders = unknownHeaders;
        response.Headers.UnknownHeaderCount = 3;
    }

    // Set response body
    if (!body.empty()) {
        dataChunk.DataChunkType = HttpDataChunkFromMemory;
        dataChunk.FromMemory.pBuffer = (PVOID)body.c_str();
        dataChunk.FromMemory.BufferLength = (ULONG)body.length();

        response.EntityChunkCount = 1;
        response.pEntityChunks = &dataChunk;
    }

    ULONG bytesSent;
    ULONG result = HttpSendHttpResponse(hReqQueue, requestId, 0, &response, nullptr, 
                                       &bytesSent, nullptr, 0, nullptr, nullptr);
    
    if (result != NO_ERROR) {
        std::cerr << "HttpSendHttpResponse failed with error: " << result << std::endl;
    } else {
        std::cout << "Response sent: " << statusCode << " (" << bytesSent << " bytes)" << std::endl;
    }
}

/**
 * Read the request body from an HTTP request
 */
inline std::string readRequestBody(HANDLE hReqQueue, PHTTP_REQUEST pRequest) {
    std::string requestBody;
    
    // First, try to get body from entity chunks if available
    if (pRequest->EntityChunkCount > 0 && pRequest->pEntityChunks) {
        HTTP_DATA_CHUNK* chunk = &pRequest->pEntityChunks[0];
        if (chunk->DataChunkType == HttpDataChunkFromMemory) {
            requestBody = std::string((char*)chunk->FromMemory.pBuffer, 
                                    chunk->FromMemory.BufferLength);
        }
    }
    
    // If not available, read the entity body
    if (requestBody.empty()) {
        std::vector<char> buffer(4096);
        ULONG bytesRead = 0;
        
        ULONG result = HttpReceiveRequestEntityBody(
            hReqQueue,
            pRequest->RequestId,
            0,
            buffer.data(),
            (ULONG)buffer.size(),
            &bytesRead,
            nullptr);
        
        if (result == NO_ERROR || result == ERROR_HANDLE_EOF) {
            if (bytesRead > 0) {
                requestBody = std::string(buffer.data(), bytesRead);
            }
        }
    }
    
    return requestBody;
}

} // namespace Http
} // namespace ArhintSigner
