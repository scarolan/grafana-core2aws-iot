#include "imu_sampler.h"
#include "config.h"
#include <M5Unified.h>

// Sample buffer for one window
static float sampleBuf[IMU_WINDOW_SAMPLES][3];
static volatile int sampleIdx = 0;
static volatile uint32_t totalSamples = 0;

// Latest computed metrics
static VibrationMetrics latestMetrics = {0, 0, 0, 0, false};
static SemaphoreHandle_t metricsMutex = nullptr;

// Forward declarations
static void imuTask(void* param);
static void computeMetrics();

void imuStartSampling() {
    // Create mutex for thread-safe metrics access
    metricsMutex = xSemaphoreCreateMutex();

    if (metricsMutex == nullptr) {
        Serial.println("ERROR: Failed to create metrics mutex");
        return;
    }

    // Create IMU sampling task pinned to Core 1
    BaseType_t result = xTaskCreatePinnedToCore(
        imuTask,
        "imu_sampler",
        IMU_TASK_STACK_SIZE,
        nullptr,
        IMU_TASK_PRIORITY,
        nullptr,
        IMU_TASK_CORE
    );

    if (result != pdPASS) {
        Serial.println("ERROR: Failed to create IMU task");
        return;
    }

    Serial.printf("IMU sampling started: %d Hz, %d sample window\n",
                  IMU_SAMPLE_RATE_HZ, IMU_WINDOW_SAMPLES);
}

static void imuTask(void* param) {
    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(1000 / IMU_SAMPLE_RATE_HZ);

    while (true) {
        // Update IMU and check for new data
        if (M5.Imu.update()) {
            auto data = M5.Imu.getImuData();

            // Store acceleration values (in g)
            sampleBuf[sampleIdx][0] = data.accel.x;
            sampleBuf[sampleIdx][1] = data.accel.y;
            sampleBuf[sampleIdx][2] = data.accel.z;

            sampleIdx++;
            totalSamples++;

            // When window is full, compute metrics
            if (sampleIdx >= IMU_WINDOW_SAMPLES) {
                computeMetrics();
                sampleIdx = 0;
            }
        }

        // Maintain precise timing
        vTaskDelayUntil(&lastWake, period);
    }
}

static void computeMetrics() {
    float sumSq = 0.0f;
    float maxMag = 0.0f;

    // Calculate RMS and peak from the sample window
    for (int i = 0; i < IMU_WINDOW_SAMPLES; i++) {
        // Compute magnitude: sqrt(x^2 + y^2 + z^2)
        float x = sampleBuf[i][0];
        float y = sampleBuf[i][1];
        float z = sampleBuf[i][2];
        float mag = sqrtf(x*x + y*y + z*z);

        sumSq += mag * mag;

        if (mag > maxMag) {
            maxMag = mag;
        }
    }

    // Update metrics with mutex protection
    if (xSemaphoreTake(metricsMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        latestMetrics.rms_g = sqrtf(sumSq / IMU_WINDOW_SAMPLES);
        latestMetrics.peak_g = maxMag;
        latestMetrics.timestamp = millis();
        latestMetrics.valid = true;

        // Try to get IMU temperature if available
        float temp = 0;
        if (M5.Imu.getTemp(&temp)) {
            latestMetrics.temp_c = temp;
        }

        xSemaphoreGive(metricsMutex);
    }
}

bool imuGetLatestMetrics(VibrationMetrics& metrics) {
    if (metricsMutex == nullptr) {
        return false;
    }

    bool success = false;

    if (xSemaphoreTake(metricsMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (latestMetrics.valid) {
            metrics = latestMetrics;
            success = true;
        }
        xSemaphoreGive(metricsMutex);
    }

    return success;
}

uint32_t imuGetSampleCount() {
    return totalSamples;
}
