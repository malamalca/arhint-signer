using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Text.RegularExpressions;
using System.Web.Script.Serialization;

namespace ArhintSigner
{
    class Program
    {
        static void Main(string[] args)
        {
            try
            {
                ReadMessage(HandleMessage);
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine("Fatal error: " + ex.Message);
                Environment.Exit(1);
            }
        }

        static void HandleMessage(Dictionary<string, object> msg)
        {
            Console.Error.WriteLine("Received message: " + new JavaScriptSerializer().Serialize(msg));

            try
            {
                object result = null;

                if (msg.ContainsKey("action"))
                {
                    string action = msg["action"].ToString();

                    if (action == "listCerts")
                    {
                        result = ListCertificates();
                    }
                    else if (action == "sign")
                    {
                        if (!msg.ContainsKey("hash") || !msg.ContainsKey("thumbprint"))
                        {
                            throw new Exception("Missing required parameters for sign action");
                        }

                        result = SignHash(msg["hash"].ToString(), msg["thumbprint"].ToString());
                    }
                    else
                    {
                        throw new Exception("Unknown action: " + action);
                    }
                }

                WriteMessage(new { result });
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine("Error: " + ex.Message);
                WriteMessage(new { error = ex.Message });
            }
        }

        static List<object> ListCertificates()
        {
            var results = new List<object>();

            using (var store = new X509Store(StoreName.My, StoreLocation.CurrentUser))
            {
                store.Open(OpenFlags.ReadOnly);

                var now = DateTime.Now;

                foreach (var cert in store.Certificates)
                {
                    // Filter: must have private key and be currently valid
                    if (cert.HasPrivateKey && cert.NotAfter > now && cert.NotBefore < now)
                    {
                        try
                        {
                            // Parse subject to extract friendly name
                            string subject = cert.Subject;
                            string displayName = "";

                            // Try to extract Given Name and Surname first (for personal certs)
                            var gnMatch = Regex.Match(subject, @"G=([^,+]+)");
                            var snMatch = Regex.Match(subject, @"SN=([^,+]+)");

                            if (gnMatch.Success && snMatch.Success)
                            {
                                displayName = gnMatch.Groups[1].Value.Trim() + " " + snMatch.Groups[1].Value.Trim();
                            }
                            else
                            {
                                // Otherwise use CN (Common Name)
                                var cnMatch = Regex.Match(subject, @"CN=([^,]+)");
                                if (cnMatch.Success)
                                {
                                    displayName = cnMatch.Groups[1].Value.Trim();
                                }
                            }

                            // If still no display name, fall back to first part of subject
                            if (string.IsNullOrEmpty(displayName))
                            {
                                displayName = subject.Split(',')[0].Replace("CN=", "").Trim();
                            }

                            // Extract Organization (O)
                            var orgMatch = Regex.Match(subject, @"\bO=([^,+]+)");
                            string organization = orgMatch.Success ? orgMatch.Groups[1].Value.Trim() : "";

                            // Add organization to display name if present
                            if (!string.IsNullOrEmpty(organization))
                            {
                                displayName += " (" + organization + ")";
                            }

                            // Extract issuer CN
                            string issuer = cert.Issuer;
                            var issuerCnMatch = Regex.Match(issuer, @"CN=([^,]+)");
                            string issuerName = issuerCnMatch.Success 
                                ? issuerCnMatch.Groups[1].Value.Trim() 
                                : issuer.Split(',')[0].Trim();

                            // Add expiry date to label for context
                            string expiry = cert.NotAfter.ToShortDateString();

                            results.Add(new
                            {
                                label = "Issued for: " + displayName + " | Issuer: " + issuerName + " (expires " + expiry + ")",
                                thumbprint = cert.Thumbprint,
                                subject = cert.Subject,
                                issuer = cert.Issuer,
                                notBefore = cert.NotBefore.ToString("o"),
                                notAfter = cert.NotAfter.ToString("o"),
                                hasPrivateKey = cert.HasPrivateKey,
                                cert = Convert.ToBase64String(cert.RawData)
                            });
                        }
                        catch (Exception ex)
                        {
                            Console.Error.WriteLine("Error processing certificate " + cert.Thumbprint + ": " + ex.Message);
                        }
                    }
                }

                store.Close();
            }

            return results;
        }

