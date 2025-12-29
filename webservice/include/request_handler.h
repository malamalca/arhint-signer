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

            // Sign the hash
            std::string signature = Certificate::signHash(params["hash"], params["thumbprint"]);
            Json::Builder response;
            response.addString("result", signature);
            Http::sendResponse(hReqQueue, pRequest->RequestId, 200, "application/json", 
                             response.toString());
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
