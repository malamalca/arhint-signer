# ArhintSigner Web Service

![Build and Test](https://github.com/malamalca/arhint-signer/actions/workflows/build-and-test.yml/badge.svg)

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
- No administrator privileges are needed to run the service (see [Listening Address](#listening-address))

## Installation

### Easy Installation (Recommended)

1. **Download or build the installer**: `ArhintSigner-Setup.exe`
2. **Run the installer** (requires Administrator privileges)
3. **Follow the installation wizard**
4. **Start the service** from Start Menu or Desktop shortcut

The installer will:
- ✅ Install the web service executable
- ✅ Create Start Menu shortcuts
- ✅ Create Desktop shortcut
- ✅ Install example HTML page and documentation

### Manual Installation

See the "Building" section below to compile from source.

## Building

### Using Visual Studio Build Tools with nmake (Recommended)

```bash
# Setup build environment and build
cmd /c ""C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && nmake all"
```

Or compile manually:

```bash
cmd /c ""C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && cl /std:c++17 /EHsc /O2 /I\"src/include\" src/arhint-signer.cpp resources/app-resource.res /Fe:release/arhint-signer.exe /link /SUBSYSTEM:WINDOWS /ENTRY:WinMainCRTStartup httpapi.lib crypt32.lib ncrypt.lib ws2_32.lib advapi32.lib shell32.lib user32.lib"
```

### Using nmake (Make)

```bash
# Build the executable (outputs to release/ folder)
nmake all

# Or build and run
nmake run
```

The executable will be created at `release\arhint-signer.exe`

### Using MinGW

```bash
g++ -std=c++17 -O2 -Isrc/include -o arhint-signer.exe src/arhint-signer.cpp -lhttpapi -lcrypt32 -lncrypt -lws2_32
```

## Building the Installer

**Requirements:**
- NSIS (Nullsoft Scriptable Install System) - Download from https://nsis.sourceforge.io/Download
- Compiled `arhint-signer.exe`

**Build:**
```bash
# Simply run the build script
build-installer.bat

# Or manually with NSIS
makensis installer\installer-arhint-signer.nsi
```

This creates `ArhintSigner-Setup.exe` ready for distribution.

## Running the Service

### Listening Address

The service listens on `http://localhost:PORT/` (default port 8082). **No administrator rights and no URL
reservation (`netsh http add urlacl`) are needed.** Use `localhost` in the URL; a request to `127.0.0.1` is
answered with `400 Bad Request` because the host name does not match the registered URL.

#### Why `localhost` and not `+`

Windows HTTP.sys accepts two kinds of URL prefixes, and the choice matters:

| Prefix | Reachable from | Needs |
|--------|----------------|-------|
| `http://localhost:8082/` | this machine only | nothing (works for a normal user) |
| `http://+:8082/` | every network interface | URL reservation made as Administrator, otherwise `HttpAddUrlToUrlGroup` fails |

History:

- The first versions registered `http://localhost:PORT/`.
- Commit `fd67019` ("Actions Update", 2026-02-10, made while the GitHub Actions workflow was being set up)
  changed the prefix to `http://+:PORT/` with the comment *"use +: to match the URL reservation pattern"*. The
  installer and the CI workflow both reserve `http://+:8082/`, and the code was changed to match it. The
  commit message gives no other reason. The CI job failed with `localhost` as long as the workflow also reserved
  `http://+:8082/` before starting the service (the service started but did not answer), and it passed once the
  workflow stopped making that reservation (observed on `windows-latest`; the mechanism was not investigated).
- Side effects of that change: the service no longer started without the reservation (`HttpAddUrlToUrlGroup
  failed`, exit code 1), it became reachable from the network although it signs with the certificates of the
  logged-in user and answers every origin (`Access-Control-Allow-Origin: *`), and it contradicted the
  "Network Binding" note below. `shutdown()` also kept removing the `localhost` URL it had never registered.
- The prefix was changed back to `http://localhost:PORT/`.

Neither the installer nor the CI workflow makes a reservation any more. **A leftover reservation for
`http://+:8082/` (made by older installers) should be removed**, because in CI the service did not answer on
`localhost` while such a reservation existed. Remove it as Administrator with
`netsh http delete urlacl url=http://+:8082/`; the installer and the uninstaller do it too. If the service starts
but does not respond after an update from an older version, check `netsh http show urlacl`.

If you really need the wildcard (access from another machine or under another host name), change `urlPrefix`
in `src/include/http_server.h` to `http://+:PORT/`, reserve it once as Administrator
(`netsh http add urlacl url=http://+:8082/ user=Everyone`) and read the security notes below first.

### Starting the Service

```bash
# Default port (8082)
release\arhint-signer.exe

# Custom port
release\arhint-signer.exe 9090
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

### GET /

Displays the web service home page with service information, version, and available endpoints.

**Request:**
```http
GET http://localhost:8082/
```

**Response:**
HTML page with:
- Service name and version
- Service status
- List of available endpoints with descriptions
- API documentation
- Security information

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
   release\arhint-signer.exe
   ```

2. **Open the demo page:**
   - Simply open `examples/example-arhint-signer.html` in any web browser
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

**Possible causes:**
1. The port is already used by another program (for example error 183, already exists). Close the other instance or start the service
   on another port: `arhint-signer.exe 9090`.
2. The executable was built from a version that listens on `http://+:PORT/` (commit `fd67019` up to the
   change described in [Listening Address](#listening-address)). Rebuild from the current source. Only such an
   old build needs a reservation (the current build must run without one, see above):
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

- `src/arhint-signer.cpp` - Main C++ web service source code
- `src/include/*.h` - Header files (HTTP server, certificate manager, crypto utils, etc.)
- `release/arhint-signer.exe` - Compiled executable (built by makefile)
- `examples/example-arhint-signer.html` - Demo HTML page with JavaScript client
- `Makefile` - Build file for make
- `installer/installer-arhint-signer.nsi` - NSIS installer script
- `build-installer.bat` - Script to build the installer
- `README.md` - This file

## Advanced Configuration

### Custom Port

```bash
arhint-signer.exe 9090
```

Then update the JavaScript:
```javascript
const SERVICE_URL = 'http://localhost:9090';
```

### Multiple Instances

Run on different ports for isolation:
```bash
# Terminal 1
arhint-signer.exe 8081

# Terminal 2
arhint-signer.exe 8082
```

### Running as Windows Service

Use tools like `nssm` (Non-Sucking Service Manager):
```bash
nssm install ArhintSigner "C:\path\to\arhint-signer.exe"
nssm start ArhintSigner
```

## License

See [LICENSE.txt](LICENSE.txt)

## Support

For issues, questions, or contributions, please refer to the main project documentation.
