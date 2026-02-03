#ifndef AWS_IOT_H
#define AWS_IOT_H

#include <Arduino.h>

// Initialize the ATECC608 secure element
// Returns true if initialization successful
bool awsInitSecureElement();

// Get the device ID (ATECC608 serial number)
// Used as Thing name and MQTT client ID
String awsGetDeviceId();

// Connect to AWS IoT Core via MQTTS
// Must call awsInitSecureElement() first
// Returns true if connected
bool awsConnect();

// Check if currently connected to AWS IoT
bool awsIsConnected();

// Publish a message to a topic
// Returns true if published successfully
bool awsPublish(const char* topic, const char* payload);

// Maintain MQTT connection (call periodically from main loop)
void awsMaintain();

// Get time from WiFi/NTP (used by BearSSL for cert validation)
unsigned long awsGetTime();

#endif // AWS_IOT_H
