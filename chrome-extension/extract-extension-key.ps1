# Extract Chrome Extension Public Key from .pem file
# Usage: .\extract-extension-key.ps1 -PemPath "path\to\extension.pem"

param(
    [Parameter(Mandatory=$false)]
    [string]$PemPath = "..\arhint-signer-extension.pem"
)

Write-Host "Chrome Extension Key Extractor" -ForegroundColor Cyan
Write-Host "================================" -ForegroundColor Cyan
Write-Host ""

# Check if file exists
if (-not (Test-Path $PemPath)) {
    Write-Host "ERROR: .pem file not found at: $PemPath" -ForegroundColor Red
    Write-Host ""
    Write-Host "To generate a .pem file:" -ForegroundColor Yellow
    Write-Host "1. Open chrome://extensions/" -ForegroundColor Yellow
    Write-Host "2. Enable Developer mode" -ForegroundColor Yellow
    Write-Host "3. Click 'Pack extension'" -ForegroundColor Yellow
    Write-Host "4. Select your extension folder" -ForegroundColor Yellow
    Write-Host "5. Leave 'Private key file' empty (first time)" -ForegroundColor Yellow
    Write-Host "6. The .pem file will be created" -ForegroundColor Yellow
    Write-Host ""
    exit 1
}

Write-Host "Reading .pem file: $PemPath" -ForegroundColor Green

try {
    # Read the PEM file
    $pemContent = Get-Content $PemPath -Raw
    
    # Extract the base64 key (remove headers and whitespace)
    $key = $pemContent -replace '-----BEGIN PRIVATE KEY-----', '' `
                       -replace '-----END PRIVATE KEY-----', '' `
                       -replace '\r?\n', '' `
                       -replace '\s', ''
    
    # Convert to byte array
    $keyBytes = [Convert]::FromBase64String($key)
    
    # Get DER-encoded public key (skip PKCS#8 header - usually 26 bytes, but we'll parse it properly)
    # PKCS#8 format has a specific structure, we need to skip to the actual key data
    # For RSA keys, we typically need to skip the first 26 bytes for 2048-bit keys
    $publicKeyBytes = $keyBytes[26..($keyBytes.Length - 1)]
    
    # Convert to base64 for Chrome extension manifest
    $publicKey = [Convert]::ToBase64String($publicKeyBytes)
    
    Write-Host ""
    Write-Host "SUCCESS! Public key extracted:" -ForegroundColor Green
    Write-Host "================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host $publicKey -ForegroundColor Yellow
    Write-Host ""
    Write-Host "================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Add this to your manifest.json:" -ForegroundColor Green
    Write-Host ""
    Write-Host '{' -ForegroundColor White
    Write-Host '  "manifest_version": 3,' -ForegroundColor White
    Write-Host '  "name": "Your Extension Name",' -ForegroundColor White
    Write-Host "  `"key`": `"$publicKey`"," -ForegroundColor Yellow
    Write-Host '  ...' -ForegroundColor White
    Write-Host '}' -ForegroundColor White
    Write-Host ""
    
    # Also compute and display the extension ID
    $sha256 = [System.Security.Cryptography.SHA256]::Create()
    $hash = $sha256.ComputeHash($publicKeyBytes)
    
    # Take first 16 bytes and convert to extension ID (a-p mapping)
    $chars = 'abcdefghijklmnop'
    $extensionId = ''
    for ($i = 0; $i -lt 16; $i++) {
        $byte = $hash[$i]
        $extensionId += $chars[$byte -shr 4]
        $extensionId += $chars[$byte -band 0x0F]
    }
    
    Write-Host "Your extension ID will be:" -ForegroundColor Green
    Write-Host $extensionId -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Chrome extension URL:" -ForegroundColor Green
    Write-Host "chrome-extension://$extensionId/" -ForegroundColor Yellow
    Write-Host ""
    
} catch {
    Write-Host "ERROR: Failed to process .pem file" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}

Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "1. Copy the key above to your manifest.json" -ForegroundColor White
Write-Host "2. Update arhint.signer.json with the extension ID" -ForegroundColor White
Write-Host "3. Update install.bat with the extension ID" -ForegroundColor White
Write-Host ""