        static string SignHash(string hashB64, string thumbprint)
        {
            // Validate inputs
            if (string.IsNullOrEmpty(hashB64))
            {
                throw new Exception("Hash is required and must be a string");
            }

            if (string.IsNullOrEmpty(thumbprint))
            {
                throw new Exception("Thumbprint is required and must be a string");
            }

            // Validate base64 format
            if (!Regex.IsMatch(hashB64, @"^[A-Za-z0-9+/]*={0,2}$"))
            {
                throw new Exception("Hash must be valid base64 encoded string");
            }

            byte[] hashBytes;
            try
            {
                hashBytes = Convert.FromBase64String(hashB64);
            }
            catch (Exception ex)
            {
                throw new Exception("Invalid base64 hash: " + ex.Message);
            }

            using (var store = new X509Store(StoreName.My, StoreLocation.CurrentUser))
            {
                store.Open(OpenFlags.ReadOnly);

                var cert = store.Certificates
                    .Find(X509FindType.FindByThumbprint, thumbprint, false)
                    .Cast<X509Certificate2>()
                    .FirstOrDefault();

                if (cert == null)
                {
                    throw new Exception("Certificate not found");
                }

                if (!cert.HasPrivateKey)
                {
                    throw new Exception("Certificate has no private key");
                }

                try
                {
                    using (RSA rsa = cert.GetRSAPrivateKey())
                    {
                        if (rsa == null)
                        {
                            throw new Exception("Unable to get RSA private key from certificate");
                        }

                        byte[] signature = rsa.SignHash(hashBytes, HashAlgorithmName.SHA256, RSASignaturePadding.Pkcs1);
                        return Convert.ToBase64String(signature);
                    }
                }
                finally
                {
                    store.Close();
                }
            }
        }

        // Chrome Native Messaging protocol implementation
        static void ReadMessage(Action<Dictionary<string, object>> callback)
        {
            using (var stdin = Console.OpenStandardInput())
            using (var reader = new BinaryReader(stdin))
            {
                while (true)
                {
                    try
                    {
                        // Read message length (4 bytes, little-endian)
                        byte[] lengthBytes = reader.ReadBytes(4);
                        if (lengthBytes.Length < 4)
                        {
                            // EOF reached
                            return;
                        }

                        int messageLength = BitConverter.ToInt32(lengthBytes, 0);

                        if (messageLength <= 0 || messageLength > 10 * 1024 * 1024)
                        {
                            throw new Exception("Invalid message length: " + messageLength);
                        }

                        // Read the message
                        byte[] messageBytes = reader.ReadBytes(messageLength);
                        if (messageBytes.Length < messageLength)
                        {
                            throw new Exception("Incomplete message received");
                        }

                        string messageJson = Encoding.UTF8.GetString(messageBytes);
                        JavaScriptSerializer serializer = new JavaScriptSerializer();
                        Dictionary<string, object> message = serializer.Deserialize<Dictionary<string, object>>(messageJson);

                        callback(message);
                    }
                    catch (EndOfStreamException)
                    {
                        // Normal exit when stdin is closed
                        return;
                    }
                }
            }
        }

        static void WriteMessage(object message)
        {
            JavaScriptSerializer serializer = new JavaScriptSerializer();
            string json = serializer.Serialize(message);
            byte[] messageBytes = Encoding.UTF8.GetBytes(json);
            byte[] lengthBytes = BitConverter.GetBytes(messageBytes.Length);

            using (var stdout = Console.OpenStandardOutput())
            using (var writer = new BinaryWriter(stdout))
            {
                writer.Write(lengthBytes);
                writer.Write(messageBytes);
                writer.Flush();
            }
        }
    }
}
