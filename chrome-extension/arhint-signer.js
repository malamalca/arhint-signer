const { spawnSync } = require('child_process');

function listCertificates() {
  const script = `Get-ChildItem Cert:\\CurrentUser\\My | Where-Object { $_.HasPrivateKey -and $_.NotAfter -gt (Get-Date) -and $_.NotBefore -lt (Get-Date) } | ForEach-Object { [PSCustomObject]@{ Thumbprint = $_.Thumbprint; Subject = $_.Subject; Issuer = $_.Issuer; NotBefore = $_.NotBefore.ToString('o'); NotAfter = $_.NotAfter.ToString('o'); HasPrivateKey = $_.HasPrivateKey; RawData = [Convert]::ToBase64String($_.RawData); FriendlyName = $_.FriendlyName } } | ConvertTo-Json`;
  
  const result = spawnSync('powershell.exe', [
    '-NoProfile',
    '-NonInteractive', 
    '-ExecutionPolicy', 'Bypass',
    '-Command', script
  ], { 
    encoding: 'utf8',
    maxBuffer: 10 * 1024 * 1024,
    windowsHide: true
  });
  
  if (result.error) {
    console.error('Failed to execute PowerShell:', result.error.message);
    return [];
  }
  
  if (result.stderr && result.stderr.trim()) {
    console.error('PowerShell stderr:', result.stderr);
  }
  
  if (!result.stdout || !result.stdout.trim()) {
    return [];
  }
  
  try {
    const parsed = JSON.parse(result.stdout);
    const certArray = Array.isArray(parsed) ? parsed : [parsed];
    
    return certArray.map(cert => {
      // Parse subject to extract friendly name
      const subject = cert.Subject;
      let displayName = '';
      
      // Try to extract Given Name and Surname first (for personal certs)
      const gnMatch = subject.match(/G=([^,+]+)/);
      const snMatch = subject.match(/SN=([^,+]+)/);
      
      if (gnMatch && snMatch) {
        displayName = `${gnMatch[1].trim()} ${snMatch[1].trim()}`;
      } else {
        // Otherwise use CN (Common Name)
        const cnMatch = subject.match(/CN=([^,]+)/);
        if (cnMatch) {
          displayName = cnMatch[1].trim();
        }
      }
      
      // If still no display name, fall back to first part of subject
      if (!displayName) {
        displayName = subject.split(',')[0].replace(/^CN=/, '').trim();
      }
      
      // Extract Organization (O)
      const orgMatch = subject.match(/\bO=([^,+]+)/);
      const organization = orgMatch ? orgMatch[1].trim() : '';
      
      // Add organization to display name if present
      if (organization) {
        displayName += ` (${organization})`;
      }
      
      // Extract issuer CN
      const issuer = cert.Issuer;
      const issuerCnMatch = issuer.match(/CN=([^,]+)/);
      const issuerName = issuerCnMatch ? issuerCnMatch[1].trim() : issuer.split(',')[0].trim();
      
      // Add expiry date to label for context
      const expiryDate = new Date(cert.NotAfter);
      const expiry = expiryDate.toLocaleDateString();
      
      return {
        label: `Issued for: ${displayName} | Issuer: ${issuerName} (expires ${expiry})`,
        thumbprint: cert.Thumbprint,
        subject: cert.Subject,
        issuer: cert.Issuer,
        notBefore: cert.NotBefore,
        notAfter: cert.NotAfter,
        hasPrivateKey: cert.HasPrivateKey,
        cert: cert.RawData
      };
    });
  } catch (e) {
    console.error('Failed to parse certificate data:', e.message);
    return [];
  }
}

function signHash(hashB64, thumbprint) {
  // Validate inputs
  if (!hashB64 || typeof hashB64 !== 'string') {
    throw new Error('Hash is required and must be a string');
  }
  
  if (!thumbprint || typeof thumbprint !== 'string') {
    throw new Error('Thumbprint is required and must be a string');
  }
  
  // Validate base64 format
  const base64Regex = /^[A-Za-z0-9+/]*={0,2}$/;
  if (!base64Regex.test(hashB64)) {
    throw new Error('Hash must be valid base64 encoded string');
  }
  
  const script = `$cert = Get-ChildItem Cert:\\CurrentUser\\My | Where-Object { $_.Thumbprint -eq '${thumbprint}' }; if (-not $cert) { throw 'Certificate not found' }; if (-not $cert.HasPrivateKey) { throw 'Certificate has no private key' }; $hashBytes = [Convert]::FromBase64String('${hashB64}'); $rsa = [System.Security.Cryptography.X509Certificates.RSACertificateExtensions]::GetRSAPrivateKey($cert); $signature = $rsa.SignHash($hashBytes, [System.Security.Cryptography.HashAlgorithmName]::SHA256, [System.Security.Cryptography.RSASignaturePadding]::Pkcs1); [Convert]::ToBase64String($signature)`;
  
  const result = spawnSync('powershell.exe', [
    '-NoProfile',
    '-NonInteractive',
    '-ExecutionPolicy', 'Bypass',
    '-Command', script
  ], { 
    encoding: 'utf8',
    windowsHide: true
  });
  
  if (result.error) {
    throw new Error(`Failed to execute PowerShell: ${result.error.message}`);
  }
  
  if (result.stderr && result.stderr.trim()) {
    throw new Error(`Signing failed: ${result.stderr.trim()}`);
  }
  
  return result.stdout.trim();
}

// Chrome Native Messaging protocol helper functions
function readMessage(callback) {
  let chunks = [];
  let totalLength = 0;
  let messageLength = null;

  process.stdin.on('readable', () => {
    let chunk;
    while ((chunk = process.stdin.read()) !== null) {
      chunks.push(chunk);
      totalLength += chunk.length;

      // Read message length (first 4 bytes)
      if (messageLength === null && totalLength >= 4) {
        const buffer = Buffer.concat(chunks);
        messageLength = buffer.readUInt32LE(0);
        chunks = [buffer.slice(4)];
        totalLength -= 4;
      }

      // Read complete message
      if (messageLength !== null && totalLength >= messageLength) {
        const buffer = Buffer.concat(chunks);
        const message = buffer.slice(0, messageLength).toString('utf8');
        chunks = [buffer.slice(messageLength)];
        totalLength -= messageLength;
        messageLength = null;

        callback(JSON.parse(message));
      }
    }
  });
}

function writeMessage(message) {
  const buffer = Buffer.from(JSON.stringify(message), 'utf8');
  const header = Buffer.alloc(4);
  header.writeUInt32LE(buffer.length, 0);
  process.stdout.write(header);
  process.stdout.write(buffer);
}

// Handle incoming messages
readMessage(msg => {
  console.error("Received message:", msg);

  try {
    let result;
    
    if (msg.action === "listCerts") {
      result = listCertificates();
    } else if (msg.action === "sign") {
      result = signHash(msg.hash, msg.thumbprint);
    }
    
    writeMessage({ result });
  } catch (e) {
    console.error("Error:", e.message);
    writeMessage({ error: e.message });
  }
});
