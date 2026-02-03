#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Arduino.h>
#include "imu_sampler.h"

// Build JSON telemetry payload from vibration metrics
// Includes device health data (battery, temperature, RSSI, uptime)
String telemetryBuildPayload(const VibrationMetrics& vib, const char* deviceId);

// Publish telemetry to AWS IoT
// Returns true if published successfully
bool telemetryPublish();

// Get the topic string for telemetry
String telemetryGetTopic(const char* deviceId);

#endif // TELEMETRY_H
