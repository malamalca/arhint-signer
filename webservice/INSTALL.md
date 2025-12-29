# ArhintSigner Web Service - Installation Guide

## Quick Install

1. **Run the installer**: `ArhintSigner-WebService-Setup.exe`
2. **Follow the installation wizard**
3. **When prompted, check "Reserve HTTP URL"** (recommended)
4. **Click "Start ArhintSigner Web Service"** on the final screen

## What Gets Installed

- **Program Files**: `C:\Program Files\ArhintSignerWS\`
  - Web service executable
  - Example HTML page
  - Documentation

- **Start Menu**: "ArhintSigner Web Service"
  - Start Web Service
  - Open Example Page
  - Reserve HTTP URL (Admin)
  - Installation Folder
  - README
  - Uninstall

- **Desktop**: Shortcut to start the service

## First Time Setup

### Reserve the HTTP URL (Important!)

The service needs permission to listen on port 8082. You have two options:

**Option 1: During Installation**
- Check the box "Reserve HTTP URL (Recommended)" on the final screen

**Option 2: After Installation**
- Go to Start Menu → ArhintSigner Web Service → "Reserve HTTP URL (Admin)"
- Right-click and select "Run as Administrator"

This only needs to be done once.

## Using the Service

### Start the Service

- **From Start Menu**: ArhintSigner Web Service → Start Web Service
- **From Desktop**: Double-click "ArhintSigner Web Service" shortcut
- **Manually**: Run `arhint-signer-webservice.exe` from installation folder

The service will run on: `http://localhost:8082`

### Test the Service

- **From Start Menu**: ArhintSigner Web Service → Open Example Page
- **Or manually**: Open `example-webservice.html` in any browser

### Stop the Service

Press `Ctrl+C` in the service window

## API Endpoints

- `GET http://localhost:8082/listCerts` - List available certificates
- `POST http://localhost:8082/sign` - Sign a hash

## Integrating with Your Application

```javascript
// List certificates
const response = await fetch('http://localhost:8082/listCerts');
const data = await response.json();
console.log(data.result); // Array of certificates

// Sign a hash
const signResponse = await fetch('http://localhost:8082/sign', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
        hash: 'base64-encoded-sha256-hash',
        thumbprint: 'certificate-thumbprint'
    })
});
const signData = await signResponse.json();
console.log(signData.result); // Base64 signature
```

## Troubleshooting

### "HttpAddUrlToUrlGroup failed" Error

**Problem**: URL not reserved

**Solution**: 
1. Go to Start Menu → ArhintSigner Web Service → "Reserve HTTP URL (Admin)"
2. Right-click and "Run as Administrator"

### "Service is not running" in Example Page

**Problem**: The service executable is not running

**Solution**: Start the service from Start Menu or Desktop shortcut

### Port 8082 Already in Use

**Problem**: Another application is using port 8082

**Solution**: Run the service with a custom port:
```bash
arhint-signer-webservice.exe 9090
```

Then update your JavaScript to use the new port:
```javascript
const SERVICE_URL = 'http://localhost:9090';
```

## Uninstall

- **From Start Menu**: ArhintSigner Web Service → Uninstall
- **From Control Panel**: Programs and Features → ArhintSigner Web Service → Uninstall

The uninstaller will:
- Remove all installed files
- Remove Start Menu and Desktop shortcuts
- Remove the HTTP URL reservation
- Clean up registry entries

## Requirements

- **Windows 10/11** (or Windows Server 2016+)
- **Certificates** in Windows Certificate Store (Personal/My) with private keys
- **Port 8082** available (or specify custom port)

## Security Notes

⚠️ **Important**: The service only listens on `localhost` by default, making it accessible only from the local machine. It includes CORS headers for local development.

For production use, consider:
- Implementing authentication
- Restricting CORS to specific origins
- Adding request logging and rate limiting
- Running with minimal privileges

## Support

For detailed documentation, see: `README-webservice.md` in the installation folder

For issues or questions, refer to the main project repository.

## License

See LICENSE.txt included with the installer.

---

**Installation Path**: `C:\Program Files\ArhintSignerWS\`  
**Service URL**: `http://localhost:8082`  
**Port**: 8082 (default, customizable)
