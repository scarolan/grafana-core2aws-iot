#ifndef CONFIG_H
#define CONFIG_H

// I2C Configuration
#define I2C_SDA_PIN       21
#define I2C_SCL_PIN       22
#define I2C_FREQUENCY     100000
#define ATECC608_ADDRESS  0x35

// ATECC608 Slot Configuration
#define PRIVATE_KEY_SLOT  0

// IMU Sampling Configuration
#define IMU_SAMPLE_RATE_HZ   500
#define IMU_WINDOW_SAMPLES   500   // 1 second window at 500Hz
#define IMU_TASK_STACK_SIZE  4096
#define IMU_TASK_PRIORITY    5
#define IMU_TASK_CORE        1

// Telemetry Configuration
#define TELEMETRY_INTERVAL_MS  5000  // Publish every 5 seconds
#define MQTT_PORT              8883

// WiFi Configuration
#define WIFI_CONNECT_TIMEOUT_MS  30000
#define WIFI_RETRY_DELAY_MS      5000

// Display Configuration
#define DISPLAY_UPDATE_INTERVAL_MS  500

// MQTT Topic Prefix
#define MQTT_TOPIC_PREFIX  "dt/vibration/"

#endif // CONFIG_H
