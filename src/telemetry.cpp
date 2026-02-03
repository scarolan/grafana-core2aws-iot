#include "telemetry.h"
#include "config.h"
#include "aws_iot.h"
#include <M5Unified.h>
#include <WiFi.h>
#include <ArduinoJson.h>

String telemetryBuildPayload(const VibrationMetrics& vib, const char* deviceId) {
    JsonDocument doc;

    // Device identification
    doc["device_id"] = deviceId;
    doc["timestamp"] = awsGetTime();

    // Vibration metrics
    JsonObject vibObj = doc["vibration"].to<JsonObject>();
    vibObj["rms_g"] = serialized(String(vib.rms_g, 4));
    vibObj["peak_g"] = serialized(String(vib.peak_g, 4));

    // Device health metrics
    JsonObject health = doc["health"].to<JsonObject>();

    // Battery voltage (mV to V)
    int32_t batteryMv = M5.Power.getBatteryVoltage();
    health["battery_v"] = serialized(String(batteryMv / 1000.0f, 2));

    // Internal temperature from AXP192
    health["temp_c"] = serialized(String(M5.Power.Axp192.getInternalTemperature(), 1));

    // WiFi signal strength
    health["rssi_dbm"] = WiFi.RSSI();

    // System uptime in seconds
    health["uptime_sec"] = millis() / 1000;

    // Free heap memory
    health["free_heap"] = ESP.getFreeHeap();

    // IMU temperature if available
    if (vib.temp_c != 0) {
        health["imu_temp_c"] = serialized(String(vib.temp_c, 1));
    }

    // Serialize to string
    String payload;
    serializeJson(doc, payload);
    return payload;
}

String telemetryGetTopic(const char* deviceId) {
    return String(MQTT_TOPIC_PREFIX) + deviceId + "/telemetry";
}

bool telemetryPublish() {
    VibrationMetrics metrics;

    if (!imuGetLatestMetrics(metrics)) {
        Serial.println("No valid vibration metrics available");
        return false;
    }

    String deviceId = awsGetDeviceId();
    String topic = telemetryGetTopic(deviceId.c_str());
    String payload = telemetryBuildPayload(metrics, deviceId.c_str());

    return awsPublish(topic.c_str(), payload.c_str());
}
