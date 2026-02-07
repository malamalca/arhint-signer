# Security Hardening Documentation

This document describes the security measures implemented in the arhint-signer-webservice to protect against common vulnerabilities.

## Implemented Security Measures

### 1. **Buffer Overflow Protection**

#### Thumbprint Conversion (certificate_manager.h)
- **Issue**: Fixed-size buffer (20 bytes) could overflow with malformed input
- **Fix**: Strict validation of thumbprint length (must be exactly 40 hex characters)
- **Validation**: Each hex character conversion is checked for success before proceeding

```cpp
// Validation before conversion
if (thumbprint.length() != 40) {
    throw std::runtime_error("Invalid thumbprint length");
}

// Checked conversion
for (size_t i = 0; i < 20; i++) {
    int result = sscanf_s(thumbprint.c_str() + (i * 2), "%2hhx", &thumbprintBytes[i]);
    if (result != 1) {
        throw std::runtime_error("Invalid thumbprint format");
    }
}
```

#### HTTP Request Body (http_utils.h)
- **Issue**: Unbounded memory allocation based on request size
- **Fix**: Maximum request body size enforced (10KB)
- **Validation**: Buffer bounds checked after read operation

```cpp
const ULONG MAX_REQUEST_SIZE = 10240; // 10KB max
if (bytesRead > 0 && bytesRead <= buffer.size() && bytesRead <= MAX_REQUEST_SIZE) {
    requestBody = std::string(buffer.data(), bytesRead);
}
```

### 2. **Denial of Service (DoS) Protection**

#### Request Body Size Limit (request_handler.h)
- **Issue**: Attackers could send extremely large requests to consume memory
- **Fix**: 10KB maximum request body size
- **Response**: HTTP 413 (Request Entity Too Large) for oversized requests

```cpp
if (requestBody.length() > 10240) {
    return HTTP 413 error;
}
```

#### JSON Parser DoS (json_utils.h)
- **Issue**: Regex catastrophic backtracking with malicious input
- **Fix**: Input size limit before regex processing (10KB)
- **Mitigation**: Prevents regex engine from processing unbounded input

```cpp
if (json.length() > 10240) {
    throw std::runtime_error("JSON input too large");
}
```

#### Base64 Decode DoS (crypto_utils.h)
- **Issue**: Malicious input could cause excessive memory allocation
- **Fix**: Input size limit (1MB) and decoded size validation
- **Protection**: Prevents memory exhaustion attacks

```cpp
if (base64.length() > 1048576) {
    return {}; // Empty vector
}
if (dataLength > 1048576) {
    return {}; // Sanity check on decoded size
}
```

### 3. **Input Validation**

#### Hash Parameter (request_handler.h)
- **Validation**: Non-empty and maximum 1024 characters
- **Purpose**: Prevent injection attacks and excessive processing
- **Response**: HTTP 400 for invalid input

```cpp
if (hash.empty() || hash.length() > 1024) {
    return HTTP 400 error;
}
```

#### Thumbprint Parameter (request_handler.h)
- **Validation**: 
  - Exactly 40 characters (SHA-1 hash in hex)
  - Only hexadecimal characters (0-9, a-f, A-F)
- **Purpose**: Prevent injection and ensure valid certificate lookup
- **Response**: HTTP 400 for invalid format

```cpp
// Length check
if (thumbprint.length() != 40) {
    return HTTP 400 error;
}

// Character validation
for (char c : thumbprint) {
    if (!isHexDigit(c)) {
        return HTTP 400 error;
    }
}
```

### 4. **JSON Injection Protection**

#### Escape Sequences (json_utils.h)
- **Protection**: All JSON strings are properly escaped
- **Characters**: Handles quotes, backslashes, control characters
- **Unicode**: Control characters converted to \uXXXX format

```cpp
case '"': o << "\\\""; break;
case '\\': o << "\\\\"; break;
case '\n': o << "\\n"; break;
// ... more escape sequences
```

### 5. **Error Handling**

- All cryptographic operations wrapped in try-catch blocks
- Resources properly released in all error paths
- Detailed error messages logged to server console
- Smart error detection: Returns HTTP 400 for validation errors, HTTP 500 for server errors
- Specific error messages help debugging without exposing system internals

#### Error Response Codes
- **400 Bad Request**: Invalid user input (malformed hash, invalid thumbprint, wrong hash length)
- **413 Request Entity Too Large**: Request body exceeds 10KB
- **500 Internal Server Error**: Server-side errors (certificate store access, cryptographic failures)

## Attack Scenarios Prevented

### 1. Buffer Overflow Attack
**Scenario**: Attacker sends thumbprint longer than 40 characters
```json
POST /sign
{"thumbprint": "AAAA...AAAA" (>40 chars), "hash": "..."}
```
**Result**: HTTP 400 - "Invalid thumbprint (must be 40 hex characters)"

### 2. DoS via Large Request
**Scenario**: Attacker sends 100MB request body
```json
POST /sign
{"hash": "AAAA...AAAA" (100MB), "thumbprint": "..."}
```
**Result**: HTTP 413 - "Request body too large (max 10KB)"

