#include "wifi_manager.h"
#include "config.h"
#include "secrets.h"
#include <M5Unified.h>

static unsigned long lastReconnectAttempt = 0;

bool wifiConnect() {
    Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > WIFI_CONNECT_TIMEOUT_MS) {
            Serial.println("WiFi connection timeout");
            return false;
        }
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.printf("WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());

    // Configure NTP for certificate validation
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    // Wait for time to sync
    Serial.print("Waiting for NTP time sync");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println();
    Serial.printf("Current time: %s", ctime(&now));

    return true;
}

bool wifiIsConnected() {
    return WiFi.status() == WL_CONNECTED;
}

int wifiGetRSSI() {
    if (wifiIsConnected()) {
        return WiFi.RSSI();
    }
    return 0;
}

void wifiMaintain() {
    if (WiFi.status() != WL_CONNECTED) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > WIFI_RETRY_DELAY_MS) {
            lastReconnectAttempt = now;
            Serial.println("WiFi disconnected, reconnecting...");
            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        }
    }
}
