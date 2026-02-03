#include "display_ui.h"
#include "config.h"
#include "aws_iot.h"
#include <M5Unified.h>
#include <WiFi.h>

// Display state
static bool wifiConnected = false;
static bool awsConnected = false;
static VibrationMetrics currentMetrics = {0, 0, 0, 0, false};
static uint32_t lastPublishTime = 0;
static uint32_t publishCount = 0;

// Colors
#define COLOR_BG        TFT_BLACK
#define COLOR_HEADER    TFT_CYAN
#define COLOR_OK        TFT_GREEN
#define COLOR_WARN      TFT_YELLOW
#define COLOR_ERROR     TFT_RED
#define COLOR_TEXT      TFT_WHITE
#define COLOR_DIM       TFT_DARKGREY

void displayInit() {
    M5.Lcd.fillScreen(COLOR_BG);
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.setTextDatum(TL_DATUM);
}

void displaySetWiFiStatus(bool connected) {
    wifiConnected = connected;
}

void displaySetAWSStatus(bool connected) {
    awsConnected = connected;
}

void displaySetMetrics(const VibrationMetrics& metrics) {
    currentMetrics = metrics;
}

static void drawStatusBar() {
    // WiFi status
    M5.Lcd.setTextColor(wifiConnected ? COLOR_OK : COLOR_ERROR, COLOR_BG);
    M5.Lcd.drawString(wifiConnected ? "WiFi:OK " : "WiFi:-- ", 10, 50);

    // AWS status
    M5.Lcd.setTextColor(awsConnected ? COLOR_OK : COLOR_ERROR, COLOR_BG);
    M5.Lcd.drawString(awsConnected ? "AWS:OK" : "AWS:--", 120, 50);

    // RSSI if connected
    if (wifiConnected) {
        M5.Lcd.setTextColor(COLOR_DIM, COLOR_BG);
        char rssi[16];
        snprintf(rssi, sizeof(rssi), "%ddBm", WiFi.RSSI());
        M5.Lcd.drawString(rssi, 220, 50);
    }
}

static uint16_t getRMSColor(float rms) {
    // Green < 1g, Yellow < 2g, Red >= 2g
    if (rms < 1.0f) return COLOR_OK;
    if (rms < 2.0f) return COLOR_WARN;
    return COLOR_ERROR;
}

static uint16_t getPeakColor(float peak) {
    // Green < 1.5g, Yellow < 3g, Red >= 3g
    if (peak < 1.5f) return COLOR_OK;
    if (peak < 3.0f) return COLOR_WARN;
    return COLOR_ERROR;
}

static void drawVibrationMetrics() {
    char buf[64];

    // RMS acceleration
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.drawString("RMS:", 10, 100);

    if (currentMetrics.valid) {
        snprintf(buf, sizeof(buf), "%.3f g  ", currentMetrics.rms_g);
        M5.Lcd.setTextColor(getRMSColor(currentMetrics.rms_g), COLOR_BG);
    } else {
        snprintf(buf, sizeof(buf), "---     ");
        M5.Lcd.setTextColor(COLOR_DIM, COLOR_BG);
    }
    M5.Lcd.drawString(buf, 80, 100);

    // Peak acceleration
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.drawString("Peak:", 10, 130);

    if (currentMetrics.valid) {
        snprintf(buf, sizeof(buf), "%.3f g  ", currentMetrics.peak_g);
        M5.Lcd.setTextColor(getPeakColor(currentMetrics.peak_g), COLOR_BG);
    } else {
        snprintf(buf, sizeof(buf), "---     ");
        M5.Lcd.setTextColor(COLOR_DIM, COLOR_BG);
    }
    M5.Lcd.drawString(buf, 80, 130);
}

static void drawDeviceInfo() {
    char buf[64];
    M5.Lcd.setTextColor(COLOR_DIM, COLOR_BG);

    // Battery voltage
    float batteryV = M5.Power.getBatteryVoltage() / 1000.0f;
    snprintf(buf, sizeof(buf), "Batt: %.2fV", batteryV);
    M5.Lcd.drawString(buf, 10, 180);

    // Temperature from AXP192
    float temp = M5.Power.Axp192.getInternalTemperature();
    snprintf(buf, sizeof(buf), "Temp: %.1fC", temp);
    M5.Lcd.drawString(buf, 140, 180);

    // Uptime
    uint32_t uptimeSec = millis() / 1000;
    uint32_t hours = uptimeSec / 3600;
    uint32_t mins = (uptimeSec % 3600) / 60;
    uint32_t secs = uptimeSec % 60;
    snprintf(buf, sizeof(buf), "Up: %02lu:%02lu:%02lu", hours, mins, secs);
    M5.Lcd.drawString(buf, 10, 205);

    // Free heap
    snprintf(buf, sizeof(buf), "Heap: %luK", ESP.getFreeHeap() / 1024);
    M5.Lcd.drawString(buf, 140, 205);
}

void displayDrawStatusScreen() {
    M5.Lcd.fillScreen(COLOR_BG);

    // Header
    M5.Lcd.setTextColor(COLOR_HEADER, COLOR_BG);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setFont(&fonts::FreeSansBold12pt7b);
    M5.Lcd.drawString("Vibration Monitor", 10, 10);

    // Reset font for rest of display
    M5.Lcd.setFont(&fonts::FreeSans9pt7b);

    // Device ID
    M5.Lcd.setTextColor(COLOR_DIM, COLOR_BG);
    String deviceId = awsGetDeviceId();
    if (deviceId.length() > 0) {
        M5.Lcd.drawString(deviceId.c_str(), 220, 15);
    }

    drawStatusBar();
    drawVibrationMetrics();
    drawDeviceInfo();
}

void displayUpdate() {
    // Update connection status
    displaySetWiFiStatus(WiFi.status() == WL_CONNECTED);
    displaySetAWSStatus(awsIsConnected());

    // Get latest metrics
    VibrationMetrics metrics;
    if (imuGetLatestMetrics(metrics)) {
        displaySetMetrics(metrics);
    }

    // Redraw screen
    drawStatusBar();
    drawVibrationMetrics();
    drawDeviceInfo();
}
