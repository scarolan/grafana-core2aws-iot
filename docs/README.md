# Documentation

This directory contains comprehensive documentation for the M5Stack Core2 AWS vibration monitoring project.

## Files

### [CLAUDE.md](CLAUDE.md)
Project context and instructions for Claude Code. Contains:
- Current project state and status
- Next steps for setup
- Device details and configuration
- Troubleshooting guide
- Command reference

### [ATECC608_ARCHITECTURE.md](ATECC608_ARCHITECTURE.md)
Deep dive into the ATECC608 secure element:
- Slot architecture and configuration
- Private key storage and operations
- Lock mechanism (one-way, permanent)
- Certificate generation process
- Security model comparison (hardware vs software)
- AWS IoT integration details

### [ATECC608_CERTIFICATE_SOLUTION.md](ATECC608_CERTIFICATE_SOLUTION.md)
Troubleshooting guide for certificate generation:
- Why the factory compressed certificate fails
- How to generate a properly formatted certificate
- Step-by-step AWS IoT registration
- Hardware security preservation
- Comparison of different approaches

### [VIBRATION_DETECTION.md](VIBRATION_DETECTION.md)
Technical explanation of vibration monitoring:
- What RMS (Root Mean Square) is and why it matters
- IMU sampling at 500Hz
- FreeRTOS task architecture
- Color-coded severity thresholds (ISO 10816)
- Dual-core implementation details
- Industrial IoT relevance

## Quick Links

- **Getting Started:** See main [README.md](../README.md)
- **Certificate Generation:** [extras/generate_cert/](../extras/generate_cert/)
- **AWS Scripts:** [aws/](../aws/)
- **Source Code:** [src/](../src/)
