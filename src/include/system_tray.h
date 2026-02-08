#pragma once

#include <windows.h>
#include <shellapi.h>
#include <string>
#include <functional>

namespace ArhintSigner {
namespace SystemTray {

constexpr UINT WM_TRAYICON = WM_USER + 1;
constexpr UINT ID_TRAY_ICON = 1;
constexpr UINT ID_TRAY_EXIT = 2001;
constexpr UINT ID_TRAY_SHOW = 2002;
constexpr UINT ID_TRAY_HIDE = 2003;

// Application icon resource ID (must match app-resource.rc)
#define IDI_APPLICATION_ICON 101

/**
 * System Tray Icon Manager
 */
class TrayIcon {
private:
    NOTIFYICONDATAW nid;
    HWND hWnd;
    HMENU hMenu;
    bool consoleVisible;
    std::function<void()> exitCallback;

    static TrayIcon* instance;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        if (instance) {
            return instance->handleMessage(hwnd, uMsg, wParam, lParam);
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    LRESULT handleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
            case WM_TRAYICON:
                if (lParam == WM_RBUTTONUP) {
                    showContextMenu();
                } else if (lParam == WM_LBUTTONDBLCLK) {
                    toggleConsole();
                }
                return 0;

            case WM_COMMAND:
                switch (LOWORD(wParam)) {
                    case ID_TRAY_EXIT:
                        if (exitCallback) {
                            exitCallback();
                        }
                        DestroyWindow(hwnd);
                        return 0;
                    case ID_TRAY_SHOW:
                        showConsole();
                        return 0;
                    case ID_TRAY_HIDE:
                        hideConsole();
                        return 0;
                }
                break;

            case WM_DESTROY:
                Shell_NotifyIconW(NIM_DELETE, &nid);
                PostQuitMessage(0);
                return 0;
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    void showContextMenu() {
        POINT pt;
        GetCursorPos(&pt);
        
        // Required for proper menu dismissal
        SetForegroundWindow(hWnd);
        
        TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
        
        // Required for proper menu dismissal
        PostMessage(hWnd, WM_NULL, 0, 0);
    }

    void updateMenu() {
        if (consoleVisible) {
            EnableMenuItem(hMenu, ID_TRAY_SHOW, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hMenu, ID_TRAY_HIDE, MF_BYCOMMAND | MF_ENABLED);
        } else {
            EnableMenuItem(hMenu, ID_TRAY_SHOW, MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, ID_TRAY_HIDE, MF_BYCOMMAND | MF_GRAYED);
        }
    }

    void toggleConsole() {
        if (consoleVisible) {
            hideConsole();
        } else {
            showConsole();
        }
    }

public:
    TrayIcon() : hWnd(nullptr), hMenu(nullptr), consoleVisible(true) {
        instance = this;
        ZeroMemory(&nid, sizeof(nid));
    }

    ~TrayIcon() {
        cleanup();
        instance = nullptr;
    }

    void showConsole() {
        HWND consoleWnd = GetConsoleWindow();
        if (!consoleWnd) {
            // Allocate console if it doesn't exist
            if (AllocConsole()) {
                // Redirect stdout/stderr to console
                FILE* fp = nullptr;
                freopen_s(&fp, "CONOUT$", "w", stdout);
                freopen_s(&fp, "CONOUT$", "w", stderr);
                freopen_s(&fp, "CONIN$", "r", stdin);
                
                // Clear error flags and sync
                std::cout.clear();
                std::cerr.clear();
                std::cin.clear();
                
                consoleWnd = GetConsoleWindow();
                if (consoleWnd) {
                    // Set console properties to prevent taskbar icon
                    LONG style = GetWindowLong(consoleWnd, GWL_EXSTYLE);
                    SetWindowLong(consoleWnd, GWL_EXSTYLE, style | WS_EX_TOOLWINDOW);
                    SetConsoleTitleW(L"ArhintSigner Web Service - Console");
                    
                    // Disable the close button on console window
                    HMENU hSysMenu = GetSystemMenu(consoleWnd, FALSE);
                    if (hSysMenu) {
                        DeleteMenu(hSysMenu, SC_CLOSE, MF_BYCOMMAND);
                    }
                    
                    // Print initial message
                    std::cout << "ArhintSigner Web Service Console" << std::endl;
                    std::cout << "================================" << std::endl;
                    std::cout << "Console allocated successfully." << std::endl;
                    std::cout << "Use tray icon menu or double-click tray to hide console" << std::endl;
                    std::cout << std::endl;
                }
            }
        }
        if (consoleWnd) {
            ShowWindow(consoleWnd, SW_SHOW);
            SetForegroundWindow(consoleWnd);
            consoleVisible = true;
            updateMenu();
        }
    }

    void hideConsole() {
        HWND consoleWnd = GetConsoleWindow();
        if (consoleWnd) {
            ShowWindow(consoleWnd, SW_HIDE);
            consoleVisible = false;
            updateMenu();
        }
    }

    bool initialize(const std::wstring& tooltip, std::function<void()> onExit = nullptr, bool startHidden = false) {
        exitCallback = onExit;
        
        // Set initial console visibility state
        HWND consoleWnd = GetConsoleWindow();
        if (consoleWnd) {
            consoleVisible = IsWindowVisible(consoleWnd) != 0;
            if (startHidden && consoleVisible) {
                consoleVisible = false;
            }
        } else {
            // No console exists (GUI app) - mark as hidden
            consoleVisible = false;
        }

        // Register window class
        WNDCLASSEXW wc = { 0 };
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"ArhintSignerTrayClass";
        
        if (!RegisterClassExW(&wc)) {
            return false;
        }

        // Create hidden window for message handling
        hWnd = CreateWindowExW(0, L"ArhintSignerTrayClass", L"ArhintSigner Tray",
                               0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
        
        if (!hWnd) {
            return false;
        }

        // Create context menu
        hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, ID_TRAY_SHOW, L"Show Console");
        AppendMenuW(hMenu, MF_STRING, ID_TRAY_HIDE, L"Hide Console");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
        
        updateMenu();

        // Setup tray icon
        nid.cbSize = sizeof(NOTIFYICONDATAW);
        nid.hWnd = hWnd;
        nid.uID = ID_TRAY_ICON;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_TRAYICON;
        
        // Load icon from executable resources
        HINSTANCE hInstance = GetModuleHandle(NULL);
        nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION_ICON));
        if (!nid.hIcon) {
            // Fallback to default icon if resource icon not found
            nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        }
        
