# ArhintSigner

Digital signature helper for Chrome/Edge extensions to access Windows Certificate Store for signing.

## Building

### C++ Version (Recommended)

Using Visual Studio Build Tools:
```powershell
cmd /c ""C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && nmake"
```

Or compile directly:
```powershell
cmd /c ""C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && cl /EHsc /O2 /Fe:arhint-signer.exe arhint-signer.cpp crypt32.lib ncrypt.lib bcrypt.lib advapi32.lib"
```

### C# Version (Alternative)

Using .NET compiler directly:
```powershell
csc /out:arhint-signer.exe /reference:System.Web.Extensions.dll arhint-signer.cs
```

Or using the build script:
```powershell
npm install
npm run build
```

## Installation

1. **Build the executable** (if not already built) - see Building section above

2. **Run the installer**:
   - Simply double-click `install.bat`
   - OR run from command prompt:
     ```cmd
     install.bat
     ```

3. **Restart your browser** (Chrome/Edge)

That's it! The installer will automatically:
- Register the native messaging host with Chrome and Edge
- No manual registry editing required

## Uninstallation

Double-click `uninstall.bat` or run:
```cmd
uninstall.bat
```

Then you can safely delete the entire folder.

## Distribution

To distribute to users, package these files:
- `dist\arhint-signer.exe` (the main executable)
- `arhint.signer.json` (the manifest)
- `install.bat` (the installer)
- `uninstall.bat` (optional - for uninstalling)
- `README.md` (optional - instructions)

Users just need to:
1. Extract the files to any folder
2. Run `install.bat`
3. Restart their browser

No administrator rights or Node.js installation required!

## Technical Details

- Uses Windows Certificate Store (`Cert:\CurrentUser\My`)
- Filters to show only valid certificates with private keys
- Supports RSA signing with SHA-256
- Compatible with Chrome and Edge browsers
