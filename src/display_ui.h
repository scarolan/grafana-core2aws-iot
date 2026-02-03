#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include "imu_sampler.h"

// Initialize the display
void displayInit();

// Draw the main status screen with all information
void displayDrawStatusScreen();

// Update the display with latest data
// Call this periodically from main loop
void displayUpdate();

// Set connection status for display
void displaySetWiFiStatus(bool connected);
void displaySetAWSStatus(bool connected);

// Set latest metrics for display
void displaySetMetrics(const VibrationMetrics& metrics);

#endif // DISPLAY_UI_H
