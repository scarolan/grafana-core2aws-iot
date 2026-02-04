# ATECC608 Secure Element Architecture

## Overview

The **ATECC608** (Microchip/Atmel Crypto Authentication) is a hardware security module (HSM) that provides secure key storage and cryptographic operations. The M5Stack Core2 for AWS includes this chip at I2C address `0x35` to enable hardware-backed TLS authentication.

## Why a Secure Element?

Traditional software-based security stores private keys in flash memory or files:
- ❌ Keys can be extracted with flash dump tools
- ❌ Keys exist in RAM during crypto operations (vulnerable to attacks)
- ❌ Malware can copy the key file

The ATECC608 provides **hardware-enforced security**:
- ✓ Private key **generated inside the chip** and never leaves
- ✓ Cryptographic operations (signing, ECDH) happen **inside the chip**
- ✓ Keys stored in **one-time programmable (OTP) memory** with lockable slots
- ✓ Physical tampering resistance
- ✓ Side-channel attack countermeasures

## Chip Specifications

| Feature | Specification |
|---------|---------------|
| **Algorithm** | ECDSA with NIST P-256 curve (secp256r1) |
| **Key Length** | 256-bit ECC (equivalent to ~3072-bit RSA) |
| **Interface** | I2C (address 0x35 on Core2 AWS, 0x60 standard) |
| **Slots** | 16 configurable slots (0-15) |
| **Storage** | 10.5 KB total EEPROM |
| **Random Number Generator** | True hardware RNG (FIPS 800-90 B/C compliant) |
| **Hash Engine** | SHA-256 in hardware |
| **Lifespan** | 25+ years data retention |

## Slot Architecture

The ATECC608 has **16 slots** (numbered 0-15). Each slot can store:
- **Private Key** (32 bytes, ECC P-256)
- **Public Key** (64 bytes, X and Y coordinates)
- **Data/Certificate** (up to 72 bytes per slot, 416 bytes total across slots)
- **Configuration** (read/write permissions, key policies)

### Slot Types

| Slot | Typical Use | Size | Lockable? |
|------|-------------|------|-----------|
| **0** | Primary private key | 32 bytes | Yes |
| **1-7** | Additional private keys | 32 bytes each | Yes |
| **8** | Certificate metadata/storage | 72 bytes | Yes |
| **9-15** | Data storage or keys | Varies | Yes |

### Slot Configuration Options

Each slot has a **SlotConfig** that defines:

```
┌─────────────────────────────────────────────────────┐
│ Slot Configuration Attributes                        │
├─────────────────────────────────────────────────────┤
│ • ReadKey: Slot requires authorization to read       │
│ • IsSecret: Slot contains a private key (encrypted)  │
│ • EncryptRead: Encrypt data when reading             │
│ • LimitedUse: Key can only be used N times           │
│ • NoMac: Disable MAC generation                      │
│ • WriteConfig: Write permission settings             │
│ • WriteKey: Slot needed to authorize writes          │
└─────────────────────────────────────────────────────┘
```

## Core2 AWS Factory Provisioning

The M5Stack Core2 for AWS comes **pre-provisioned** from the factory:

### Factory Configuration

| Slot | Contents | Purpose |
|------|----------|---------|
| **0** | **ECC P-256 Private Key** | Used for TLS client authentication |
| **8** | Certificate metadata | Used by ECCX08SelfSignedCert for reconstruction |
| **10** | Compressed certificate | Factory-generated certificate (problematic format) |
| **Config** | Locked | Slot configurations locked (cannot change) |
| **Data** | Unlocked (varies) | Key material locked, data slots may vary |

**Important:** The chip arrives in a **partially locked** state:
- ✓ Configuration zone: **LOCKED** (slot policies fixed)
- ✓ Private key in slot 0: **LOCKED** (cannot be read or changed)
- ? Data/OTP zones: May or may not be locked (device-specific)

## The Lock Mechanism

The ATECC608 uses **one-way locks** - once locked, configuration and keys are **permanent**.

### Lock Stages

