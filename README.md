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
└── LICENSE.txt
```

## 🎯 Features

- ✅ Browser-independent solution (works in any browser)
- ✅ Simpler architecture (no extension needed)
- ✅ Direct JavaScript API calls via fetch/axios
- ✅ Easier deployment and updates
- ✅ Modern web application integration

## 📖 Documentation

- See [webservice/README-webservice.md](webservice/README-webservice.md) for complete guide

## 🔧 Features (Both Implementations)

- List certificates from Windows Certificate Store (`Cert:\CurrentUser\My`)
- Filter valid certificates with private keys
- Sign SHA-256 hashes using RSA certificates
- Support for both CNG and legacy CryptoAPI
- User-friendly certificate selection interface

## 🛠️ Building

```bash
cd webservice
# Using Visual Studio Build Tools:
cmd /c ""C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && cl /EHsc /O2 /Fe:arhint-signer-webservice.exe arhint-signer-webservice.cpp httpapi.lib crypt32.lib ncrypt.lib ws2_32.lib advapi32.lib"
```

## 📋 Requirements

- **Windows 10/11** (or Windows Server 2016+)
- **Visual Studio Build Tools 2019+** or MinGW-w64
- **Certificates** in Windows Certificate Store with private keys

## 🔐 Security Notes

The web service accesses the Windows Certificate Store and requires user interaction to select certificates. It includes CORS headers for local development - modify for production use.

## 📄 License

See [LICENSE.txt](LICENSE.txt)

## 🤝 Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.

---

**Architecture:**

```
HTML → HTTP Fetch API → C++ HTTP Server
```

Direct HTTP communication provides a simple, browser-independent solution.
