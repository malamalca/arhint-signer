#pragma once

#include <windows.h>
#include <http.h>
#include <string>
#include <iostream>
#include "http_utils.h"
#include "json_utils.h"
#include "certificate_manager.h"

namespace ArhintSigner {
namespace RequestHandler {

/**
 * Handle incoming HTTP requests and route to appropriate handlers
 */
inline void handleRequest(HANDLE hReqQueue, PHTTP_REQUEST pRequest) {
    std::string url(pRequest->pRawUrl);
    std::string method(pRequest->Verb == HttpVerbGET ? "GET" : 
                      pRequest->Verb == HttpVerbPOST ? "POST" :
                      pRequest->Verb == HttpVerbOPTIONS ? "OPTIONS" : "UNKNOWN");

    std::cout << "Request: " << method << " " << url << std::endl;

    try {
        // Handle OPTIONS for CORS preflight
        if (pRequest->Verb == HttpVerbOPTIONS) {
            Http::sendResponse(hReqQueue, pRequest->RequestId, 200, "text/plain", "");
            return;
        }

        // Parse request path
        std::string path = url;
        size_t queryPos = path.find('?');
        if (queryPos != std::string::npos) {
            path = path.substr(0, queryPos);
        }

        // Handle /listCerts endpoint
        if (path == "/listCerts" || path == "/api/listCerts") {
            std::cout << "Listing certificates..." << std::endl;
            std::string certs = Certificate::listCertificates();
            std::cout << "Found certificates, sending response..." << std::endl;
            
            Json::Builder response;
            response.addArray("result", certs);
            std::string responseStr = response.toString();
            
            std::cout << "Response size: " << responseStr.length() << " bytes" << std::endl;
            Http::sendResponse(hReqQueue, pRequest->RequestId, 200, "application/json", responseStr);
            std::cout << "Response sent successfully" << std::endl;
            return;
        }

        // Handle /sign endpoint
        if ((path == "/sign" || path == "/api/sign") && pRequest->Verb == HttpVerbPOST) {
            std::string requestBody = Http::readRequestBody(hReqQueue, pRequest);

            if (requestBody.empty()) {
                Json::Builder errorResponse;
                errorResponse.addString("error", "Request body is required");
                Http::sendResponse(hReqQueue, pRequest->RequestId, 400, "application/json", 
                                  errorResponse.toString());
                return;
            }
            
            // Security: Limit request body size to prevent DoS (max 10KB)
            if (requestBody.length() > 10240) {
                Json::Builder errorResponse;
                errorResponse.addString("error", "Request body too large (max 10KB)");
                Http::sendResponse(hReqQueue, pRequest->RequestId, 413, "application/json", 
                                  errorResponse.toString());
                return;
            }

            std::cout << "Request body: " << requestBody << std::endl;

            // Parse JSON request
            auto params = Json::parse(requestBody);
            
            std::cout << "Parsed params - hash: '" << params["hash"] 
                     << "', thumbprint: '" << params["thumbprint"] << "'" << std::endl;
            
            if (params.find("hash") == params.end() || params.find("thumbprint") == params.end()) {
                Json::Builder errorResponse;
                errorResponse.addString("error", "Missing required parameters: hash and thumbprint");
                Http::sendResponse(hReqQueue, pRequest->RequestId, 400, "application/json", 
                                  errorResponse.toString());
                return;
            }
            
            // Security: Validate hash parameter (base64 encoded, reasonable length)
            const std::string& hash = params["hash"];
            if (hash.empty() || hash.length() > 1024) {
                Json::Builder errorResponse;
                errorResponse.addString("error", "Invalid hash parameter (max 1024 chars)");
                Http::sendResponse(hReqQueue, pRequest->RequestId, 400, "application/json", 
                                  errorResponse.toString());
                return;
            }
            
            // Security: Validate thumbprint (40 hex chars for SHA1)
            const std::string& thumbprint = params["thumbprint"];
            if (thumbprint.length() != 40) {
                Json::Builder errorResponse;
                errorResponse.addString("error", "Invalid thumbprint (must be 40 hex characters)");
                Http::sendResponse(hReqQueue, pRequest->RequestId, 400, "application/json", 
                                  errorResponse.toString());
                return;
            }
            
            // Security: Validate thumbprint contains only hex characters
            for (char c : thumbprint) {
                if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
                    Json::Builder errorResponse;
                    errorResponse.addString("error", "Invalid thumbprint (must contain only hex characters)");
                    Http::sendResponse(hReqQueue, pRequest->RequestId, 400, "application/json", 
                                      errorResponse.toString());
                    return;
                }
            }

            // Sign the hash
            try {
                std::string signature = Certificate::signHash(params["hash"], params["thumbprint"]);
                Json::Builder response;
                response.addString("result", signature);
                Http::sendResponse(hReqQueue, pRequest->RequestId, 200, "application/json", 
                                 response.toString());
            }
            catch (const std::exception& ex) {
                std::string errorMsg = ex.what();
                // Check if this is a validation error (user input error) or server error
                bool isValidationError = 
                    errorMsg.find("Invalid") != std::string::npos ||
                    errorMsg.find("required") != std::string::npos ||
                    errorMsg.find("must be") != std::string::npos ||
                    errorMsg.find("Expected") != std::string::npos ||
                    errorMsg.find("not found") != std::string::npos;
                
                int statusCode = isValidationError ? 400 : 500;
                Json::Builder errorResponse;
                errorResponse.addString("error", errorMsg);
                Http::sendResponse(hReqQueue, pRequest->RequestId, static_cast<USHORT>(statusCode), "application/json", 
                                  errorResponse.toString());
            }
            return;
        }

        // 404 Not Found
        Json::Builder errorResponse;
        errorResponse.addString("error", "Endpoint not found");
        Http::sendResponse(hReqQueue, pRequest->RequestId, 404, "application/json", 
                          errorResponse.toString());
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        Json::Builder errorResponse;
        errorResponse.addString("error", ex.what());
        Http::sendResponse(hReqQueue, pRequest->RequestId, 500, "application/json", 
                          errorResponse.toString());
    }
}

} // namespace RequestHandler
} // namespace ArhintSigner
