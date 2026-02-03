#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>

// Initialize WiFi and connect to configured network
// Returns true if connected successfully
bool wifiConnect();

// Check if WiFi is currently connected
bool wifiIsConnected();

// Get WiFi signal strength in dBm
int wifiGetRSSI();

// Reconnect if disconnected (call periodically from main loop)
void wifiMaintain();

#endif // WIFI_MANAGER_H
