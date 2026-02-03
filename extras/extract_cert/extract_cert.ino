// Certificate Extraction Sketch for M5Stack Core2 AWS
// Reads the device certificate from ATECC608 slot 10 and prints to Serial
//
// Flash this, open Serial Monitor at 115200, copy the certificate output

#include <Wire.h>
#include <ArduinoECCX08.h>

#define ATECC608_ADDRESS 0x35
#define CERT_SLOT 10
#define CERT_LENGTH 72  // Compressed cert length

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    delay(1000);

    Serial.println("\n========================================");
    Serial.println("  ATECC608 Certificate Extractor");
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

    // Print device serial number
    String serialNum = ECCX08.serialNumber();
    Serial.println("Device Serial Number (use as Thing name):");
    Serial.println(serialNum);
    Serial.println();

    // Check if locked
    if (!ECCX08.locked()) {
        Serial.println("WARNING: ATECC608 is not locked!");
        Serial.println("Device may not be provisioned.");
    }

    // Read compressed certificate from slot 10
    byte compressedCert[CERT_LENGTH];
    if (!ECCX08.readSlot(CERT_SLOT, compressedCert, CERT_LENGTH)) {
        Serial.println("ERROR: Failed to read certificate from slot 10");
        while (1) delay(1000);
    }

    // The ATECC608 stores a compressed certificate that needs to be
    // reconstructed. For AWS IoT, you typically need the full PEM certificate.
    //
    // Option 1: Use the ECCX08CertClass to reconstruct (if available)
    // Option 2: Use the ESP Cryptoauth utility to extract
    //
    // For now, print the raw compressed cert data:

    Serial.println("Compressed certificate data (hex):");
    for (int i = 0; i < CERT_LENGTH; i++) {
        if (compressedCert[i] < 0x10) Serial.print("0");
        Serial.print(compressedCert[i], HEX);
        if ((i + 1) % 16 == 0) Serial.println();
        else Serial.print(" ");
    }
    Serial.println("\n");

    // Generate the public key from slot 0
    byte publicKey[64];
    if (ECCX08.generatePublicKey(0, publicKey)) {
        Serial.println("Public Key (64 bytes, uncompressed without 0x04 prefix):");
        for (int i = 0; i < 64; i++) {
            if (publicKey[i] < 0x10) Serial.print("0");
            Serial.print(publicKey[i], HEX);
            if ((i + 1) % 16 == 0) Serial.println();
            else Serial.print(" ");
        }
        Serial.println();
    }

    Serial.println("\n========================================");
    Serial.println("To get the full PEM certificate, use one of:");
    Serial.println("1. Espressif's esp_cryptoauth_utility");
    Serial.println("2. M5Stack's provisioning tool");
    Serial.println("3. Microchip Trust Platform");
    Serial.println("========================================");
}

void loop() {
    delay(10000);
}
