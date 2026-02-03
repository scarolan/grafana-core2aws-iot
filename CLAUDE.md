# CLAUDE.md - Project Context for Claude Code

## Project Summary

This is a vibration monitoring IoT demo using M5Stack Core2 AWS. The firmware is complete and tested. The device certificate has been extracted. **The project is ready for AWS backend setup and Grafana dashboard creation.**

## Current State

| Component | Status |
|-----------|--------|
| Firmware | Complete, compiles successfully |
| ATECC608 cert extraction | Done - cert is in `device.pem` and `secrets.h.example` |
| WiFi credentials | Need to be added to `secrets.h` |
| AWS IoT registration | **NOT DONE** - next step |
| Timestream database | NOT DONE |
| IoT Rule (MQTT → Timestream) | NOT DONE |
| Grafana dashboard | NOT DONE |

## Device Details

- **Device ID / Thing Name:** `012333B76CAC4C3701`
- **AWS Region:** `us-west-2`
- **IoT Endpoint:** `a2zey9c7ts6fdf-ats.iot.us-west-2.amazonaws.com`
- **MQTT Topic:** `dt/vibration/012333B76CAC4C3701/telemetry`

## Immediate Next Steps

### 1. Create secrets.h
```bash
cp src/secrets.h.example src/secrets.h
# Edit secrets.h - only need to change WIFI_SSID and WIFI_PASSWORD
```

### 2. Register Certificate with AWS IoT
```bash
aws iot register-certificate-without-ca \
  --certificate-pem file://device.pem \
  --status ACTIVE \
  --region us-west-2
```
Save the `certificateArn` from output.

### 3. Create Thing and Policy
```bash
# Create thing
aws iot create-thing --thing-name "012333B76CAC4C3701" --region us-west-2

# Create policy
aws iot create-policy --policy-name "VibrationMonitorPolicy" --region us-west-2 \
  --policy-document '{
    "Version": "2012-10-17",
    "Statement": [
      {"Effect": "Allow", "Action": "iot:Connect", "Resource": "*"},
      {"Effect": "Allow", "Action": "iot:Publish", "Resource": "*"}
    ]
  }'

# Attach (use cert ARN from step 2)
aws iot attach-policy --policy-name "VibrationMonitorPolicy" --target <CERT_ARN> --region us-west-2
aws iot attach-thing-principal --thing-name "012333B76CAC4C3701" --principal <CERT_ARN> --region us-west-2
```

### 4. Build and Flash
```bash
pio run -t upload --upload-port <COM_PORT>
pio device monitor --port <COM_PORT> --baud 115200
```

### 5. Verify in AWS Console
- IoT Core → MQTT test client → Subscribe to `dt/vibration/+/telemetry`
- Should see JSON every 5 seconds

### 6. Timestream + IoT Rule
See README.md for full commands to:
- Create VibrationDB database
- Create Telemetry table
- Create IAM role for IoT
- Create IoT Rule to route MQTT → Timestream

### 7. Grafana Cloud
- Install Amazon Timestream plugin
- Add data source with AWS credentials
- Create dashboard (queries in README.md)

## Key Technical Decisions

- **ArduinoECCX08 library:** Uses HarringayMakerSpace fork pinned to commit `9864c4c` because it has `begin(address)` support. The Core2 AWS uses I2C address `0x35` (not the default `0x60`).

- **IMU sampling:** FreeRTOS task pinned to Core 1, samples at 500Hz, computes RMS/peak over 1-second windows.

- **Certificate:** Self-signed cert generated from ATECC608's private key. The private key never leaves the chip - this provides hardware-backed proof that data came from this specific device.

## File Structure

```
src/
├── main.cpp           # Setup/loop, orchestrates everything
├── config.h           # Constants (pins, timing, addresses)
├── secrets.h          # WiFi + AWS creds (git-ignored, copy from .example)
├── wifi_manager.*     # WiFi connection + NTP time sync
├── aws_iot.*          # ATECC608 init, BearSSL, MQTT client
├── imu_sampler.*      # 500Hz sampling task, RMS/peak calculation
├── telemetry.*        # JSON payload builder
└── display_ui.*       # LCD status display
```

## Troubleshooting

If MQTT connection fails:
1. Check certificate is registered and ACTIVE in AWS IoT Console
2. Verify policy is attached to certificate
3. Verify thing is attached to certificate
4. Check endpoint matches your region

If no data in Timestream:
1. Check IoT Rule is enabled
2. Verify IAM role has timestream:WriteRecords permission
3. Test with MQTT test client first to confirm device is publishing

## Commands Reference

```bash
# Build
pio run

# Flash
pio run -t upload --upload-port COM10

# Monitor
pio device monitor --port COM10 --baud 115200

# Clean build
pio run -t clean
```
