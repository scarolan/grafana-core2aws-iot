# CLAUDE.md - Project Context for Claude Code

## Project Summary

This is a **complete** vibration monitoring IoT demo using M5Stack Core2 AWS. The firmware is tested and operational. Device is publishing telemetry to AWS IoT Core and data is flowing to Timestream.

## Current State

| Component | Status |
|-----------|--------|
| Firmware | ✅ Complete - dramatic vibration gauge with LovyanGFX |
| ATECC608 cert generation | ✅ Complete - working certificate in `extras/certificates/` |
| WiFi credentials | ✅ Configured in `src/secrets.h` |
| AWS IoT registration | ✅ Complete - Thing, Policy, Certificate attached |
| Timestream database | ✅ Complete - VibrationDB with Telemetry table |
| IoT Rule (MQTT → Timestream) | ✅ Complete - Direct Timestream action (no Lambda) |
| Grafana dashboard | ⏳ **TODO** - See GitHub issue #1 |
| Documentation | ✅ Complete - Architecture, vibration, certificate docs |

## Device Details

- **Device ID / Thing Name:** `012333B76CAC4C3701`
- **AWS Region:** `us-east-1`
- **IoT Endpoint:** `afujw4lyol38p-ats.iot.us-east-1.amazonaws.com`
- **MQTT Topic:** `dt/vibration/012333B76CAC4C3701/telemetry`
- **Certificate ARN:** `arn:aws:iot:us-east-1:494614287886:cert/91da4e92657a24f3928e5642d382fc4e6f5bfdfca16a9d98be4e407dc0181689`

## What Works

✅ **Device Firmware:**
- 500Hz IMU sampling on dedicated FreeRTOS task (Core 1)
- RMS and peak vibration calculation over 1-second windows
- Dramatic gauge display with colored needle (green/yellow/red zones)
- MQTT publishing every 5 seconds to AWS IoT Core
- Hardware-backed TLS authentication with ATECC608

✅ **AWS Infrastructure:**
- Device registered with AWS IoT Core in us-east-1
- Thing, Certificate, and Policy properly attached
- IoT Rule routing telemetry directly to Timestream
- Timestream database storing vibration metrics

✅ **Documentation:**
- `ATECC608_ARCHITECTURE.md` - Complete secure element explanation
- `ATECC608_CERTIFICATE_SOLUTION.md` - Certificate generation guide
- `VIBRATION_DETECTION.md` - RMS and FreeRTOS implementation details
- `README.md` - Full setup and usage guide

## Remaining Work

⏳ **Grafana Dashboard** (GitHub issue #1):
- Install Amazon Timestream plugin in Grafana Cloud
- Add Timestream data source
- Create panels for RMS, peak, battery, temperature
- Set up alerting for high vibration / device offline

See: https://github.com/your-repo/issues/1

## Key Technical Decisions

- **ArduinoECCX08 library:** Uses HarringayMakerSpace fork pinned to commit `9864c4c` because it has `begin(address)` support. The Core2 AWS uses I2C address `0x35` (not the default `0x60`).

- **IMU sampling:** FreeRTOS task pinned to Core 1, samples at 500Hz, computes RMS/peak over 1-second windows.

- **Certificate:** Self-signed cert generated from ATECC608's private key. The private key never leaves the chip - this provides hardware-backed proof that data came from this specific device.

## Repository Structure

```
grafana-core2aws-iot/
├── README.md                    # Main documentation
├── platformio.ini               # Build configuration
├── src/                         # Firmware source
│   ├── main.cpp                 # Setup/loop, orchestrates everything
│   ├── config.h                 # Constants (pins, timing, addresses)
│   ├── secrets.h                # WiFi + AWS creds (git-ignored)
│   ├── secrets.h.example        # Template for secrets
│   ├── wifi_manager.*           # WiFi connection + NTP time sync
│   ├── aws_iot.*                # ATECC608 init, BearSSL, MQTT client
│   ├── imu_sampler.*            # 500Hz sampling task, RMS/peak calculation
│   ├── telemetry.*              # JSON payload builder
│   └── display_ui.*             # Vibration gauge with LovyanGFX
├── docs/                        # Documentation
│   ├── CLAUDE.md                # This file (project context)
│   ├── ATECC608_ARCHITECTURE.md # Secure element deep dive
│   ├── ATECC608_CERTIFICATE_SOLUTION.md  # Certificate guide
│   └── VIBRATION_DETECTION.md   # RMS and FreeRTOS explanation
├── extras/                      # Additional tools
│   ├── extract_cert/            # Certificate extraction sketch (old)
│   ├── generate_cert/           # Certificate generator sketch (current)
│   └── certificates/            # Device certificates
│       └── device_new.pem       # Working certificate for AWS
└── aws/                         # AWS helper scripts
    ├── register_cert.py         # Certificate registration
    ├── registration_helper.py   # Advanced registration (experimental)
    └── timestream_writer.py     # Lambda function (not used - using IoT Rule)
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
