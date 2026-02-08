# Makefile for ArhintSigner Web Service
# Windows C++ HTTP Server

CXX = cl
RC = rc
CXXFLAGS = /std:c++17 /EHsc /O2 /W3 /I"src/include"
LDFLAGS = /SUBSYSTEM:WINDOWS /ENTRY:WinMainCRTStartup httpapi.lib crypt32.lib ncrypt.lib ws2_32.lib advapi32.lib shell32.lib user32.lib
RELEASE_DIR = release
TARGET = $(RELEASE_DIR)\arhint-signer.exe
SOURCE = src\arhint-signer.cpp
RESOURCE = resources\app-resource.rc
RESOURCE_OBJ = resources\app-resource.res
ICON_GEN = resources\icon\create-icon.exe
ICON_SOURCE = resources\icon\create-icon.cpp

.PHONY: all clean run icons

all: icons $(TARGET)

icons: $(ICON_GEN)
	@echo Generating icon files...
	@cd resources\icon && ..\..\$(ICON_GEN)
	@echo Icon generation complete.

$(ICON_GEN): $(ICON_SOURCE)
	@echo Building icon generator...
	$(CXX) $(CXXFLAGS) $(ICON_SOURCE) /Fe$(ICON_GEN) /link gdi32.lib user32.lib
	@echo Icon generator built.

$(RESOURCE_OBJ): $(RESOURCE)
	@echo Compiling resources...
	$(RC) /fo $(RESOURCE_OBJ) $(RESOURCE)

$(TARGET): $(SOURCE) $(RESOURCE_OBJ)
	@if not exist $(RELEASE_DIR) mkdir $(RELEASE_DIR)
	@echo Building ArhintSigner Web Service...
	$(CXX) $(CXXFLAGS) $(SOURCE) $(RESOURCE_OBJ) /Fe$(TARGET) /link $(LDFLAGS)
	@echo Build complete: $(TARGET)

clean:
	@echo Cleaning...
	@if exist $(TARGET) del /F $(TARGET)
	@if exist $(RESOURCE_OBJ) del /F $(RESOURCE_OBJ)
	@if exist $(ICON_GEN) del /F $(ICON_GEN)
	@if exist *.obj del /F *.obj
	@if exist $(RELEASE_DIR) rmdir $(RELEASE_DIR)
	@if exist icon\app-icon-*.ico del /F icon\app-icon-*.ico
	@echo Clean complete.

run: $(TARGET)
	@echo Starting ArhintSigner Web Service...
	@echo.
	@echo Note: If you get a URL reservation error, run as Administrator:
	@echo   netsh http add urlacl url=http://+:8082/ user=Everyone
	@echo.
	$(TARGET)

help:
	@echo ArhintSigner Web Service - Makefile Help
	@echo ========================================
	@echo.
	@echo Available targets:
	@echo   make          - Build icons and web service executable
	@echo   make icons    - Generate icon files only
	@echo   make clean    - Remove built files
	@echo   make run      - Build and run the web service
	@echo   make help     - Show this help message
	@echo.
	@echo Usage:
	@echo   1. Build: make
	@echo   2. Run:   arhint-signer.exe [port]
	@echo   3. Test:  Open example-arhint-signer.html in your browser
	@echo.
	@echo Requirements:
	@echo   - MinGW-w64 (g++ compiler)
	@echo   - Windows SDK (for HTTP.h and httpapi.lib)
	@echo.
