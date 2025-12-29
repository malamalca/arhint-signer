/**
 * ArhintSigner Web Service
 * 
 * A Windows HTTP service for digitally signing hashes using certificates
 * from the Windows certificate store.
 * 
 * Architecture:
 * - include/http_server.h       : HTTP server initialization and request loop
 * - include/request_handler.h   : Request routing and endpoint handling
 * - include/certificate_manager.h : Certificate operations (list, sign)
 * - include/http_utils.h        : HTTP response utilities
 * - include/json_utils.h        : JSON serialization/parsing
 * - include/crypto_utils.h      : Base64 encoding/decoding
 * - include/string_utils.h      : String manipulation utilities
 * - include/system_tray.h       : System tray icon management
 */

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <atomic>
#include <thread>

#include "include/http_server.h"
#include "include/request_handler.h"
#include "include/system_tray.h"

using namespace ArhintSigner;

// Global flag for clean shutdown
std::atomic<bool> g_running(true);

// Forward declare tray icon pointer
SystemTray::TrayIcon* g_trayIcon = nullptr;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // No console created at startup - prevents flicker and taskbar icon
    // Console will be allocated only when user requests it via tray menu
    
    // Parse port from command line (default: 8082)
    int port = 8082;
    if (lpCmdLine && strlen(lpCmdLine) > 0) {
        port = atoi(lpCmdLine);
        if (port <= 0 || port > 65535) {
            port = 8082;
        }
    }

    // Create and initialize server
    Server::HttpServer server(port);
    
    if (!server.initialize()) {
        // Show error in message box since no console exists
        std::wstring errorMsg = L"Failed to initialize HTTP server on port " + std::to_wstring(port) + 
                               L"\n\nMake sure the URL is reserved:\nnetsh http add urlacl url=http://+:" + 
                               std::to_wstring(port) + L"/ user=Everyone";
        MessageBoxW(NULL, errorMsg.c_str(), L"ArhintSigner Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    // Initialize system tray icon
    SystemTray::TrayIcon trayIcon;
    g_trayIcon = &trayIcon;  // Set global pointer for console handler
    std::wstring tooltip = L"ArhintSigner Web Service\nPort: " + std::to_wstring(port);
    
    if (!trayIcon.initialize(tooltip, []() {
        g_running = false;
    }, true)) {  // Pass true to indicate console is hidden
        MessageBoxW(NULL, L"Failed to create system tray icon", L"ArhintSigner Warning", MB_ICONWARNING | MB_OK);
    } else {
        // Show balloon notification
        trayIcon.showBalloon(L"ArhintSigner Web Service", 
                            L"Service is running on port " + std::to_wstring(port) + L"\nDouble-click tray icon to show console", 
                            NIIF_INFO);
    }

    // Start HTTP processing in a separate thread
    std::thread httpThread([&server]() {
        server.processRequests(RequestHandler::handleRequest);
    });

    // Main message loop for tray icon
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!g_running) {
            break;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    trayIcon.cleanup();
    server.shutdown();
    
    // Wait for HTTP thread to finish
    if (httpThread.joinable()) {
        httpThread.join();
    }
    
    return 0;
}
