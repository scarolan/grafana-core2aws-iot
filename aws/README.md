# AWS Scripts

This directory contains helper scripts for AWS IoT Core and Timestream integration.

## Files

### [register_cert.py](register_cert.py)
Simple Python script to register device certificate with AWS IoT Core.

**Usage:**
```bash
python register_cert.py device.pem
```

### [registration_helper.py](registration_helper.py)
Advanced registration helper using M5Stack's manifest-based approach. This was an experimental approach that uses the compressed certificate format with JWS signing.

**Note:** We ended up using the simpler `register-certificate-without-ca` approach instead. See [../docs/ATECC608_CERTIFICATE_SOLUTION.md](../docs/ATECC608_CERTIFICATE_SOLUTION.md) for details.

### [timestream_writer.py](timestream_writer.py)
AWS Lambda function that writes IoT telemetry to Timestream.

**Note:** This Lambda approach was replaced with a direct IoT Rule → Timestream integration (no Lambda needed).

**Original purpose:**
- Parse MQTT messages from IoT Core
- Write records to Timestream database
- Handle different data types (DOUBLE, BIGINT)

**Current setup:**
Instead of using this Lambda, we use an IoT Rule with a Timestream action:
```bash
aws iot create-topic-rule \
  --rule-name VibrationToTimestream \
  --topic-rule-payload '{
    "sql": "SELECT * FROM '\''dt/vibration/+/telemetry'\''",
    "actions": [{
      "timestream": {
        "roleArn": "arn:aws:iam::494614287886:role/IoTTimestreamRole",
        "databaseName": "VibrationDB",
        "tableName": "Telemetry",
        "dimensions": [{"name": "device_id", "value": "${device_id}"}]
      }
    }]
  }'
```

See main [README.md](../README.md) for complete setup instructions.

## Prerequisites

All scripts require:
- AWS CLI configured with credentials
- Appropriate IAM permissions
- Python 3.7+ with boto3 installed

```bash
pip install boto3
```

## Related Documentation

- [AWS IoT Core Documentation](https://docs.aws.amazon.com/iot/)
- [AWS Timestream Documentation](https://docs.aws.amazon.com/timestream/)
- [IoT Rules Actions](https://docs.aws.amazon.com/iot/latest/developerguide/iot-rule-actions.html)
