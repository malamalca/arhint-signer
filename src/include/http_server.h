#pragma once

#include <windows.h>
#include <http.h>
#include <string>
#include <vector>
#include <iostream>
#include <atomic>

#pragma comment(lib, "httpapi.lib")

// Forward declare global running flag
extern std::atomic<bool> g_running;

namespace ArhintSigner {
namespace Server {

/**
 * HTTP Server configuration and state
 */
class HttpServer {
private:
    HTTPAPI_VERSION httpApiVersion;
    HTTP_SERVER_SESSION_ID sessionId;
    HTTP_URL_GROUP_ID urlGroupId;
    HANDLE hReqQueue;
    int port;
    bool initialized;

public:
    HttpServer(int serverPort = 8082) 
        : httpApiVersion(HTTPAPI_VERSION_2)
        , sessionId(0)
        , urlGroupId(0)
        , hReqQueue(nullptr)
        , port(serverPort)
        , initialized(false) {
    }

    ~HttpServer() {
        shutdown();
    }

    /**
     * Initialize and start the HTTP server
     */
    bool initialize() {
        if (initialized) return true;

        // Initialize HTTP API
        ULONG result = HttpInitialize(httpApiVersion, HTTP_INITIALIZE_SERVER, nullptr);
        if (result != NO_ERROR) {
            return false;
        }

        // Create server session
        result = HttpCreateServerSession(httpApiVersion, &sessionId, 0);
        if (result != NO_ERROR) {
            std::cerr << "HttpCreateServerSession failed with error " << result << std::endl;
            HttpTerminate(HTTP_INITIALIZE_SERVER, nullptr);
            return false;
        }

        // Create URL group
        result = HttpCreateUrlGroup(sessionId, &urlGroupId, 0);
        if (result != NO_ERROR) {
            std::cerr << "HttpCreateUrlGroup failed with error " << result << std::endl;
            HttpCloseServerSession(sessionId);
            HttpTerminate(HTTP_INITIALIZE_SERVER, nullptr);
            return false;
        }

        // Create request queue
        result = HttpCreateRequestQueue(httpApiVersion, nullptr, nullptr, 0, &hReqQueue);
        if (result != NO_ERROR) {
            std::cerr << "HttpCreateRequestQueue failed with error " << result << std::endl;
            HttpCloseUrlGroup(urlGroupId);
            HttpCloseServerSession(sessionId);
            HttpTerminate(HTTP_INITIALIZE_SERVER, nullptr);
            return false;
        }

        // Bind URL group to request queue
        HTTP_BINDING_INFO bindingInfo;
        bindingInfo.Flags.Present = 1;
        bindingInfo.RequestQueueHandle = hReqQueue;

        result = HttpSetUrlGroupProperty(urlGroupId, HttpServerBindingProperty, 
                                         &bindingInfo, sizeof(bindingInfo));
        if (result != NO_ERROR) {
            std::cerr << "HttpSetUrlGroupProperty failed with error " << result << std::endl;
            HttpCloseRequestQueue(hReqQueue);
            HttpCloseUrlGroup(urlGroupId);
            HttpCloseServerSession(sessionId);
            HttpTerminate(HTTP_INITIALIZE_SERVER, nullptr);
            return false;
        }

        // Add URL to listen on
        std::wstring urlPrefix = L"http://localhost:" + std::to_wstring(port) + L"/";
        result = HttpAddUrlToUrlGroup(urlGroupId, urlPrefix.c_str(), 0, 0);
        if (result != NO_ERROR) {
            HttpCloseRequestQueue(hReqQueue);
            HttpCloseUrlGroup(urlGroupId);
            HttpCloseServerSession(sessionId);
            HttpTerminate(HTTP_INITIALIZE_SERVER, nullptr);
            return false;
        }

        initialized = true;
        return true;
    }

