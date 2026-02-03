#ifndef IMU_SAMPLER_H
#define IMU_SAMPLER_H

#include <Arduino.h>

// Vibration metrics computed from IMU samples
struct VibrationMetrics {
    float rms_g;       // Root mean square acceleration magnitude
    float peak_g;      // Peak acceleration magnitude
    float temp_c;      // IMU temperature (if available)
    uint32_t timestamp; // Timestamp when metrics were computed
    bool valid;        // True if metrics are valid
};

// Initialize and start the IMU sampling task
// Creates a FreeRTOS task pinned to Core 1
void imuStartSampling();

// Get the latest computed vibration metrics
// Returns true if valid metrics are available
bool imuGetLatestMetrics(VibrationMetrics& metrics);

// Get raw sample count (for debugging)
uint32_t imuGetSampleCount();

#endif // IMU_SAMPLER_H
