# Vibration Detection System

## Overview

This project implements industrial-grade vibration monitoring using the M5Stack Core2 AWS's built-in MPU6886 IMU (Inertial Measurement Unit). The system samples 3-axis acceleration at 500Hz and computes RMS (Root Mean Square) and peak vibration metrics.

## What Are We Measuring?

**G-Forces (Acceleration)**

The MPU6886 provides 3-axis acceleration data in units of **g**, where:
- 1g = 9.81 m/s² (Earth's gravitational acceleration)
- The sensor measures acceleration along X, Y, and Z axes
- We compute the **magnitude**: `√(x² + y² + z²)` to get total acceleration regardless of orientation

## What is RMS?

**RMS = Root Mean Square**

RMS is a statistical measure that represents the **effective energy** of a varying signal. For vibration analysis, RMS is the industry standard because it captures the sustained vibrational energy, not just brief spikes.

### The Math

From `imu_sampler.cpp:76-97`:

```cpp
// 1. Sample at 500Hz for 1 second (500 samples)
for (int i = 0; i < 500; i++) {
    // 2. Calculate 3D acceleration magnitude
    float x = sampleBuf[i][0];
    float y = sampleBuf[i][1];
    float z = sampleBuf[i][2];
    float mag = sqrt(x*x + y*y + z*z);

    // 3. Square it and sum
    sumSq += mag * mag;

    // 4. Track peak
    if (mag > maxMag) {
        maxMag = mag;
    }
}

// 5. RMS = square root of the average of squared values
rms_g = sqrt(sumSq / IMU_WINDOW_SAMPLES);
peak_g = maxMag;
```

### Formula

```
RMS = √(1/N × Σ(magnitude²))
```

Where:
- N = number of samples (500)
- magnitude = √(x² + y² + z²) for each sample
- Σ = sum over all samples

## Why RMS Instead of Peak?

**RMS captures sustained vibrational energy, not just transient shocks.**

| Scenario | Peak Value | RMS Value | Interpretation |
|----------|-----------|-----------|----------------|
| Device dropped | ~10g | ~1.5g | Brief shock, low sustained energy |
| Continuous vibration | ~3g | ~2.5g | High sustained energy |
| Device at rest | ~1g | ~1g | Just gravity |
| Gentle shake | ~2g | ~1.3g | Moderate brief energy |

### Real-World Example

Imagine monitoring a motor:
- **Peak alone** would trigger false alarms every time someone walks past and causes a vibration spike
- **RMS** smooths out transients and reveals the true operational vibration level
- If RMS is rising over time, bearings may be wearing out even if peak values stay constant

## Color-Coded Severity Thresholds

From `display_ui.cpp:75-79`:

| Color | RMS Range | Meaning |
|-------|-----------|---------|
| 🟢 Green | < 1.0g | Normal operation, minimal vibration |
| 🟡 Yellow | 1.0 - 2.0g | Elevated vibration, worth monitoring |
| 🔴 Red | ≥ 2.0g | High vibration, potential machinery issue |

These thresholds are loosely based on **ISO 10816** standards for vibration severity in rotating machinery. In industrial predictive maintenance:
- **< 1g RMS**: Smooth operation, no action needed
- **1-2g RMS**: Acceptable but watch for upward trends
- **> 2g RMS**: Investigate for bearing wear, imbalance, misalignment, or looseness

## FreeRTOS Implementation

The vibration detection uses a dedicated **FreeRTOS task** for precise, high-frequency sampling without interference from WiFi, display updates, or MQTT publishing.

### Task Configuration

From `config.h`:
```cpp
#define IMU_SAMPLE_RATE_HZ   500       // Sample every 2ms
#define IMU_WINDOW_SAMPLES   500       // 1 second window
#define IMU_TASK_STACK_SIZE  4096      // 4KB stack
#define IMU_TASK_PRIORITY    5         // High priority
#define IMU_TASK_CORE        1         // Pin to Core 1
```

### Why FreeRTOS?

The ESP32 has **dual cores**:
- **Core 0**: Runs WiFi, BLE, and Arduino loop() by default
- **Core 1**: Available for application tasks

By pinning the IMU task to Core 1 with high priority:
- ✓ Sampling happens at precise 500Hz intervals
- ✓ No interference from WiFi radio interrupts
- ✓ No delays from MQTT/TLS operations
- ✓ Metrics computed in parallel with networking

### Task Creation

From `imu_sampler.cpp:18-45`:
```cpp
void imuStartSampling() {
    // Create mutex for thread-safe access
    metricsMutex = xSemaphoreCreateMutex();

    // Create IMU sampling task pinned to Core 1
    xTaskCreatePinnedToCore(
        imuTask,                    // Task function
        "imu_sampler",              // Name (for debugging)
        IMU_TASK_STACK_SIZE,        // Stack size
        nullptr,                    // Parameters
        IMU_TASK_PRIORITY,          // Priority
        nullptr,                    // Task handle
        IMU_TASK_CORE               // Pin to Core 1
    );
}
```

### Timing Precision

The task uses `vTaskDelayUntil()` for precise periodic execution:

```cpp
TickType_t lastWake = xTaskGetTickCount();
const TickType_t period = pdMS_TO_TICKS(1000 / 500);  // 2ms

while (true) {
    // Sample IMU
    if (M5.Imu.update()) {
        auto data = M5.Imu.getImuData();
        sampleBuf[sampleIdx][0] = data.accel.x;
        sampleBuf[sampleIdx][1] = data.accel.y;
        sampleBuf[sampleIdx][2] = data.accel.z;
        // ...
    }

    // Wait until next sample time (maintains 500Hz precisely)
    vTaskDelayUntil(&lastWake, period);
}
```

This ensures 500Hz sampling regardless of how long the IMU read takes.

## Data Flow

```
┌─────────────────────────────────────────────────────────────┐
│ Core 1: IMU Task (500Hz)                                    │
│                                                              │
│  MPU6886 ─→ Sample (x,y,z) ─→ 500 samples ─→ Compute RMS  │
│             every 2ms           1 sec window    & Peak      │
│                                                   ↓         │
│                                              [Mutex Lock]   │
│                                                   ↓         │
│                                          latestMetrics      │
└──────────────────────────────────────────────────┬──────────┘
                                                   │
┌──────────────────────────────────────────────────┴──────────┐
│ Core 0: Main Loop                                           │
│                                                              │
│  ┌─────────────────┐    ┌──────────────────┐              │
│  │ Display Update  │    │ MQTT Publish     │              │
│  │ (every 500ms)   │    │ (every 5 sec)    │              │
│  │                 │    │                  │              │
│  │ Read metrics ───┼────┼─→ [Mutex Lock]   │              │
│  │ Draw gauge      │    │   Read metrics   │              │
│  └─────────────────┘    │   Build JSON     │              │
│                         │   Publish to AWS │              │
│                         └──────────────────┘              │
└─────────────────────────────────────────────────────────────┘
```

## Published Data Format

Every 5 seconds, the device publishes to `dt/vibration/012333B76CAC4C3701/telemetry`:

```json
{
  "device_id": "012333B76CAC4C3701",
  "timestamp": 1738636800,
  "vibration": {
    "rms_g": 1.23,
    "peak_g": 2.45
  },
  "health": {
    "battery_v": 4.15,
    "temp_c": 28.5,
    "imu_temp_c": 31.2,
    "rssi_dbm": -65,
    "uptime_sec": 12345,
    "free_heap": 98304
  }
}
```

## Why This Matters for Industrial IoT

This implementation demonstrates key concepts for production industrial monitoring:

1. **High-frequency sampling (500Hz)** captures vibrations from fast-rotating machinery (up to 250Hz by Nyquist theorem)

2. **RMS computation** aligns with ISO standards used by industrial vibration analysts

3. **Dual-core architecture** ensures real-time sampling without network interference

4. **Thread-safe design** with mutexes prevents race conditions between sampling and publishing

5. **Hardware-backed security** with ATECC608 ensures data integrity and device authentication

6. **Time-series storage** in AWS Timestream enables trend analysis and predictive maintenance

## References

- **ISO 10816**: Mechanical vibration - Evaluation of machine vibration by measurements on non-rotating parts
- **Nyquist-Shannon Sampling Theorem**: Sample rate must be ≥ 2× highest frequency to capture
- **FreeRTOS Documentation**: https://www.freertos.org/
- **MPU6886 Datasheet**: 6-axis accelerometer + gyroscope specifications

## Code Locations

- `src/imu_sampler.cpp` - Sampling task and RMS computation
- `src/imu_sampler.h` - VibrationMetrics struct definition
- `src/display_ui.cpp` - Gauge visualization and color thresholds
- `src/config.h` - Sampling parameters and task configuration
