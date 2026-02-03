// Certificate Extraction for M5Stack Core2 AWS
// Generates a self-signed certificate using the ATECC608's private key
// The private key never leaves the chip - only the public cert is exported

#include <Arduino.h>
#include <Wire.h>
#include <ArduinoECCX08.h>
#include <utility/ECCX08SelfSignedCert.h>
#include <utility/PEMUtils.h>

#define ATECC608_ADDRESS 0x35
#define KEY_SLOT 0
#define CERT_DATA_SLOT 10  // Slot for storing cert date/signature

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    delay(2000);

    Serial.println("\n============================================");
    Serial.println("  M5Stack Core2 AWS - Certificate Generator");
    Serial.println("============================================\n");

    // Initialize I2C for ATECC608
    Wire.begin(21, 22, 100000);

    // Initialize ATECC608 at address 0x35
    if (!ECCX08.begin(ATECC608_ADDRESS)) {
        Serial.println("ERROR: Failed to initialize ATECC608!");
        Serial.println("Make sure you're using Core2 for AWS (not regular Core2)");
        while (1) delay(1000);
    }

    Serial.println("ATECC608 initialized successfully!\n");

    // Get and print device serial number
    String serialNum = ECCX08.serialNumber();
    Serial.println("=== DEVICE SERIAL NUMBER ===");
    Serial.println("(Use this as your AWS IoT Thing name)");
    Serial.println();
    Serial.println(serialNum);
    Serial.println();

    // Check if chip is locked
    if (!ECCX08.locked()) {
        Serial.println("WARNING: ATECC608 is not locked.");
        Serial.println("Will generate a new self-signed certificate.\n");
    } else {
        Serial.println("ATECC608 is locked and ready.\n");
    }

    // Try to reconstruct existing certificate first
    Serial.println("Attempting to reconstruct existing certificate...");

    bool certReady = false;

    if (ECCX08SelfSignedCert.beginReconstruction(KEY_SLOT, CERT_DATA_SLOT)) {
        if (ECCX08SelfSignedCert.endReconstruction()) {
            Serial.println("Successfully reconstructed certificate from device.\n");
            certReady = true;
        }
    }

    if (!certReady) {
        Serial.println("No existing certificate found. Generating new self-signed certificate...\n");

        // Configure certificate details
        ECCX08SelfSignedCert.setCommonName(serialNum);
        ECCX08SelfSignedCert.setOrganizationName("M5Stack");
        ECCX08SelfSignedCert.setCountryName("US");

        // Set validity period
        ECCX08SelfSignedCert.setIssueYear(2024);
        ECCX08SelfSignedCert.setIssueMonth(1);
        ECCX08SelfSignedCert.setIssueDay(1);
        ECCX08SelfSignedCert.setIssueHour(0);
        ECCX08SelfSignedCert.setExpireYears(40);

        // Generate new key = false (use existing key in slot 0)
        if (!ECCX08SelfSignedCert.beginStorage(KEY_SLOT, CERT_DATA_SLOT, false)) {
            Serial.println("ERROR: Failed to begin certificate storage");
            Serial.println("The ATECC608 may not support this operation.");

            // Fall back to generating cert without storing
            Serial.println("\nGenerating certificate without persistent storage...");
        }

        String result = ECCX08SelfSignedCert.endStorage();
        if (result.length() > 0) {
            Serial.println("Certificate generated and stored.\n");
            certReady = true;
        } else {
            Serial.println("WARNING: Could not store certificate.\n");
        }
    }

    // Get the certificate bytes
    uint8_t* certBytes = ECCX08SelfSignedCert.bytes();
    int certLen = ECCX08SelfSignedCert.length();

    if (certBytes && certLen > 0) {
        // Convert to PEM format
        String certPEM = PEMUtils.base64Encode(certBytes, certLen,
            "-----BEGIN CERTIFICATE-----\n",
            "-----END CERTIFICATE-----\n");

        Serial.println("=== DEVICE CERTIFICATE (PEM) ===");
        Serial.println("Copy everything between and including the BEGIN/END lines");
        Serial.println("Paste this into your secrets.h DEVICE_CERTIFICATE");
        Serial.println();
        Serial.println(certPEM);
        Serial.println("=== END CERTIFICATE ===\n");

        // Print SHA1 fingerprint
        String sha1 = ECCX08SelfSignedCert.sha1();
        Serial.println("Certificate SHA1 fingerprint:");
        Serial.println(sha1);
        Serial.println();
    } else {
        Serial.println("ERROR: Failed to get certificate bytes");
    }

    // Also print the public key for reference
    byte publicKey[64];
    if (ECCX08.generatePublicKey(KEY_SLOT, publicKey)) {
        Serial.println("=== PUBLIC KEY (hex, 64 bytes) ===");
        for (int i = 0; i < 64; i++) {
            if (publicKey[i] < 0x10) Serial.print("0");
            Serial.print(publicKey[i], HEX);
            if ((i + 1) % 32 == 0) Serial.println();
        }
        Serial.println("\n");
    }

    Serial.println("============================================");
    Serial.println("Next steps:");
    Serial.println("1. Copy the certificate above to secrets.h");
    Serial.println("2. Save the certificate to a .pem file");
    Serial.println("3. Register with AWS IoT:");
    Serial.println("   aws iot register-certificate-without-ca \\");
    Serial.println("     --certificate-pem file://device.pem \\");
    Serial.println("     --status ACTIVE");
    Serial.println("============================================");
}

void loop() {
    delay(10000);
}
