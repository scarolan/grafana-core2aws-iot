// Self-Signed Certificate Generator for M5Stack Core2 AWS
// Generates a properly formatted certificate with valid dates and subject fields
// that AWS IoT will accept
//
// Based on ArduinoECCX08 ECCX08SelfSignedCert example
// Adapted for Core2 AWS ATECC608 at address 0x35

#include <Wire.h>
#include <ArduinoECCX08.h>
#include <utility/ECCX08SelfSignedCert.h>

#define ATECC608_ADDRESS 0x35

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    delay(1000);

    Serial.println("\n========================================");
    Serial.println("  ATECC608 Certificate Generator");
    Serial.println("  M5Stack Core2 for AWS");
    Serial.println("========================================\n");

    // Initialize I2C
    Wire.begin(21, 22, 100000);

    // Initialize ATECC608 at address 0x35
    if (!ECCX08.begin(ATECC608_ADDRESS)) {
        Serial.println("ERROR: Failed to initialize ATECC608!");
        Serial.println("Check that you're using Core2 for AWS (not regular Core2)");
        while (1) delay(1000);
    }

    String serialNumber = ECCX08.serialNumber();
    Serial.print("ECCX08 Serial Number = ");
    Serial.println(serialNumber);
    Serial.println();

    if (!ECCX08.locked()) {
        Serial.println("WARNING: ATECC608 is not locked!");
        Serial.println("This device may not be properly provisioned.");
        Serial.println("For production devices, the chip should be locked.");
        Serial.println();
    }

    Serial.println("Generating self-signed certificate with valid dates and subject fields...");
    Serial.println();

    // Use private key slot 0 (standard for ATECC608)
    // Use storage slot 8 for certificate metadata
    // Don't generate a new key - use existing key in slot 0
    int privateKeySlot = 0;
    int storageSlot = 8;
    bool generateNewKey = false;

    if (!ECCX08SelfSignedCert.beginStorage(privateKeySlot, storageSlot, generateNewKey)) {
        Serial.println("ERROR: Failed to begin certificate generation!");
        Serial.println("This might fail if the chip is pre-provisioned.");
        Serial.println("Try using the pre-provisioned certificate instead.");
        while (1) delay(1000);
    }

    // Set certificate fields
    ECCX08SelfSignedCert.setCommonName(serialNumber);
    ECCX08SelfSignedCert.setIssueYear(2026);
    ECCX08SelfSignedCert.setIssueMonth(2);
    ECCX08SelfSignedCert.setIssueDay(3);
    ECCX08SelfSignedCert.setIssueHour(0);
    ECCX08SelfSignedCert.setExpireYears(30);  // Valid until 2056

    String cert = ECCX08SelfSignedCert.endStorage();

    if (!cert) {
        Serial.println("ERROR: Certificate generation failed!");
        while (1) delay(1000);
    }

    Serial.println("========================================");
    Serial.println("SUCCESS! Generated certificate:");
    Serial.println("========================================");
    Serial.println(cert);
    Serial.println();

    Serial.print("SHA1 Fingerprint: ");
    Serial.println(ECCX08SelfSignedCert.sha1());
    Serial.println();

    Serial.println("========================================");
    Serial.println("Next steps:");
    Serial.println("1. Copy the certificate above (including BEGIN/END lines)");
    Serial.println("2. Save to device.pem");
    Serial.println("3. Register with AWS IoT:");
    Serial.println("   aws iot register-certificate-without-ca \\");
    Serial.println("     --certificate-pem file://device.pem \\");
    Serial.println("     --status ACTIVE");
    Serial.println("========================================");
}

void loop() {
    delay(10000);
}
