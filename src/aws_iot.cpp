#include "aws_iot.h"
#include "config.h"
#include "secrets.h"

#include <WiFi.h>
#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>
#include <ArduinoMqttClient.h>
#include <time.h>

// Network clients
static WiFiClient wifiClient;
static BearSSLClient sslClient(wifiClient);
static MqttClient mqttClient(sslClient);

// Device identifier from ATECC608 serial number
static String deviceId;

bool awsInitSecureElement() {
    // Initialize I2C for ATECC608 (address 0x35 on Core2 AWS)
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQUENCY);

    // Use 0x35 address for Core2 AWS ATECC608
    if (!ECCX08.begin(ATECC608_ADDRESS)) {
        Serial.println("ERROR: ATECC608 initialization failed!");
        Serial.println("Check I2C connection and address (0x35 for Core2 AWS)");
        return false;
    }

    // Get device serial number (used as Thing name / client ID)
    deviceId = ECCX08.serialNumber();
    Serial.printf("ATECC608 initialized. Device ID: %s\n", deviceId.c_str());

    // Check if device is locked (required for crypto operations)
    if (!ECCX08.locked()) {
        Serial.println("WARNING: ATECC608 is not locked!");
        Serial.println("Device may need provisioning.");
    }

    return true;
}

String awsGetDeviceId() {
    return deviceId;
}

unsigned long awsGetTime() {
    // Get time from NTP (configured in wifi_manager)
    return (unsigned long)time(nullptr);
}

bool awsConnect() {
    if (deviceId.length() == 0) {
        Serial.println("ERROR: Secure element not initialized");
        return false;
    }

    Serial.println("Configuring BearSSL with ATECC608...");

    // Configure BearSSL to use ATECC608 for private key operations
    ArduinoBearSSL.onGetTime(awsGetTime);
    sslClient.setEccSlot(PRIVATE_KEY_SLOT, DEVICE_CERTIFICATE);

    // Set MQTT client ID to device serial number
    mqttClient.setId(deviceId);

    // Set keep-alive and timeout
    mqttClient.setKeepAliveInterval(60 * 1000);  // 60 seconds
    mqttClient.setConnectionTimeout(10 * 1000);   // 10 seconds

    Serial.printf("Connecting to AWS IoT: %s:%d\n", AWS_IOT_ENDPOINT, MQTT_PORT);

    if (!mqttClient.connect(AWS_IOT_ENDPOINT, MQTT_PORT)) {
        int err = mqttClient.connectError();
        Serial.printf("MQTT connect failed! Error code: %d\n", err);

        // Decode common error codes
        switch (err) {
            case -1: Serial.println("  -> Connection refused"); break;
            case -2: Serial.println("  -> Timeout"); break;
            case -3: Serial.println("  -> Network error"); break;
            default: Serial.println("  -> Unknown error"); break;
        }
        return false;
    }

    Serial.println("Connected to AWS IoT Core!");
    return true;
}

bool awsIsConnected() {
    return mqttClient.connected();
}

bool awsPublish(const char* topic, const char* payload) {
    if (!mqttClient.connected()) {
        Serial.println("Cannot publish: not connected to AWS IoT");
        return false;
    }

    mqttClient.beginMessage(topic);
    mqttClient.print(payload);
    int result = mqttClient.endMessage();

    if (result) {
        Serial.printf("Published to %s (%d bytes)\n", topic, strlen(payload));
        return true;
    } else {
        Serial.printf("Publish failed to %s\n", topic);
        return false;
    }
}

void awsMaintain() {
    mqttClient.poll();
}