        // Set tooltip
        wcsncpy_s(nid.szTip, tooltip.c_str(), _TRUNCATE);

        // Add icon to system tray
        if (!Shell_NotifyIconW(NIM_ADD, &nid)) {
            DestroyWindow(hWnd);
            return false;
        }

        return true;
    }

    void updateTooltip(const std::wstring& tooltip) {
        wcsncpy_s(nid.szTip, tooltip.c_str(), _TRUNCATE);
        Shell_NotifyIconW(NIM_MODIFY, &nid);
    }

    void showBalloon(const std::wstring& title, const std::wstring& message, DWORD infoFlags = NIIF_INFO) {
        nid.uFlags |= NIF_INFO;
        wcsncpy_s(nid.szInfoTitle, title.c_str(), _TRUNCATE);
        wcsncpy_s(nid.szInfo, message.c_str(), _TRUNCATE);
        nid.dwInfoFlags = infoFlags;
        nid.uTimeout = 5000;
        Shell_NotifyIconW(NIM_MODIFY, &nid);
        nid.uFlags &= ~NIF_INFO;
    }

    void processMessages() {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                if (exitCallback) {
                    exitCallback();
                }
                return;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    void cleanup() {
        if (nid.hWnd) {
            Shell_NotifyIconW(NIM_DELETE, &nid);
        }
        if (hMenu) {
            DestroyMenu(hMenu);
            hMenu = nullptr;
        }
        if (hWnd) {
            DestroyWindow(hWnd);
            hWnd = nullptr;
        }
    }

    HWND getHWND() const { return hWnd; }
    bool isConsoleVisible() const { return consoleVisible; }
};

TrayIcon* TrayIcon::instance = nullptr;

} // namespace SystemTray
} // namespace ArhintSigner
