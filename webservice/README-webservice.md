# ArhintSigner Web Service

A C++ HTTP server implementation that provides digital signature services using Windows Certificate Store.

## Overview

This project provides a simple HTTP web service that can be called directly from JavaScript in any web browser.

### Architecture

```
HTML Page → HTTP Request → C++ Web Service (HTTP.sys)
```

## Features

- ✅ **Direct HTTP API** - No browser extension required
- ✅ **CORS Support** - Works with any web page
- ✅ **Same Functionality** - List certificates and sign hashes
- ✅ **Windows Certificate Store** - Uses system certificates with private keys
- ✅ **SHA-256 Signing** - Supports both CNG and legacy CryptoAPI
- ✅ **Lightweight** - Single executable, no dependencies
- ✅ **System Tray Integration** - Hide to tray, show/hide console, exit via context menu
- ✅ **Custom Icon** - Professional certificate/key themed icon

## Requirements

- **Windows 10/11** (or Windows Server 2016+)
- **MinGW-w64** with g++ compiler
- **Windows SDK** (for HTTP API and crypto libraries)
- **Administrator privileges** (first run only, to reserve URL)

## Installation

### Easy Installation (Recommended)

1. **Download or build the installer**: `ArhintSigner-WebService-Setup.exe`
2. **Run the installer** (requires Administrator privileges)
3. **Follow the installation wizard**
4. **Choose to reserve HTTP URL** when prompted (recommended)
5. **Start the service** from Start Menu or Desktop shortcut

The installer will:
- ✅ Install the web service executable
- ✅ Create Start Menu shortcuts
- ✅ Create Desktop shortcut
- ✅ Optionally reserve the HTTP URL
- ✅ Install example HTML page and documentation

### Manual Installation

See the "Building" section below to compile from source.

## Building

### Using Visual Studio Build Tools (Recommended)

```bash
cmd /c ""C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && cl /EHsc /O2 /Fe:arhint-signer-webservice.exe arhint-signer-webservice.cpp httpapi.lib crypt32.lib ncrypt.lib ws2_32.lib advapi32.lib"
```

### Using Make

```bash
# Build the executable
make -f Makefile-webservice

# Or build and run
make -f Makefile-webservice run
```

### Using MinGW

```bash
g++ -std=c++17 -O2 -o arhint-signer-webservice.exe arhint-signer-webservice.cpp -lhttpapi -lcrypt32 -lncrypt -lws2_32
```

## Building the Installer

**Requirements:**
- NSIS (Nullsoft Scriptable Install System) - Download from https://nsis.sourceforge.io/Download
- Compiled `arhint-signer-webservice.exe`

**Build:**
```bash
# Simply run the build script
build-installer.bat

# Or manually with NSIS
makensis installer-webservice.nsi
```

This creates `ArhintSigner-WebService-Setup.exe` ready for distribution.

## Running the Service

### First Time Setup

The first time you run the service, you need to reserve the HTTP URL. Run as **Administrator**:

```bash
netsh http add urlacl url=http://+:8080/ user=Everyone
```

This only needs to be done once. It allows non-admin users to run the service on port 8082.

### Starting the Service

```bash
# Default port (8082)
arhint-signer-webservice.exe

# Custom port
arhint-signer-webservice.exe 9090
```

The service will output:
```
ArhintSigner Web Service
========================
Starting HTTP server on port 8082...
Server started successfully!
Listening on http://localhost:8082/

Endpoints:
  GET  /listCerts - List available certificates
  POST /sign      - Sign a hash

Press Ctrl+C to stop the server.
```

## API Endpoints

### GET /listCerts

Lists all valid certificates from the Windows Certificate Store that have private keys.

**Request:**
```http
GET http://localhost:8082/listCerts
```

**Response:**
```json
{
  "result": [
    {
      "label": "Issued for: John Doe | Issuer: Example CA (expires 12/31/2025)",
      "thumbprint": "A1B2C3D4E5F6...",
      "subject": "CN=John Doe, O=Example Corp",
      "issuer": "Example CA",
      "notBefore": "2024-01-01T00:00:00.000Z",
      "notAfter": "2025-12-31T23:59:59.000Z",
      "hasPrivateKey": true,
      "cert": "MIIDbTCCAlWgAwIBAgI..."
    }
  ]
}
```

### POST /sign

Signs a SHA-256 hash using the specified certificate.

**Request:**
```http
POST http://localhost:8082/sign
Content-Type: application/json

{
  "hash": "47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=",
  "thumbprint": "A1B2C3D4E5F6..."
}
```

