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

// Sprite for needle (Lovyan technique - small sprite, rotate it!)
static LGFX_Sprite needle(&M5.Lcd);

// Gauge parameters
static const int GAUGE_CENTER_X = 160;
static const int GAUGE_CENTER_Y = 150;
static const int GAUGE_RADIUS = 90;
static float lastAngle = 180.0f;  // Track last needle position

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

    // Create tiny sprite for needle (Lovyan technique!)
    needle.setColorDepth(16);
    needle.createSprite(4, GAUGE_RADIUS - 25);
    needle.setPivot(2, GAUGE_RADIUS - 25);  // Pivot at bottom center

    // Draw needle shape on sprite
    needle.fillSprite(TFT_TRANSPARENT);
    needle.fillRect(0, 0, 4, needle.height(), TFT_WHITE);
    needle.fillCircle(2, 0, 3, TFT_WHITE);
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
    // Simple status indicators at bottom
    M5.Lcd.setFont(&fonts::FreeSans9pt7b);

    // WiFi indicator
    M5.Lcd.fillCircle(20, 225, 6, wifiConnected ? COLOR_OK : COLOR_ERROR);
    M5.Lcd.setTextColor(COLOR_DIM, COLOR_BG);
    M5.Lcd.drawString("WiFi", 30, 220);

    // AWS indicator - moved 80 pixels right
    M5.Lcd.fillCircle(130, 225, 6, awsConnected ? COLOR_OK : COLOR_ERROR);
    M5.Lcd.setTextColor(COLOR_DIM, COLOR_BG);
    M5.Lcd.drawString("AWS", 140, 220);
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

static void drawGaugeBackground() {
    // Draw static gauge background (only once!)
    const float maxG = 3.0f;

    // Title at very top, centered
    M5.Lcd.setFont(&fonts::FreeSansBold12pt7b);
    M5.Lcd.setTextColor(COLOR_HEADER, COLOR_BG);
    M5.Lcd.setTextDatum(TC_DATUM);
    M5.Lcd.drawString("VIBRATION RMS", 160, 5);
    M5.Lcd.setTextDatum(TL_DATUM);

    // Draw thick gauge background arcs
    for (int i = 0; i < 15; i++) {
        M5.Lcd.drawArc(GAUGE_CENTER_X, GAUGE_CENTER_Y, GAUGE_RADIUS-i, GAUGE_RADIUS-i-1, 180, 240, COLOR_OK);
        M5.Lcd.drawArc(GAUGE_CENTER_X, GAUGE_CENTER_Y, GAUGE_RADIUS-i, GAUGE_RADIUS-i-1, 240, 300, COLOR_WARN);
        M5.Lcd.drawArc(GAUGE_CENTER_X, GAUGE_CENTER_Y, GAUGE_RADIUS-i, GAUGE_RADIUS-i-1, 300, 360, COLOR_ERROR);
    }

    // Draw scale markings
    for (int i = 0; i <= 12; i++) {
        float angle = 180 + (i * 15);
        float rad = angle * PI / 180.0;
        int x1 = GAUGE_CENTER_X + (GAUGE_RADIUS - 16) * cos(rad);
        int y1 = GAUGE_CENTER_Y + (GAUGE_RADIUS - 16) * sin(rad);
        int x2 = GAUGE_CENTER_X + (GAUGE_RADIUS - 25) * cos(rad);
        int y2 = GAUGE_CENTER_Y + (GAUGE_RADIUS - 25) * sin(rad);
        M5.Lcd.drawLine(x1, y1, x2, y2, TFT_WHITE);
    }

    // Draw scale numbers (adjusted positions)
    M5.Lcd.setFont(&fonts::FreeSansBold12pt7b);
    M5.Lcd.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Lcd.drawString("0", GAUGE_CENTER_X - 95, GAUGE_CENTER_Y + 10);
    M5.Lcd.drawString("1", GAUGE_CENTER_X - 80, GAUGE_CENTER_Y - 50);  // Adjusted: left 5px, down 5px
    M5.Lcd.drawString("2", GAUGE_CENTER_X - 8, GAUGE_CENTER_Y - 90);   // Moved up 10px
    M5.Lcd.drawString("3", GAUGE_CENTER_X + 65, GAUGE_CENTER_Y - 55);
}

static void drawVibrationGauge() {
    const float maxG = 3.0f;

    // Erase old needle area - clear full needle length (65px) + tip radius
    M5.Lcd.fillCircle(GAUGE_CENTER_X, GAUGE_CENTER_Y, 70, COLOR_BG);

    if (currentMetrics.valid) {
        float rms = constrain(currentMetrics.rms_g, 0, maxG);
        float angle = 180 + (rms / maxG) * 180;  // 180° to 360°

        uint16_t needleColor = getRMSColor(rms);

        // Set pivot and draw rotated needle (Lovyan technique!)
        M5.Lcd.setPivot(GAUGE_CENTER_X, GAUGE_CENTER_Y);
        needle.setPaletteColor(1, needleColor);
        // Fine-tuned: +90° for sprite alignment, -30° to point at correct value
        needle.pushRotated(angle + 60);

        lastAngle = angle;

        // Center hub
        M5.Lcd.fillCircle(GAUGE_CENTER_X, GAUGE_CENTER_Y, 6, needleColor);

        // Value in center
        char buf[16];
        snprintf(buf, sizeof(buf), "%.2f", currentMetrics.rms_g);
        M5.Lcd.setFont(&fonts::FreeSansBold18pt7b);
        M5.Lcd.setTextColor(needleColor, COLOR_BG);
        M5.Lcd.drawString(buf, GAUGE_CENTER_X - 40, GAUGE_CENTER_Y + 20);

        M5.Lcd.setFont(&fonts::FreeSansBold12pt7b);
        M5.Lcd.setTextColor(COLOR_DIM, COLOR_BG);
        M5.Lcd.drawString("g", GAUGE_CENTER_X + 35, GAUGE_CENTER_Y + 30);
    } else {
        M5.Lcd.setFont(&fonts::FreeSansBold18pt7b);
        M5.Lcd.setTextColor(COLOR_DIM, COLOR_BG);
        M5.Lcd.drawString("--", GAUGE_CENTER_X - 25, GAUGE_CENTER_Y + 20);
    }
}

static void drawVibrationMetrics() {
    drawVibrationGauge();
}

static void drawDeviceInfo() {
    // Keep it minimal - just battery
    char buf[16];
    M5.Lcd.setFont(&fonts::FreeSans9pt7b);
    M5.Lcd.setTextColor(COLOR_DIM, COLOR_BG);

    float batteryV = M5.Power.getBatteryVoltage() / 1000.0f;
    snprintf(buf, sizeof(buf), "%.1fV", batteryV);
    M5.Lcd.drawString(buf, 270, 220);
}

void displayDrawStatusScreen() {
    // Draw static gauge background once
    drawGaugeBackground();

    // Draw dynamic parts
    drawVibrationMetrics();
    drawStatusBar();
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

    // Redraw only dynamic parts (needle rotates via pushRotated)
    drawVibrationMetrics();
    drawStatusBar();
    drawDeviceInfo();
}
