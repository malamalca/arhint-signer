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

        // Handle /health endpoint (for health checks)
        if (path == "/health" && pRequest->Verb == HttpVerbGET) {
            Json::Builder response;
            response.addString("status", "ok");
            response.addString("service", "ArhintSigner");
            Http::sendResponse(hReqQueue, pRequest->RequestId, 200, "application/json", response.toString());
            return;
        }

        // Handle /certificates endpoint (modern API)
        if (path == "/certificates" && pRequest->Verb == HttpVerbGET) {
            std::cout << "Listing certificates..." << std::endl;
            std::string certs = Certificate::listCertificates();
            std::cout << "Found certificates, sending response..." << std::endl;
            
            Json::Builder response;
            response.addArray("certificates", certs);
            std::string responseStr = response.toString();
            
            std::cout << "Response size: " << responseStr.length() << " bytes" << std::endl;
            Http::sendResponse(hReqQueue, pRequest->RequestId, 200, "application/json", responseStr);
            std::cout << "Response sent successfully" << std::endl;
            return;
        }

        // Handle root path - display service info page
        if (path == "/" && pRequest->Verb == HttpVerbGET) {
            std::string homePage = R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ArhintSigner Web Service</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            max-width: 900px;
            margin: 50px auto;
            padding: 20px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: #333;
        }
        .container {
            background: white;
            border-radius: 10px;
            padding: 40px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.2);
        }
        h1 {
            color: #667eea;
            border-bottom: 3px solid #667eea;
            padding-bottom: 10px;
            margin-bottom: 20px;
        }
        h2 {
            color: #764ba2;
            margin-top: 30px;
        }
        .version {
            color: #666;
            font-size: 0.9em;
            margin-bottom: 20px;
        }
        .endpoint {
            background: #f8f9fa;
            border-left: 4px solid #667eea;
            padding: 15px;
            margin: 15px 0;
            border-radius: 5px;
        }
        .endpoint-title {
            font-weight: bold;
            color: #667eea;
            font-size: 1.1em;
            margin-bottom: 5px;
        }
        .endpoint-method {
            display: inline-block;
            background: #764ba2;
            color: white;
            padding: 3px 8px;
            border-radius: 3px;
            font-size: 0.85em;
            margin-right: 10px;
        }
        .endpoint-description {
            color: #666;
            margin-top: 5px;
        }
        .status {
            background: #d4edda;
            border: 1px solid #c3e6cb;
            color: #155724;
            padding: 12px;
            border-radius: 5px;
            margin: 20px 0;
        }
        code {
            background: #f4f4f4;
            padding: 2px 6px;
            border-radius: 3px;
            font-family: 'Courier New', monospace;
        }
        a {
            color: #667eea;
            text-decoration: none;
        }
        a:hover {
            text-decoration: underline;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔐 ArhintSigner Web Service</h1>
        <div class="version">Version 1.0.0</div>
        
        <div class="status">
            ✓ Service is running and ready to process requests
        </div>
        
        <h2>📋 Available Endpoints</h2>
        
        <div class="endpoint">
            <div class="endpoint-title">
                <span class="endpoint-method">GET</span>
                <code>/listCerts</code>
            </div>
            <div class="endpoint-description">
                List all available signing certificates from the Windows certificate store.
                Returns a JSON array of certificates with their thumbprints, subject names, and validity dates.
            </div>
        </div>
        
        <div class="endpoint">
            <div class="endpoint-title">
                <span class="endpoint-method">POST</span>
                <code>/sign</code>
            </div>
            <div class="endpoint-description">
                Sign a hash using a specified certificate. Requires JSON body with <code>hash</code> (base64-encoded) 
                and <code>thumbprint</code> (40-char hex string) parameters. Returns the digital signature.
            </div>
        </div>
        
        <h2>📚 API Documentation</h2>
        <p>
            <strong>Base URL:</strong> <code>http://localhost:8082</code>
        </p>
        <p>
            <strong>Response Format:</strong> JSON<br>
            <strong>CORS:</strong> Enabled for cross-origin requests
        </p>
        
        <h2>🔒 Security</h2>
        <ul>
            <li>All certificates are accessed from the Windows certificate store</li>
            <li>Only locally installed certificates can be used for signing</li>
            <li>Request body size limited to 10KB to prevent DoS attacks</li>
            <li>Input validation on all parameters</li>
        </ul>
        
        <h2>ℹ️ About</h2>
        <p>
            ArhintSigner Web Service provides a secure HTTP API for digital signing operations using 
            Windows certificate store. It's designed for local applications that need to leverage 
            system certificates for cryptographic operations.
        </p>
    </div>
</body>
</html>)";
            Http::sendResponse(hReqQueue, pRequest->RequestId, 200, "text/html", homePage);
            return;
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
