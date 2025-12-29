# ArhintSigner

Digital signature solutions for Windows Certificate Store - providing multiple implementation approaches.

## 🚀 Quick Start

Choose the implementation that best fits your needs:

### Option 1: Web Service (Recommended for modern apps)
**Direct HTTP API - No browser extension required**

```bash
cd webservice
arhint-signer-webservice.exe
```

Then open `example-webservice.html` in any browser. Works with Chrome, Firefox, Edge, etc.

### Option 2: Chrome Extension (For Chrome-specific integration)
**Native messaging host for Chrome/Edge extensions**

```bash
cd chrome-extension
# Build and install - see chrome-extension/README.md
```

## 📁 Project Structure

```
arhint-signer/
├── webservice/              # HTTP Web Service implementation
│   ├── arhint-signer-webservice.cpp
│   ├── arhint-signer-webservice.exe
│   ├── example-webservice.html
│   ├── Makefile-webservice
│   └── README-webservice.md
│
├── chrome-extension/        # Chrome Extension + Native Messaging
│   ├── arhint-signer.cpp    (C++ native host)
│   ├── arhint-signer.cs     (C# alternative)
│   ├── arhint-signer.js     (Node.js alternative)
│   ├── example.html
│   ├── installer.nsi
│   ├── Makefile
│   └── README.md
│
└── LICENSE.txt
```

## 🎯 Which One to Use?

### Use **Web Service** if you want:
- ✅ Browser-independent solution (works in any browser)
- ✅ Simpler architecture (no extension needed)
- ✅ Direct JavaScript API calls via fetch/axios
- ✅ Easier deployment and updates
- ✅ Modern web application integration

### Use **Chrome Extension** if you need:
- ✅ Deep Chrome/Edge extension integration
- ✅ Browser extension ecosystem features
- ✅ Extension permissions and sandboxing
- ✅ Chrome Web Store distribution

## 📖 Documentation

- **Web Service**: See [webservice/README-webservice.md](webservice/README-webservice.md)
- **Chrome Extension**: See [chrome-extension/README.md](chrome-extension/README.md)

## 🔧 Features (Both Implementations)

- List certificates from Windows Certificate Store (`Cert:\CurrentUser\My`)
- Filter valid certificates with private keys
- Sign SHA-256 hashes using RSA certificates
- Support for both CNG and legacy CryptoAPI
- User-friendly certificate selection interface

## 🛠️ Building

### Web Service
```bash
cd webservice
# Using Visual Studio Build Tools:
cmd /c ""C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && cl /EHsc /O2 /Fe:arhint-signer-webservice.exe arhint-signer-webservice.cpp httpapi.lib crypt32.lib ncrypt.lib ws2_32.lib advapi32.lib"
```

### Chrome Extension
```bash
cd chrome-extension
# Using Visual Studio Build Tools:
cmd /c ""C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && cl /EHsc /O2 /Fe:arhint-signer.exe arhint-signer.cpp crypt32.lib ncrypt.lib bcrypt.lib advapi32.lib"
```

## 📋 Requirements

- **Windows 10/11** (or Windows Server 2016+)
- **Visual Studio Build Tools 2019+** or MinGW-w64
- **Certificates** in Windows Certificate Store with private keys

## 🔐 Security Notes

Both implementations access the Windows Certificate Store and require user interaction to select certificates. The web service includes CORS headers for local development - modify for production use.

## 📄 License

See [LICENSE.txt](LICENSE.txt)

## 🤝 Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.

---

**Architecture Comparison:**

```
Chrome Extension:
HTML → Chrome Extension → Native Messaging → C++ Executable

Web Service:
HTML → HTTP Fetch API → C++ HTTP Server
```

The web service approach eliminates the Chrome extension layer, making it simpler and more universal.
