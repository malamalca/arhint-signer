# ArhintSigner Web Service - Code Architecture

This document describes the modular architecture of the ArhintSigner web service.

## Overview

The codebase has been refactored from a single monolithic file (~840 lines) into a well-organized modular structure with clear separation of concerns.

## Architecture

```
webservice/
├── arhint-signer-webservice.cpp    (Main entry point - ~100 lines)
└── include/
    ├── http_server.h               (HTTP server management)
    ├── request_handler.h           (Request routing & endpoints)
    ├── certificate_manager.h       (Certificate operations)
    ├── http_utils.h                (HTTP utilities)
    ├── json_utils.h                (JSON serialization)
    ├── crypto_utils.h              (Cryptography utilities)
    ├── string_utils.h              (String manipulation)
    └── system_tray.h               (System tray icon management)
```

## Module Responsibilities

### 1. **arhint-signer-webservice.cpp** (Main Entry Point)
- Minimal main file (~47 lines)
- Command-line argument parsing
- Server initialization and startup
- Clean, easy to understand program flow

### 2. **include/http_server.h** (Server Management)
**Namespace:** `ArhintSigner::Server`

**Class:** `HttpServer`
- Server lifecycle management (initialize, shutdown)
- HTTP.sys API configuration
- URL registration and binding
- Request processing loop
- RAII-compliant resource management

**Key Methods:**
- `initialize()` - Set up HTTP server
- `processRequests()` - Main request loop (template method)
- `shutdown()` - Clean resource cleanup

### 3. **include/request_handler.h** (Request Routing)
**Namespace:** `ArhintSigner::RequestHandler`

**Functions:**
- `handleRequest()` - Main request dispatcher
- Routes to appropriate endpoint handlers
- Handles CORS preflight requests
- Error handling and exception management

**Endpoints:**
- `GET /listCerts` - List available certificates
- `POST /sign` - Sign a hash with a certificate
- `OPTIONS *` - CORS preflight

### 4. **include/certificate_manager.h** (Certificate Operations)
**Namespace:** `ArhintSigner::Certificate`

**Functions:**
- `listCertificates()` - Enumerate certificates from Windows store
- `signHash()` - Sign a hash using certificate private key
- `getCertNameString()` - Extract certificate name information

**Features:**
- Supports both CNG and legacy CryptoAPI
- Automatic key type detection
- Certificate validation (expiration, private key availability)
- Proper resource cleanup with RAII principles

### 5. **include/http_utils.h** (HTTP Utilities)
**Namespace:** `ArhintSigner::Http`

**Functions:**
- `sendResponse()` - Send HTTP response with CORS headers
- `readRequestBody()` - Read POST request body

**Features:**
- CORS header management
- Automatic content-type handling
- Status code mapping
- Body chunk reading

### 6. **include/json_utils.h** (JSON Utilities)
**Namespace:** `ArhintSigner::Json`

**Class:** `Builder`
- Simple JSON object builder
- Type-safe methods (addString, addBool, addArray, addObject)
- Automatic JSON escaping

**Functions:**
- `parse()` - Basic JSON parsing for request parameters

### 7. **include/crypto_utils.h** (Cryptography Utilities)
**Namespace:** `ArhintSigner::Crypto`

**Functions:**
- `base64Encode()` - Encode binary data to base64
- `base64Decode()` - Decode base64 to binary data

Uses Windows CryptoAPI for encoding/decoding.

### 8. **include/string_utils.h** (String Utilities)
**Namespace:** `ArhintSigner::Utils`

**Functions:**
- `extractDNField()` - Parse DN (Distinguished Name) fields
- `fileTimeToISO()` - Convert FILETIME to ISO 8601
- `fileTimeToShortDate()` - Convert FILETIME to short format
- `trim()` - Remove whitespace from strings

### 9. **include/system_tray.h** (System Tray Management)
**Namespace:** `ArhintSigner::SystemTray`

