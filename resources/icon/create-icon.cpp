// Simple Icon Generator for ArhintSigner
// Creates a basic ICO file with certificate/key imagery
// Compile: cl /EHsc /O2 create-icon.cpp /link gdi32.lib

#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>

#pragma pack(push, 2)
struct ICONDIR {
    WORD idReserved;
    WORD idType;
    WORD idCount;
};

struct ICONDIRENTRY {
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwBytesInRes;
    DWORD dwImageOffset;
};
#pragma pack(pop)

void DrawCertificateIcon(HDC hdc, int size) {
    // Background - Blue
    HBRUSH bgBrush = CreateSolidBrush(RGB(30, 58, 138));
    RECT bgRect = {0, 0, size, size};
    FillRect(hdc, &bgRect, bgBrush);
    DeleteObject(bgBrush);
    
    int margin = size / 8;
    int certWidth = size * 5 / 10;
    int certHeight = size * 7 / 10;
    int certX = margin;
    int certY = margin;
    
    // Certificate/Document - White
    HBRUSH certBrush = CreateSolidBrush(RGB(255, 255, 255));
    HPEN certPen = CreatePen(PS_SOLID, size/64, RGB(203, 213, 225));
    SelectObject(hdc, certBrush);
    SelectObject(hdc, certPen);
    RoundRect(hdc, certX, certY, certX + certWidth, certY + certHeight, size/20, size/20);
    DeleteObject(certBrush);
    DeleteObject(certPen);
    
    // Certificate lines (text representation)
    HPEN linePen = CreatePen(PS_SOLID, size/64, RGB(148, 163, 184));
    SelectObject(hdc, linePen);
    int lineY = certY + size/8;
    for (int i = 0; i < 3; i++) {
        MoveToEx(hdc, certX + size/16, lineY, NULL);
        LineTo(hdc, certX + certWidth - size/16, lineY);
        lineY += size/16;
    }
    DeleteObject(linePen);
    
    // Seal/Badge - Blue circle
    HBRUSH sealBrush = CreateSolidBrush(RGB(59, 130, 246));
    HPEN sealPen = CreatePen(PS_SOLID, size/64, RGB(30, 64, 175));
    SelectObject(hdc, sealBrush);
    SelectObject(hdc, sealPen);
    int sealRadius = size/8;
    int sealX = certX + certWidth/2;
    int sealY = certY + certHeight * 2/3;
    Ellipse(hdc, sealX - sealRadius, sealY - sealRadius, sealX + sealRadius, sealY + sealRadius);
    DeleteObject(sealBrush);
    DeleteObject(sealPen);
    
    // Key symbol - Orange
    int keyX = certX + certWidth + size/16;
    int keyY = certY + certHeight/3;
    int keyHeadRadius = size/10;
    
    // Key head (outer circle)
    HBRUSH keyBrush = CreateSolidBrush(RGB(245, 158, 11));
    HPEN keyPen = CreatePen(PS_SOLID, size/48, RGB(217, 119, 6));
    SelectObject(hdc, keyBrush);
    SelectObject(hdc, keyPen);
    Ellipse(hdc, keyX - keyHeadRadius, keyY - keyHeadRadius, 
            keyX + keyHeadRadius, keyY + keyHeadRadius);
    
    // Key head (inner circle)
    int innerRadius = keyHeadRadius / 2;
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Ellipse(hdc, keyX - innerRadius, keyY - innerRadius,
            keyX + innerRadius, keyY + innerRadius);
    
    // Key shaft
    SelectObject(hdc, keyBrush);
    RECT shaftRect = {keyX - keyHeadRadius - size/8, keyY - size/32, 
                      keyX + keyHeadRadius/4, keyY + size/32};
    RoundRect(hdc, shaftRect.left, shaftRect.top, shaftRect.right, shaftRect.bottom, 
              size/64, size/64);
    
    // Key teeth
    RECT tooth1 = {shaftRect.left, keyY - size/24, shaftRect.left + size/32, keyY + size/24};
    RECT tooth2 = {shaftRect.left + size/24, keyY - size/20, 
                   shaftRect.left + size/24 + size/32, keyY + size/20};
    Rectangle(hdc, tooth1.left, tooth1.top, tooth1.right, tooth1.bottom);
    Rectangle(hdc, tooth2.left, tooth2.top, tooth2.right, tooth2.bottom);
    
    DeleteObject(keyBrush);
    DeleteObject(keyPen);
}

bool CreateIconFile(const char* filename, int size) {
    // Create bitmap
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size;
    bmi.bmiHeader.biHeight = -size; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    void* pBits = nullptr;
    HBITMAP hBitmap = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    SelectObject(hdcMem, hBitmap);
    
    // Draw icon
    DrawCertificateIcon(hdcMem, size);
    
    // Get bitmap data
    BITMAP bitmap;
    GetObject(hBitmap, sizeof(BITMAP), &bitmap);
    DWORD imageSize = bitmap.bmWidthBytes * bitmap.bmHeight;
    
    // Create ICO file
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        return false;
    }
    
    ICONDIR iconDir = {0, 1, 1};
    file.write((char*)&iconDir, sizeof(iconDir));
    
    ICONDIRENTRY iconEntry = {};
    iconEntry.bWidth = (size >= 256) ? 0 : size;
    iconEntry.bHeight = (size >= 256) ? 0 : size;
    iconEntry.bColorCount = 0;
    iconEntry.wPlanes = 1;
    iconEntry.wBitCount = 32;
    iconEntry.dwBytesInRes = sizeof(BITMAPINFOHEADER) + imageSize;
    iconEntry.dwImageOffset = sizeof(ICONDIR) + sizeof(ICONDIRENTRY);
    file.write((char*)&iconEntry, sizeof(iconEntry));
    
    // Write bitmap header
    BITMAPINFOHEADER bmpHeader = bmi.bmiHeader;
    bmpHeader.biHeight = size * 2; // Include AND mask
    file.write((char*)&bmpHeader, sizeof(bmpHeader));
    
    // Write bitmap data
    file.write((char*)pBits, imageSize);
    
    // Write AND mask (all transparent)
    std::vector<BYTE> andMask(size * size / 8, 0);
    file.write((char*)andMask.data(), andMask.size());
    
    file.close();
    
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    
    return true;
}

int main() {
    std::cout << "ArhintSigner Icon Generator\n";
    std::cout << "============================\n\n";
    
    const int sizes[] = {16, 32, 48, 64, 128, 256};
    
    for (int size : sizes) {
        char filename[256];
        sprintf_s(filename, "app-icon-%d.ico", size);
        
        std::cout << "Creating " << filename << "... ";
        if (CreateIconFile(filename, size)) {
            std::cout << "OK\n";
        } else {
            std::cout << "FAILED\n";
        }
    }
    
    std::cout << "\nDone! Icons created successfully.\n";
    std::cout << "Main icon: app-icon-32.ico (recommended for system tray)\n";
    
    return 0;
}