    /**
     * Process incoming requests (blocking call)
     */
    template<typename RequestHandler>
    void processRequests(RequestHandler handler) {
        if (!initialized) {
            return;
        }

        ULONG requestBufferSize = sizeof(HTTP_REQUEST) + 2048;
        std::vector<BYTE> requestBuffer(requestBufferSize);
        PHTTP_REQUEST pRequest = (PHTTP_REQUEST)requestBuffer.data();

        while (g_running) {
            RtlZeroMemory(pRequest, requestBufferSize);
            ULONG bytesRead;

            ULONG result = HttpReceiveHttpRequest(hReqQueue, HTTP_NULL_ID, 0, pRequest, 
                                                  requestBufferSize, &bytesRead, nullptr);

            if (result == NO_ERROR) {
                handler(hReqQueue, pRequest);
            }
            else if (result == ERROR_MORE_DATA) {
                // Request buffer too small, resize and retry
                requestBufferSize = bytesRead;
                requestBuffer.resize(requestBufferSize);
                pRequest = (PHTTP_REQUEST)requestBuffer.data();

                result = HttpReceiveHttpRequest(hReqQueue, pRequest->RequestId, 0, pRequest,
                                               requestBufferSize, &bytesRead, nullptr);
                if (result == NO_ERROR) {
                    handler(hReqQueue, pRequest);
                }
            }
            else if (result == ERROR_CONNECTION_INVALID) {
                // Connection closed, continue
                continue;
            }
            else {
                // Error receiving request
                break;
            }
        }
    }

    /**
     * Process one request (non-blocking)
     * Returns true if a request was processed, false if no request available
     */
    template<typename RequestHandler>
    bool processOneRequest(RequestHandler handler, ULONG timeoutMs = 0) {
        if (!initialized) {
            return false;
        }

        ULONG requestBufferSize = sizeof(HTTP_REQUEST) + 2048;
        std::vector<BYTE> requestBuffer(requestBufferSize);
        PHTTP_REQUEST pRequest = (PHTTP_REQUEST)requestBuffer.data();

        RtlZeroMemory(pRequest, requestBufferSize);
        ULONG bytesRead;

        // Use overlapped I/O for non-blocking operation
        OVERLAPPED overlapped = { 0 };
        overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        ULONG result = HttpReceiveHttpRequest(hReqQueue, HTTP_NULL_ID, 
                                              HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY,
                                              pRequest, requestBufferSize, &bytesRead, &overlapped);

        bool processed = false;

        if (result == ERROR_IO_PENDING) {
            // Wait for request with timeout
            DWORD waitResult = WaitForSingleObject(overlapped.hEvent, timeoutMs);
            if (waitResult == WAIT_OBJECT_0) {
                // Get the result of the overlapped operation
                DWORD bytesTransferred = 0;
                if (GetOverlappedResult(hReqQueue, &overlapped, &bytesTransferred, FALSE)) {
                    // Request received successfully
                    handler(hReqQueue, pRequest);
                    processed = true;
                }
            }
        }
        else if (result == NO_ERROR) {
            handler(hReqQueue, pRequest);
            processed = true;
        }
        else if (result == ERROR_MORE_DATA) {
            // Request buffer too small, resize and retry
            requestBufferSize = bytesRead;
            requestBuffer.resize(requestBufferSize);
            pRequest = (PHTTP_REQUEST)requestBuffer.data();

            result = HttpReceiveHttpRequest(hReqQueue, pRequest->RequestId, 0, pRequest,
                                           requestBufferSize, &bytesRead, nullptr);
            if (result == NO_ERROR) {
                handler(hReqQueue, pRequest);
                processed = true;
            }
        }

        CloseHandle(overlapped.hEvent);
        return processed;
    }

    /**
     * Shutdown the server and cleanup resources
     */
    void shutdown() {
        if (!initialized) return;

        std::wstring urlPrefix = L"http://localhost:" + std::to_wstring(port) + L"/";
        
        if (hReqQueue) {
            HttpCloseRequestQueue(hReqQueue);
        }
        if (urlGroupId) {
            HttpRemoveUrlFromUrlGroup(urlGroupId, urlPrefix.c_str(), 0);
            HttpCloseUrlGroup(urlGroupId);
        }
        if (sessionId) {
            HttpCloseServerSession(sessionId);
        }
        HttpTerminate(HTTP_INITIALIZE_SERVER, nullptr);

        initialized = false;
    }

    int getPort() const { return port; }
    bool isInitialized() const { return initialized; }
};

} // namespace Server
} // namespace ArhintSigner