**Class:** `TrayIcon`
- System tray icon with context menu
- Show/hide console window
- Exit application from tray
- Balloon notifications
- Message processing loop

**Key Methods:**
- `initialize()` - Create tray icon with tooltip and exit callback
- `processMessages()` - Handle Windows messages for tray icon
- `showBalloon()` - Display notification balloon
- `updateTooltip()` - Update icon tooltip text
- `cleanup()` - Remove tray icon

**Features:**
- Right-click context menu with Show/Hide/Exit options
- Double-click to toggle console visibility
- Integrates with main message loop
- Graceful shutdown on exit

## Design Principles

### 1. **Separation of Concerns**
Each module has a single, well-defined responsibility:
- HTTP server management is isolated from business logic
- Certificate operations are independent of HTTP handling
- Utilities are reusable across modules

### 2. **Namespace Organization**
All code is under `ArhintSigner` namespace with logical sub-namespaces:
```cpp
ArhintSigner::
├── Server::         (HTTP server)
├── RequestHandler:: (Request routing)
├── Certificate::    (Certificate operations)
├── Http::           (HTTP utilities)
├── Json::           (JSON handling)
├── Crypto::         (Cryptography)
└── Utils::          (General utilities)
```

### 3. **Header-Only Design**
Most modules are header-only with inline functions:
- **Benefits:**
  - No separate compilation needed for utilities
  - Easier to maintain
  - Compiler can better optimize
  - Template-friendly

### 4. **RAII and Resource Management**
- `HttpServer` class manages all HTTP.sys resources
- Automatic cleanup in destructor
- Exception-safe resource handling in certificate operations

### 5. **Type Safety**
- Strong typing with C++ classes
- Namespace isolation prevents naming conflicts
- Template methods for generic request handling

### 6. **Error Handling**
- Consistent exception-based error handling
- Clear error messages with context
- Proper resource cleanup on errors

## Benefits of This Architecture

### 1. **Maintainability**
- Small, focused files (largest is ~400 lines)
- Clear module boundaries
- Easy to locate and fix bugs
- Self-documenting structure

### 2. **Testability**
- Each module can be tested independently
- Mock HTTP requests for handler testing
- Isolated certificate operations

### 3. **Extensibility**
- Easy to add new endpoints in `request_handler.h`
- New utilities can be added to respective modules
- Server configuration can be extended without affecting business logic

### 4. **Readability**
- Main file is only ~47 lines
- Clear program flow from entry to exit
- Well-named namespaces and functions
- Comprehensive documentation

### 5. **Performance**
- Header-only design enables aggressive inlining
- No runtime overhead from modularization
- Template-based request handler allows specialization

## Compilation

All modules are included via headers. Single compilation unit:

```bash
cl /std:c++17 /EHsc /O2 /W3 arhint-signer-webservice.cpp 
   /Fearhint-signer-webservice.exe 
   /link httpapi.lib crypt32.lib ncrypt.lib ws2_32.lib advapi32.lib
```

## Future Improvements

Potential enhancements while maintaining the architecture:

1. **Configuration Module**
   - Add `include/config.h` for server settings
   - Support for configuration files
   - Environment variable parsing

3. **Logging Module**
   - Add `include/logger.h` for structured logging
   - Log levels (debug, info, warn, error)
   - File and console output

4. **Authentication**
   - Add `include/auth.h` for API key validation
   - Token-based authentication

4. **Certificate Filtering**
   - Enhanced certificate search
   - Custom certificate stores
   - Certificate validation policies

5. **Response Caching**
   - Cache certificate lists
   - Configurable cache expiration

## Migration from Old Code

The old monolithic file has been backed up as `arhint-signer-webservice-old.cpp`.

**Changes:**
- All functionality preserved
- Same API endpoints and behavior
- Improved code organization
- Better error handling
- Easier to maintain and extend

**No breaking changes** - The service operates identically to the previous version.
