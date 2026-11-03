# ArhintSigner Web Service - Installation Guide

## Quick Install

1. **Run the installer**: `ArhintSigner-Setup.exe`
2. **Follow the installation wizard**
3. **Click "Start ArhintSigner Web Service"** on the final screen

## What Gets Installed

- **Program Files**: `C:\Program Files\ArhintSignerWS\`
  - Web service executable
  - Example HTML page
  - Documentation

- **Start Menu**: "ArhintSigner Web Service"
  - Start Web Service
  - Open Example Page
  - Installation Folder
  - README
  - Uninstall

- **Desktop**: Shortcut to start the service

- **Auto-start (Optional)**: Configure the service to start automatically when Windows starts

## First Time Setup

### HTTP URL reservation (not required)

The service listens on `http://localhost:8082/`, which a normal user may register without any reservation or
administrator rights. Earlier builds listened on `http://+:8082/` and therefore needed the reservation
(made by older installers or with `netsh http add urlacl url=http://+:8082/ user=Everyone` as Administrator).
The current installer does not make one and removes a leftover one, because while such a reservation existed the
service did not answer on `localhost` in our CI. If the service starts but does not respond, run
`netsh http delete urlacl url=http://+:8082/` as Administrator. See "Listening Address" in README.md.

Open the service as `http://localhost:8082/` (not `127.0.0.1`).

### Auto-start Configuration (Optional)

During installation, you can choose to have the service start automatically when Windows boots:

- Check the box "Auto-start on Windows Startup" during installation
- The service will launch in the system tray on every Windows startup
- You can disable this later by removing the registry entry from:
  `HKLM\Software\Microsoft\Windows\CurrentVersion\Run\ArhintSignerWS`

## Using the Service

### Start the Service

- **From Start Menu**: ArhintSigner Web Service → Start Web Service
- **From Desktop**: Double-click "ArhintSigner Web Service" shortcut
- **Manually**: Run `arhint-signer.exe` from installation folder (or `release\arhint-signer.exe` if building from source)
- **Auto-start**: If enabled during installation, starts automatically with Windows

The service will run on: `http://localhost:8082`

### View Service Information

Open a web browser and navigate to `http://localhost:8082` to see:
- Service status and version
- Available API endpoints
- API documentation
- Security information

### Test the Service

- **From Start Menu**: ArhintSigner Web Service → Open Example Page
- **Or manually**: Open `example-arhint-signer.html` in any browser

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

**Problem**: the port is used by another program, or the executable is an older build that listens on
`http://+:8082/` and needs a URL reservation.

**Solution**:
1. Close any other running ArhintSigner instance, or start the service on another port.
2. Install the current version and remove a leftover reservation with
   `netsh http delete urlacl url=http://+:8082/` (as Administrator). Only an old build that listens on
   `http://+:8082/` needs the reservation (`netsh http add urlacl url=http://+:8082/ user=Everyone`).

### "Service is not running" in Example Page

**Problem**: The service executable is not running

**Solution**: Start the service from Start Menu or Desktop shortcut

### Port 8082 Already in Use

**Problem**: Another application is using port 8082

**Solution**: Run the service with a custom port:
```bash
arhint-signer.exe 9090
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
- Remove the HTTP URL reservation (if one was made)
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

For detailed documentation, see: `README.md` in the installation folder

For issues or questions, refer to the main project repository.

## License

See LICENSE.txt included with the installer.

---

**Installation Path**: `C:\Program Files\ArhintSignerWS\`  
**Service URL**: `http://localhost:8082`  
**Port**: 8082 (default, customizable)