```
┌─────────────────────────────────────────────────────┐
│ Stage 1: Configuration Unlocked                     │
├─────────────────────────────────────────────────────┤
│ • You can write slot configurations                 │
│ • You can define read/write policies                │
│ • You can set which slots hold keys vs data         │
│                                                      │
│         ↓ LOCK CONFIG (ONE-WAY, PERMANENT)          │
│                                                      │
├─────────────────────────────────────────────────────┤
│ Stage 2: Configuration Locked, Data Unlocked        │
├─────────────────────────────────────────────────────┤
│ • Slot policies are now permanent                   │
│ • You can write private keys to slots               │
│ • You can write certificates/data                   │
│                                                      │
│         ↓ LOCK DATA (ONE-WAY, PERMANENT)            │
│                                                      │
├─────────────────────────────────────────────────────┤
│ Stage 3: Fully Locked (Production Mode)             │
├─────────────────────────────────────────────────────┤
│ • Keys cannot be read or changed                    │
│ • Keys can only be USED (sign, ECDH)                │
│ • Some data slots may remain writable if configured │
│ • This is the state your Core2 AWS ships in         │
└─────────────────────────────────────────────────────┘
```

### Why Lock?

Once locked:
- ✓ Private key **cannot be extracted** (even with physical access)
- ✓ Slot configurations **cannot be changed** (attacker can't weaken security)
- ✓ Device identity is **permanent** (key tied to specific chip forever)

### Checking Lock Status

From `aws_iot.cpp:34-38`:
```cpp
if (!ECCX08.locked()) {
    Serial.println("WARNING: ATECC608 is not locked!");
    Serial.println("Device may need provisioning.");
}
```

## Private Key Operations

### Key Generation (Factory)

When M5Stack manufactures the Core2 AWS:

```
1. Factory generates private key INSIDE the ATECC608
   └─> GenKey command with KeyID=0
   └─> Private key stored in slot 0 (never exposed)

2. Factory locks configuration zone
   └─> Lock(Config) command
   └─> Slot policies now permanent

3. Factory locks data zone
   └─> Lock(Data) command
   └─> Private key in slot 0 now immutable

4. Factory generates compressed certificate
   └─> Sign command using slot 0
   └─> Certificate stored in slot 10 (compressed format)
```

**Result:** Private key exists only inside the chip, locked, unreadable.

### Private Key Never Leaves the Chip

When performing TLS/MQTT authentication:

```
┌────────────────────────────────────────────────────┐
│ TLS Handshake (Client Certificate Authentication)  │
└────────────────────────────────────────────────────┘

1. AWS IoT sends: "Prove you have the private key"
   └─> Sends a challenge (random data to sign)

2. ESP32 sends challenge to ATECC608 via I2C
   └─> "Sign this data using private key in slot 0"

3. ATECC608 performs signing INSIDE the chip
   └─> Reads private key from slot 0
   └─> Computes ECDSA signature
   └─> Returns SIGNATURE (not the key!)

4. ESP32 sends signature back to AWS IoT
   └─> AWS validates signature with public key from certificate

5. AWS IoT: "Signature valid, you must have the private key"
   ✓ Connection authenticated
```

**Key point:** The ESP32 **never sees the private key**, only signatures.

### Code Implementation

From `aws_iot.cpp:62`:
```cpp
// Configure BearSSL to use ATECC608 for private key operations
sslClient.setEccSlot(PRIVATE_KEY_SLOT, DEVICE_CERTIFICATE);
```

This tells BearSSL:
- "When you need to sign something, use the private key in slot 0 of the ATECC608"
- "Here's the certificate that contains the public key"

BearSSL then:
1. Receives TLS challenge from AWS IoT
2. Sends `Sign` command to ATECC608 via I2C
3. Gets signature back
4. Continues TLS handshake with signature

The private key is **used but never exposed**.

## Certificate Generation Problem

### The Compressed Certificate Issue

The factory-generated certificate in slot 10 has issues:

```
Original device.pem (from slot 10):
❌ Issuer: (empty)
❌ Subject: (empty)
❌ Valid From: Aug 28 2005
❌ Valid To: Aug 28 2005
❌ Format: Compressed/reconstructed format

AWS IoT Response:
🔴 CertificateValidationException: The certificate could not be parsed
```

**Why?** The ATECC608 stores certificates in a **compressed format** to save space (only 72 bytes per slot). The certificate must be reconstructed, but the factory reconstruction creates invalid dates and empty fields.

### Our Solution: Generate New Certificate

From `extras/generate_cert/src/main.cpp`:

```cpp
// Use private key slot 0 (EXISTING key, locked in chip)
int privateKeySlot = 0;

// Use storage slot 8 for certificate metadata
int storageSlot = 8;

// DON'T generate new key - use the existing locked key!
bool generateNewKey = false;

ECCX08SelfSignedCert.beginStorage(privateKeySlot, storageSlot, generateNewKey);

// Set proper certificate fields
ECCX08SelfSignedCert.setCommonName(serialNumber);
ECCX08SelfSignedCert.setIssueYear(2026);
ECCX08SelfSignedCert.setIssueMonth(2);
ECCX08SelfSignedCert.setIssueDay(3);
ECCX08SelfSignedCert.setExpireYears(30);

// Generate certificate (signing happens IN the chip using slot 0)
String cert = ECCX08SelfSignedCert.endStorage();
```

### What This Does

```
1. Read public key from slot 0
   └─> GenKey(mode=PublicKeyComputation, slot=0)
   └─> Chip returns PUBLIC key (this is okay!)

2. Build X.509 certificate structure
   └─> Subject: CN=012333B76CAC4C3701
   └─> Issuer: CN=012333B76CAC4C3701 (self-signed)
   └─> Valid: 2026-02-03 to 2056-02-03
   └─> Public Key: (from step 1)

3. Sign the certificate
   └─> Hash certificate with SHA-256
   └─> Send hash to chip: Sign(slot=0, hash)
   └─> Chip signs with PRIVATE KEY (inside chip)
   └─> Returns SIGNATURE

4. Complete certificate
   └─> Certificate + Signature = Valid X.509 certificate
   └─> Print to serial for copy/paste
```

**Critical:** The private key is used for signing but **never read or exposed**.

### Hardware Security Maintained ✓

- ✓ Same private key as factory (slot 0, locked)
- ✓ Private key never left the chip
- ✓ Certificate properly formatted for AWS IoT
- ✓ Can regenerate certificate anytime (same key, new dates)

## Serial Number as Device ID

The ATECC608 has a **unique 9-byte serial number** burned in at manufacture (unmodifiable).

From `aws_iot.cpp:30-32`:
```cpp
// Get device serial number (used as Thing name / client ID)
deviceId = ECCX08.serialNumber();
Serial.printf("ATECC608 initialized. Device ID: %s\n", deviceId.c_str());
```

**Serial Number:** `012333B76CAC4C3701`
- First byte: `01` (manufacturer code)
- Next 8 bytes: Unique device identifier

This becomes:
- **AWS IoT Thing Name:** `012333B76CAC4C3701`
- **MQTT Client ID:** `012333B76CAC4C3701`
- **Certificate Common Name:** `CN=012333B76CAC4C3701`

## Security Model Summary

### Without ATECC608 (Software-Only)

```
┌──────────────────────────────────────────┐
│ ESP32 Flash Memory                       │
├──────────────────────────────────────────┤
│ private_key.pem (PEM file)               │
│ -----BEGIN PRIVATE KEY-----              │
│ MIGHAgEAMBMG... [PRIVATE KEY HERE]       │
│ -----END PRIVATE KEY-----                │
└──────────────────────────────────────────┘
         ↓
❌ Can be dumped with esptool.py
❌ Exists in RAM during use
❌ Can be copied/extracted
```

### With ATECC608 (Hardware-Backed)

```
┌──────────────────────────────────────────┐
│ ATECC608 Slot 0 (Locked)                 │
├──────────────────────────────────────────┤
│ Private Key: [LOCKED, UNREADABLE]        │
│                                           │
│ Available operations:                    │
│  • Sign(data) → signature                │
│  • ECDH(public_key) → shared_secret      │
│  • GenPublicKey() → public_key           │
│                                           │
│ Unavailable operations:                  │
│  • Read() → FORBIDDEN                    │
│  • Write() → FORBIDDEN (locked)          │
└──────────────────────────────────────────┘
         ↓
✓ Cannot be extracted (even with physical access)
✓ Operations happen inside tamper-resistant chip
✓ Key tied to this specific device forever
```

## Comparison Table

| Feature | Software Key | ATECC608 Hardware Key |
|---------|-------------|----------------------|
| **Storage** | Flash file | Locked slot in secure element |
| **Extractable** | Yes (flash dump) | No (hardware-enforced) |
| **Exists in RAM** | Yes (during crypto ops) | No (operations in chip) |
| **Can be copied** | Yes | No |
| **Device-specific** | No (key is portable) | Yes (key locked to chip) |
| **Tamper resistance** | None | Physical & side-channel countermeasures |
| **Cost** | Free | ~$1-2 per chip |
| **Ideal for** | Development/testing | Production IoT devices |

## Real-World Analogy

**Software-only security** is like keeping your house key under the doormat:
- Convenient, but anyone who knows where to look can copy it
- If someone gets the key, they can make copies
- No way to prove it was YOUR key that unlocked the door

**ATECC608 security** is like having a biometric lock:
- The "key" (your fingerprint) never leaves your finger
- To unlock, you must physically be there
- Cannot be copied or transferred
- Proves that THIS specific finger (device) opened the door

## AWS IoT Integration

### Data Flow

```
┌─────────────────────────────────────────────────────────┐
│ AWS IoT Core (TLS 1.2 with mutual authentication)      │
└───────────────────────┬─────────────────────────────────┘
                        │ TLS handshake
                        │ • Verify server cert
                        │ • Send client cert
                        │ • Sign challenge
                        ↓
┌─────────────────────────────────────────────────────────┐
│ ESP32: BearSSL TLS Library                              │
├─────────────────────────────────────────────────────────┤
│ sslClient.setEccSlot(0, DEVICE_CERTIFICATE)             │
│  • Knows: public cert, slot number                      │
│  • Doesn't know: private key                            │
└───────────────────────┬─────────────────────────────────┘
                        │ I2C commands
                        │ • Sign(slot=0, hash)
                        │ • Response: signature
                        ↓
┌─────────────────────────────────────────────────────────┐
│ ATECC608 @ 0x35                                         │
├─────────────────────────────────────────────────────────┤
│ Slot 0: [LOCKED PRIVATE KEY]                            │
│  • Receives: hash to sign                               │
│  • Computes: ECDSA signature using slot 0               │
│  • Returns: signature (NOT the key)                     │
└─────────────────────────────────────────────────────────┘
```

### Certificate Registration

```bash
# Certificate contains:
#  - Public key (can be shared freely)
#  - Device serial number (CN=012333B76CAC4C3701)
#  - Validity dates (2026-2056)
#  - Signature (proves it was signed by slot 0)

aws iot register-certificate-without-ca \
  --certificate-pem file://device_new.pem \
  --status ACTIVE \
  --region us-east-1

# Returns: certificateArn (used to attach policies/things)
```

AWS IoT now knows:
- This device has the public key from the certificate
- To authenticate, device must sign challenges with matching private key
- Only the chip with serial `012333B76CAC4C3701` can generate valid signatures

## Additional ATECC608 Features

Beyond private key storage, the ATECC608 offers:

### 1. Hardware Random Number Generator (RNG)
```cpp
byte random[32];
ECCX08.random(random, sizeof(random));
// True hardware randomness (not pseudo-random)
```

### 2. Secure Storage
Slots can store non-key data with encryption/authentication:
- API keys
- Passwords
- Configuration data

### 3. ECDH Key Agreement
```cpp
// Compute shared secret without exposing private key
ECCX08.ecdhWithSlot(slot, publicKey, sharedSecret);
```

### 4. SHA-256 in Hardware
```cpp
// Faster than software SHA-256
ECCX08.beginSHA256();
ECCX08.updateSHA256(data, len);
ECCX08.endSHA256(hash);
```

### 5. Monotonic Counter
Slots can act as tamper-evident counters (cannot decrement, only increment).

## Limitations

### What ATECC608 Does NOT Do

- ❌ **AES encryption** (ATECC608 is ECC-only; use ATECC608A for AES)
- ❌ **RSA** (only NIST P-256 ECC)
- ❌ **TLS acceleration** (only handles private key ops, not full TLS)
- ❌ **Large data storage** (only 10.5KB total)
- ❌ **Unlocking** (once locked, permanent)

### Slot 0 Cannot Be Changed

On the Core2 AWS:
- Slot 0 private key is **locked forever**
- You can't generate a new key
- You can't delete the existing key
- You can only **use** the key (sign, ECDH)

**Advantage:** Device identity is permanent (can't be spoofed or changed)
**Disadvantage:** If AWS cert is compromised, must revoke and use new cert (can't rekey the chip)

## References

- **ATECC608 Datasheet:** [Microchip ATECC608A-MAHDA-T](https://www.microchip.com/wwwproducts/en/ATECC608A)
- **ArduinoECCX08 Library:** [GitHub - arduino-libraries/ArduinoECCX08](https://github.com/arduino-libraries/ArduinoECCX08)
- **BearSSL Integration:** [GitHub - arduino-libraries/ArduinoBearSSL](https://github.com/arduino-libraries/ArduinoBearSSL)
- **M5Stack Core2 AWS:** [M5Stack Docs](https://docs.m5stack.com/en/core/core2_for_aws)
- **AWS IoT Device SDK:** Uses certificates for mutual TLS authentication

## Code Locations

- `src/aws_iot.cpp:19-41` - ATECC608 initialization and lock check
- `src/aws_iot.cpp:52-89` - BearSSL configuration with ECC slot
- `extras/generate_cert/src/main.cpp` - Certificate generation utility
- `src/secrets.h` - Device certificate (public key + signature)
- `ATECC608_CERTIFICATE_SOLUTION.md` - Certificate generation troubleshooting
