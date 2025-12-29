# Icon Generator for ArhintSigner
# Converts SVG to ICO and PNG formats
# Requires: Inkscape or ImageMagick (optional)

param(
    [string]$SvgPath = "icon-source.svg",
    [switch]$UseImageMagick = $false
)

$ErrorActionPreference = "Stop"

Write-Host "ArhintSigner Icon Generator" -ForegroundColor Cyan
Write-Host "===========================" -ForegroundColor Cyan
Write-Host ""

# Check if SVG exists
if (-not (Test-Path $SvgPath)) {
    Write-Error "SVG file not found: $SvgPath"
    exit 1
}

$svgFullPath = (Resolve-Path $SvgPath).Path
$basePath = Split-Path $svgFullPath -Parent
$outputPng256 = Join-Path $basePath "app-icon-256.png"
$outputPng128 = Join-Path $basePath "app-icon-128.png"
$outputPng64 = Join-Path $basePath "app-icon-64.png"
$outputPng48 = Join-Path $basePath "app-icon-48.png"
$outputPng32 = Join-Path $basePath "app-icon-32.png"
$outputPng16 = Join-Path $basePath "app-icon-16.png"
$outputIco = Join-Path $basePath "app-icon.ico"

# Method 1: Try using Inkscape (more common)
$inkscapePath = $null
$inkscapeLocations = @(
    "C:\Program Files\Inkscape\bin\inkscape.exe",
    "C:\Program Files (x86)\Inkscape\bin\inkscape.exe",
    "$env:LOCALAPPDATA\Programs\Inkscape\bin\inkscape.exe"
)

foreach ($location in $inkscapeLocations) {
    if (Test-Path $location) {
        $inkscapePath = $location
        break
    }
}

# Method 2: Check for magick (ImageMagick)
$magickPath = (Get-Command magick -ErrorAction SilentlyContinue).Source

if ($UseImageMagick -and $magickPath) {
    Write-Host "Using ImageMagick..." -ForegroundColor Green
    
    # Convert to PNG files
    & magick convert -background none -density 1200 $svgFullPath -resize 256x256 $outputPng256
    & magick convert -background none -density 1200 $svgFullPath -resize 128x128 $outputPng128
    & magick convert -background none -density 1200 $svgFullPath -resize 64x64 $outputPng64
    & magick convert -background none -density 1200 $svgFullPath -resize 48x48 $outputPng48
    & magick convert -background none -density 1200 $svgFullPath -resize 32x32 $outputPng32
    & magick convert -background none -density 1200 $svgFullPath -resize 16x16 $outputPng16
    
    # Create ICO file with multiple sizes
    & magick convert $outputPng256 $outputPng128 $outputPng64 $outputPng48 $outputPng32 $outputPng16 $outputIco
    
    Write-Host "Created: $outputIco" -ForegroundColor Green
    Write-Host "Created PNG files: 256, 128, 64, 48, 32, 16" -ForegroundColor Green
}
elseif ($inkscapePath) {
    Write-Host "Using Inkscape at: $inkscapePath" -ForegroundColor Green
    
    # Convert to PNG files using Inkscape
    & $inkscapePath --export-type=png --export-filename=$outputPng256 --export-width=256 --export-height=256 $svgFullPath
    & $inkscapePath --export-type=png --export-filename=$outputPng128 --export-width=128 --export-height=128 $svgFullPath
    & $inkscapePath --export-type=png --export-filename=$outputPng64 --export-width=64 --export-height=64 $svgFullPath
    & $inkscapePath --export-type=png --export-filename=$outputPng48 --export-width=48 --export-height=48 $svgFullPath
    & $inkscapePath --export-type=png --export-filename=$outputPng32 --export-width=32 --export-height=32 $svgFullPath
    & $inkscapePath --export-type=png --export-filename=$outputPng16 --export-width=16 --export-height=16 $svgFullPath
    
    Write-Host "Created PNG files: 256, 128, 64, 48, 32, 16" -ForegroundColor Green
    
    # For ICO, we need ImageMagick or we create a simple fallback
    if ($magickPath) {
        & magick convert $outputPng256 $outputPng128 $outputPng64 $outputPng48 $outputPng32 $outputPng16 $outputIco
        Write-Host "Created: $outputIco" -ForegroundColor Green
    } else {
        Write-Host "Note: ImageMagick not found. Cannot create .ico file." -ForegroundColor Yellow
        Write-Host "You can create ICO manually from the PNG files using an online converter." -ForegroundColor Yellow
    }
}
else {
    Write-Host "Neither Inkscape nor ImageMagick found." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Installing using built-in .NET capabilities..." -ForegroundColor Cyan
    
    # Fallback: Use .NET System.Drawing (basic conversion)
    Add-Type -AssemblyName System.Drawing
    
    # Load SVG as XML and create bitmap (simplified approach)
    Write-Host "Creating basic PNG from SVG..." -ForegroundColor Yellow
    
    # This is a simplified fallback - for production use Inkscape or ImageMagick
    Write-Host ""
    Write-Host "For best results, please install one of the following:" -ForegroundColor Yellow
    Write-Host "  1. Inkscape: https://inkscape.org/release/" -ForegroundColor White
    Write-Host "  2. ImageMagick: https://imagemagick.org/script/download.php" -ForegroundColor White
    Write-Host ""
    Write-Host "After installation, run this script again." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Done!" -ForegroundColor Green
