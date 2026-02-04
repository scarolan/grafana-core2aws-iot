# Certificates

This directory contains device certificates for AWS IoT authentication.

## Files

### [device_new.pem](device_new.pem)
The properly formatted device certificate generated using the ATECC608 secure element.

**Details:**
- **Device ID:** 012333B76CAC4C3701
- **Common Name:** CN=012333B76CAC4C3701
- **Valid From:** 2026-02-03
- **Valid To:** 2056-02-03
- **Algorithm:** ECDSA with NIST P-256 curve
- **Certificate ARN:** arn:aws:iot:us-east-1:494614287886:cert/91da4e92657a24f3928e5642d382fc4e6f5bfdfca16a9d98be4e407dc0181689
- **Fingerprint:** 699ea81f9eb0f227f655b3b2ab03157a50677f16

**How it was generated:**
This certificate was created using the [generate_cert](../generate_cert/) sketch, which:
1. Uses the existing locked private key in ATECC608 slot 0
2. Generates a proper X.509 certificate with valid dates and subject fields
3. Signs it using the hardware secure element
4. Never exposes the private key

**Important:** The private key remains locked inside the ATECC608 chip and is never exposed. This certificate contains only the public key.

## Registering with AWS IoT

To register this certificate with AWS IoT Core:

```bash
# Register certificate
aws iot register-certificate-without-ca \
  --certificate-pem file://device_new.pem \
  --status ACTIVE \
  --region us-east-1

# Save the certificateArn from output, then attach policy
aws iot attach-policy \
  --policy-name VibrationMonitorPolicy \
  --target <CERTIFICATE_ARN> \
  --region us-east-1

# Attach thing
aws iot attach-thing-principal \
  --thing-name 012333B76CAC4C3701 \
  --principal <CERTIFICATE_ARN> \
  --region us-east-1
```

## Regenerating the Certificate

If you need to generate a new certificate (same private key, new validity dates):

```bash
cd extras/generate_cert
pio run -t upload --upload-port COM3
pio device monitor --port COM3 --baud 115200
```

Copy the certificate output and save as a new .pem file.

**Note:** The private key never changes (it's locked in the ATECC608), but you can generate new certificates using that key anytime.

## Related Documentation

- [../docs/ATECC608_CERTIFICATE_SOLUTION.md](../../docs/ATECC608_CERTIFICATE_SOLUTION.md) - Certificate troubleshooting guide
- [../docs/ATECC608_ARCHITECTURE.md](../../docs/ATECC608_ARCHITECTURE.md) - Secure element architecture
- [../generate_cert/](../generate_cert/) - Certificate generator sketch
