#include <M5Unified.h>
#include "config.h"
#include "wifi_manager.h"
#include "aws_iot.h"
#include "imu_sampler.h"
#include "telemetry.h"
#include "display_ui.h"

// Timing variables
static unsigned long lastTelemetryTime = 0;
static unsigned long lastDisplayTime = 0;

// State tracking
static bool awsInitialized = false;
static bool awsConnectedState = false;

void setup() {
    // Initialize M5Stack
    auto cfg = M5.config();
    cfg.internal_imu = true;  // Enable internal IMU
    M5.begin(cfg);

    Serial.begin(115200);
    delay(100);

    Serial.println("\n========================================");
    Serial.println("  Vibration Monitoring IoT Demo");
    Serial.println("  M5Stack Core2 AWS + AWS IoT Core");
    Serial.println("========================================\n");

    // Initialize display
    displayInit();
    displayDrawStatusScreen();

    // Initialize ATECC608 secure element
    Serial.println("Initializing secure element...");
    if (!awsInitSecureElement()) {
        Serial.println("FATAL: Secure element init failed!");
        M5.Lcd.setTextColor(TFT_RED);
        M5.Lcd.drawString("ATECC608 INIT FAILED", 10, 100);
        while (1) delay(1000);
    }

    // Connect to WiFi
    Serial.println("Connecting to WiFi...");
    displayUpdate();

    if (!wifiConnect()) {
        Serial.println("FATAL: WiFi connection failed!");
        M5.Lcd.setTextColor(TFT_RED);
        M5.Lcd.drawString("WIFI CONNECT FAILED", 10, 100);
        while (1) delay(1000);
    }

    displayUpdate();

    // Connect to AWS IoT
    Serial.println("Connecting to AWS IoT...");
    if (!awsConnect()) {
        Serial.println("WARNING: AWS IoT connection failed - will retry");
        awsConnectedState = false;
    } else {
        awsConnectedState = true;
    }

    displayUpdate();

    // Start IMU sampling task
    Serial.println("Starting IMU sampling...");
    imuStartSampling();

    // Initial display update
    displayDrawStatusScreen();

    Serial.println("\nSetup complete! Starting main loop...\n");
}

void loop() {
    // Update M5Stack (buttons, touch, etc.)
    M5.update();

    // Maintain WiFi connection
    wifiMaintain();

    // Maintain MQTT connection
    awsMaintain();

    // Handle AWS reconnection if needed
    if (wifiIsConnected() && !awsIsConnected()) {
        if (awsConnectedState || !awsInitialized) {
            Serial.println("AWS IoT disconnected, attempting reconnect...");
            if (awsConnect()) {
                awsConnectedState = true;
                awsInitialized = true;
                Serial.println("Reconnected to AWS IoT");
            } else {
                awsConnectedState = false;
            }
        }
    } else if (awsIsConnected()) {
        awsConnectedState = true;
        awsInitialized = true;
    }

    // Publish telemetry at configured interval
    unsigned long now = millis();
    if (now - lastTelemetryTime >= TELEMETRY_INTERVAL_MS) {
        lastTelemetryTime = now;

        if (awsIsConnected()) {
            if (telemetryPublish()) {
                Serial.println("Telemetry published successfully");
            } else {
                Serial.println("Telemetry publish failed");
            }
        } else {
            Serial.println("Skipping telemetry - not connected to AWS IoT");
        }
    }

    // Update display at configured interval
    if (now - lastDisplayTime >= DISPLAY_UPDATE_INTERVAL_MS) {
        lastDisplayTime = now;
        displayUpdate();
    }

    // Small delay to prevent tight loop
    delay(10);
}