### 3. Regex DoS
**Scenario**: Attacker sends crafted JSON to trigger catastrophic backtracking
```json
POST /sign
{"key": "value\"\"\"\"\"\"\"\"\"\"..." (repeated patterns)}
```
**Result**: Rejected before regex processing - "JSON input too large"

### 4. Non-Hex Thumbprint
**Scenario**: Attacker tries SQL injection or path traversal in thumbprint
```json
POST /sign
{"thumbprint": "../../etc/passwd; DROP TABLE users;", "hash": "..."}
```
**Result**: HTTP 400 - "Invalid thumbprint (must contain only hex characters)"

### 5. Memory Exhaustion via Base64
**Scenario**: Attacker sends extremely large base64 hash
```json
POST /sign
{"thumbprint": "...", "hash": "AAAA...AAAA" (10MB base64)}
```
**Result**: Base64 decode returns empty vector, signature fails gracefully

### 6. Invalid Hash Data
**Scenario**: Attacker sends invalid or corrupted hash
```json
POST /sign
{"thumbprint": "9D6E68FEA1B30D0F4800D10F416C04D84FD4E6AC", "hash": "cfsdfsd"}
```
**Result**: HTTP 400 - "Invalid hash length: X bytes. Expected 20 (SHA-1), 32 (SHA-256), or 64 (SHA-512) bytes"

## Security Best Practices

### Input Validation
✅ All user inputs validated before processing
✅ Whitelist approach (only hex for thumbprint)
✅ Size limits on all variable-length inputs

### Memory Safety
✅ No fixed-size buffers without bounds checking
✅ All allocations size-limited
✅ Bounds checked after read operations

### Resource Management
✅ RAII patterns for automatic cleanup
✅ All cryptographic handles properly released
✅ Exception-safe resource management

### Error Handling
✅ No sensitive information in error messages
✅ All exceptions caught and handled
✅ Detailed logging for debugging (server-side only)

### Defense in Depth
✅ Multiple validation layers
✅ Size limits at HTTP, JSON, and crypto layers
✅ Early rejection of invalid input

## Recommended Additional Measures

### 1. Rate Limiting
Consider implementing rate limiting to prevent brute force attacks:
- Limit requests per IP address
- Throttle failed authentication attempts
- Implement exponential backoff

### 2. Authentication
Current implementation assumes local-only access. For remote access:
- Implement API key authentication
- Use HTTPS/TLS for transport security
- Consider mutual TLS for client authentication

### 3. Audit Logging
Enhanced logging for security events:
- Log all failed validation attempts
- Track certificate usage patterns
- Monitor for suspicious activity

### 4. Monitoring
- Alert on repeated validation failures
- Monitor memory usage for DoS detection
- Track response times for anomaly detection

## Testing Security Measures

### Test Cases
1. **Oversized Request**: Send 20KB JSON → Expect HTTP 413
2. **Invalid Thumbprint Length**: Send 39-char thumbprint → Expect HTTP 400
3. **Non-Hex Thumbprint**: Send "ZZZZ..." thumbprint → Expect HTTP 400
4. **Oversized Hash**: Send 2KB hash → Expect HTTP 400
5. **Malformed JSON**: Send invalid JSON → Expect HTTP 400 or 500
6. **Missing Parameters**: Omit hash or thumbprint → Expect HTTP 400
7. **Invalid Hash Data**: Send corrupted base64 → Expect HTTP 400
8. **Wrong Hash Length**: Send 10-byte hash → Expect HTTP 400

### Example Test (PowerShell)
```powershell
# Test oversized request
$body = @{
    thumbprint = "A" * 10000
    hash = "test"
} | ConvertTo-Json

Invoke-WebRequest -Uri "http://localhost:8082/sign" -Method POST -Body $body
# Expected: 413 Request Entity Too Large (caught at request body read)

# Test invalid thumbprint
$body = @{
    thumbprint = "INVALID_NOT_HEX!"
    hash = "dGVzdA=="
} | ConvertTo-Json

Invoke-WebRequest -Uri "http://localhost:8082/sign" -Method POST -Body $body
# Expected: 400 Bad Request - "Invalid thumbprint (must contain only hex characters)"

# Test invalid hash
$body = @{
    thumbprint = "9D6E68FEA1B30D0F4800D10F416C04D84FD4E6AC"
    hash = "InvalidBase64!"
} | ConvertTo-Json

Invoke-WebRequest -Uri "http://localhost:8082/sign" -Method POST -Body $body
# Expected: 400 Bad Request - "Invalid hash length: X bytes..."
```

## Compliance Notes

- **OWASP Top 10**: Addresses A03:2021 – Injection, A04:2021 – Insecure Design
- **CWE-120**: Buffer Copy without Checking Size of Input (MITIGATED)
- **CWE-400**: Uncontrolled Resource Consumption (MITIGATED)
- **CWE-625**: Permissive Regular Expression (MITIGATED)

## Version History

- **v1.1** (2025-12-29): Added comprehensive input validation and DoS protection
- **v1.0** (2025-12-28): Initial release with basic security measures
