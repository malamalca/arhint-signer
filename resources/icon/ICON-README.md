# Icon Files

This directory contains the application icon files for ArhintSigner Web Service.

## Files

- `icon-source.svg` - SVG source file with certificate and key design (256x256)
- `app-icon-16.ico` to `app-icon-256.ico` - ICO files in various sizes (16x16 to 256x256)
- `create-icon.cpp` - C++ program that generates the icon files
- `generate-icon.ps1` - PowerShell script for SVG to ICO conversion (requires Inkscape/ImageMagick)

## Icon Design

The icon features:
- Blue background representing security
- White certificate document with gray text lines
- Blue circular seal/badge with border
- Golden key symbol (circle head with shaft and teeth)

This design visually represents the application's purpose: certificate-based digital signing.

## Regenerating Icons

### Method 1: Using the C++ Generator (Recommended)
```cmd
# From root directory
nmake icons

# Or manually compile and run
cl /std:c++17 /EHsc /O2 resources\icon\create-icon.cpp /Feresources\icon\create-icon.exe /link gdi32.lib user32.lib
cd resources\icon
create-icon.exe
```

This generates all ICO files automatically.

### Method 2: Using SVG Conversion
```powershell
.\generate-icon.ps1
```

Requires Inkscape or ImageMagick installed.

## Usage

The application uses `app-icon-32.ico` embedded as a resource in the executable for the system tray icon. The icon is compiled into the executable via `resources/app-resource.rc`.

The installer uses `app-icon-48.ico` for its icon display.

All icon files are included in the installer and deployed to the installation directory.
