# ATECC608 Certificate Generation - The Solution

## Problem: Why the Original Certificate Failed

The ATECC608 secure element stores certificates in a **compressed format** that AWS IoT's API rejects. When you read the certificate from slot 10 using standard methods, you get a certificate with structural issues.

### Original `device.pem` Issues:

**Problems:**
- ❌ Empty issuer field (DN is null)
- ❌ Empty subject field (DN is null
- ❌ Invalid dates: NotBefore and NotAfter both set to `Aug 28 2005` (expired!)
- ❌ Compressed certificate format instead of full X.509

**AWS IoT Error:**
```
CertificateValidationException: The certificate could not be parsed
```

## Solution: Generate a Properly Formatted Certificate

The **ArduinoECCX08 library** provides `ECCX08SelfSignedCert` utility that generates a proper X.509 certificate with valid dates and subject fields, while using the existing private key in the ATECC608 chip.

### Why This Works:

1. **Uses existing hardware private key** in slot 0 (never leaves the chip)
2. **Generates proper X.509 certificate** with valid dates and subject
3. **Stores certificate metadata** in storage slot 8
4. **AWS IoT accepts** the properly formatted certificate

### Hardware Security Maintained ✓

The private key **NEVER leaves the ATECC608 chip**. The certificate generation process:
- Reads the public key from slot 0
- Creates a self-signed certificate with proper fields
- Signs it using the private key (operation happens inside the chip)
- Stores metadata in slot 8 for reconstruction

## Step-by-Step: Generate Valid Certificate

### 1. Flash the Certificate Generator Sketch

```bash
cd extras/generate_cert
pio run -t upload --upload-port COM3
```

The sketch (`extras/generate_cert/src/main.cpp`):
- Initializes ATECC608 at address 0x35
- Uses private key slot 0 (existing key, no new key generation)
- Uses storage slot 8 for certificate metadata
- Sets certificate dates: Issue 2026-02-03, Expires 2056-02-03
- Sets Common Name to device serial number

### 2. Run and Capture Certificate

Open serial monitor (115200 baud):
```bash
pio device monitor --port COM3 --baud 115200
```

You'll see the generated certificate with:
- ✓ Valid issuer: CN=012333B76CAC4C3701
- ✓ Valid subject: CN=012333B76CAC4C3701
- ✓ Valid dates: 2026-02-03 to 2056-02-03
- ✓ Full X.509 format

### 3. Save Certificate

Copy the certificate (including BEGIN/END lines) and save it to a file.

### 4. Register with AWS IoT

```bash
# Register certificate
aws iot register-certificate-without-ca \
  --certificate-pem file://device_new.pem \
  --status ACTIVE \
  --region ca-central-1

# Save the certificateArn from output

# Attach policy
aws iot attach-policy \
  --policy-name VibrationMonitorPolicy \
  --target <CERTIFICATE_ARN> \
  --region ca-central-1

# Attach thing
aws iot attach-thing-principal \
  --thing-name 012333B76CAC4C3701 \
  --principal <CERTIFICATE_ARN> \
  --region ca-central-1
```

### 5. Update secrets.h

Edit `src/secrets.h` and replace the `DEVICE_CERTIFICATE` with your new certificate.

### 6. Flash Main Firmware

```bash
pio run -t upload --upload-port COM3
pio device monitor --port COM3 --baud 115200
```

You should see:
```
Connected to AWS IoT Core!
Published to dt/vibration/012333B76CAC4C3701/telemetry (212 bytes)
```

## Technical Details

### Certificate Storage in ATECC608

The ATECC608 has several slot types:

| Slot | Purpose | Used By |
|------|---------|---------|
| 0 | Private key (ECC P-256) | BearSSL for TLS signing |
| 8 | Storage (certificate metadata) | ECCX08SelfSignedCert reconstruction |
| 10 | Compressed certificate (factory) | Original compressed format |

### How ArduinoECCX08 Generates Certificates

```cpp
// From extras/generate_cert/src/main.cpp

// Initialize with existing key (don't generate new one)
ECCX08SelfSignedCert.beginStorage(
    0,      // privateKeySlot - use existing key in slot 0
    8,      // storageSlot - store metadata in slot 8
    false   // generateNewKey - keep existing key!
);

// Set certificate fields
ECCX08SelfSignedCert.setCommonName(serialNumber);
ECCX08SelfSignedCert.setIssueYear(2026);
ECCX08SelfSignedCert.setIssueMonth(2);
ECCX08SelfSignedCert.setIssueDay(3);
ECCX08SelfSignedCert.setExpireYears(30);

// Generate and sign (signing happens in ATECC608)
String cert = ECCX08SelfSignedCert.endStorage();
```

### Why M5Stack Tutorials Work

Official M5Stack tutorials use a complex registration helper that:
1. Uses `esp-cryptoauthlib` to read ATECC608's compressed certificate
2. Reconstructs proper X.509 using Microchip Trust Platform libraries
3. Creates a JWS-signed manifest structure
4. Registers via AWS IoT's manifest-based API

**Our approach is simpler:**
- Generate a new properly formatted certificate using ArduinoECCX08
- Use standard AWS IoT `register-certificate-without-ca` API
- No need for complex manifest structures or Python dependencies

## Key Takeaways

1. **Don't use the compressed certificate from slot 10** - it has structural issues AWS IoT rejects

2. **Generate a proper certificate** using `ECCX08SelfSignedCert` utility

3. **Hardware security is maintained** - private key never leaves the ATECC608

4. **The certificate can be regenerated** anytime by running the generator sketch

5. **No AWS-generated certificates needed** - you're using the actual hardware secure element!

## Comparison

| Approach | Uses ATECC608? | Private Key Security | Complexity |
|----------|----------------|---------------------|------------|
| **ECCX08SelfSignedCert (Our Solution)** | ✓ Yes | Hardware-backed | Simple |
| AWS-generated certificate | ✗ No | File-based | Simple |
| M5Stack registration helper | ✓ Yes | Hardware-backed | Complex |
| Compressed cert from slot 10 | ✓ Tries to | Hardware-backed | Fails |

## Troubleshooting

### Certificate Generation Fails
- Make sure you're running on the Core2 AWS (not regular Core2)
- ATECC608 must be at I2C address 0x35
- If chip is locked, you can't modify slot configurations

### Connection Still Fails After New Certificate
- Verify you updated `secrets.h` with the NEW certificate
- Check certificate is registered as ACTIVE in AWS IoT Console
- Verify policy is attached to the certificate
- Confirm thing is attached to the certificate

### Want to Start Fresh?
Simply delete and recreate the certificate in AWS IoT, then re-run the generator sketch. The private key in the ATECC608 never changes (it's locked), but you can generate new certificates that use it anytime.

## Result

✓ Device connects to AWS IoT using ATECC608 hardware security
✓ Private key never leaves the secure element
✓ Certificate properly formatted and accepted by AWS IoT
✓ Ready for production webinar demo!
