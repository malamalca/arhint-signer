# Building the NSIS Installer

## Prerequisites

1. **Install NSIS** (Nullsoft Scriptable Install System)
   - Download from: https://nsis.sourceforge.io/Download
   - Or install via Chocolatey: `choco install nsis`

2. **Build the executable**
   ```powershell
   npm install
   npm run build
   ```

3. **Prepare extension files**
   - Create `extension` folder in the project root
   - Copy these files from `d:\Dev\arhint-signer-extension\` to `extension\`:
     - `manifest.json`
     - `background.js`

## Build the Installer

Right-click on `installer.nsi` and select "Compile NSIS Script"

OR run from command line:
```powershell
makensis installer.nsi
```

This creates: `ArhintSigner-Setup.exe`

## What the Installer Does

**Installation:**
- Installs `arhint-signer.exe` to Program Files
- Creates and registers the native messaging manifest
- Registers with Chrome and Edge
- Copies extension files
- Creates uninstaller and Start Menu shortcuts
- Adds entry to Windows Add/Remove Programs

**Uninstallation:**
- Removes all installed files
- Unregisters from Chrome and Edge
- Removes registry entries
- Cleans up Start Menu shortcuts
- Prompts user to manually remove Chrome extension

## Distribution

Distribute the single `ArhintSigner-Setup.exe` file.

Users:
1. Run the installer (requires admin rights)
2. Follow the prompt to load the unpacked extension in Chrome
3. Restart browser