**Response:**
```json
{
  "result": "kXJhD8Hn3uOzVq9..."
}
```

**Error Response:**
```json
{
  "error": "Certificate not found"
}
```

## Using the Demo Page

1. **Start the web service:**
   ```bash
   arhint-signer-webservice.exe
   ```

2. **Open the demo page:**
   - Simply open `example-webservice.html` in any web browser
   - Works directly with the HTTP API!

3. **Test the functionality:**
   - Click "Generate Test Hash" to create a sample hash
   - Click "Sign with Certificate" to select a certificate
   - View the signature result

## JavaScript Integration

Here's how to use the web service from your own HTML/JavaScript:

```javascript
// List certificates
async function listCertificates() {
    const response = await fetch('http://localhost:8082/listCerts');
    const data = await response.json();
    return data.result; // Array of certificates
}

// Sign a hash
async function signHash(hashBase64, thumbprint) {
    const response = await fetch('http://localhost:8082/sign', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({
            hash: hashBase64,
            thumbprint: thumbprint
        })
    });
    const data = await response.json();
    if (data.error) {
        throw new Error(data.error);
    }
    return data.result; // Base64 signature
}

// Example usage
async function example() {
    try {
        // Get certificates
        const certs = await listCertificates();
        console.log('Found certificates:', certs);

        // Generate test hash
        const encoder = new TextEncoder();
        const data = encoder.encode('Test message');
        const hashBuffer = await crypto.subtle.digest('SHA-256', data);
        const hashArray = Array.from(new Uint8Array(hashBuffer));
        const hashBase64 = btoa(String.fromCharCode(...hashArray));

        // Sign with first certificate
        if (certs.length > 0) {
            const signature = await signHash(hashBase64, certs[0].thumbprint);
            console.log('Signature:', signature);
        }
    } catch (error) {
        console.error('Error:', error);
    }
}
```

## Security Considerations

⚠️ **Important Security Notes:**

1. **Local Development Only** - This service is designed for local development and testing. The default CORS policy (`Access-Control-Allow-Origin: *`) allows any website to call the service.

2. **Network Binding** - The service only binds to `localhost` by default, making it inaccessible from other machines.

3. **No Authentication** - There's no authentication mechanism. Any local application can request signatures.

4. **Production Use** - For production:
   - Implement proper authentication (API keys, tokens, etc.)
   - Restrict CORS to specific origins
   - Add request logging and rate limiting
   - Consider HTTPS (requires certificate configuration)
   - Run as a Windows Service with proper permissions

## Troubleshooting

### Error: "HttpAddUrlToUrlGroup failed"

**Solution:** Reserve the URL as Administrator:
```bash
netsh http add urlacl url=http://+:8082/ user=Everyone
```

### Error: "Service is not running"

**Check:**
1. Is the executable running?
2. Is it listening on the correct port?
3. Is your firewall blocking port 8082?

### Error: "No certificates found"

**Check:**
1. Do you have certificates in Windows Certificate Store (User → Personal)?
2. Do the certificates have private keys?
3. Are the certificates currently valid (not expired)?

### Build Errors

**Missing HTTP.h:**
- Install Windows SDK
- Make sure Windows SDK path is in your include path

**Linker errors:**
- Ensure you're linking: `httpapi.lib`, `crypt32.lib`, `ncrypt.lib`, `ws2_32.lib`

## Files

- `arhint-signer-webservice.cpp` - Main C++ web service source code
- `arhint-signer-webservice.exe` - Compiled executable
- `example-webservice.html` - Demo HTML page with JavaScript client
- `Makefile-webservice` - Build file for make
- `installer-webservice.nsi` - NSIS installer script
- `build-installer.bat` - Script to build the installer
- `README-webservice.md` - This file

## Advanced Configuration

### Custom Port

```bash
arhint-signer-webservice.exe 9090
```

Then update the JavaScript:
```javascript
const SERVICE_URL = 'http://localhost:9090';
```

### Multiple Instances

Run on different ports for isolation:
```bash
# Terminal 1
arhint-signer-webservice.exe 8081

# Terminal 2
arhint-signer-webservice.exe 8082
```

### Running as Windows Service

Use tools like `nssm` (Non-Sucking Service Manager):
```bash
nssm install ArhintSigner "C:\path\to\arhint-signer-webservice.exe"
nssm start ArhintSigner
```

## License

See [LICENSE.txt](LICENSE.txt)

## Support

For issues, questions, or contributions, please refer to the main project documentation.
