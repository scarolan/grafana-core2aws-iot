# Grafana Dashboard

This directory contains the production Grafana Cloud dashboard for the M5Stack Core2 AWS vibration monitoring system.

## Dashboard Files

**`core2_iot_vibration_dashboard.yaml`** - Dashboard in YAML format (recommended for Grafana Cloud)

**`core2_iot_vibration_dashboard.json`** - Dashboard in JSON format (alternative format)

Both files contain the same dashboard with all panels configured and tested. Use whichever format works best with your Grafana version.

## What's Included

The dashboard monitors device `012333B76CAC4C3701` and displays:

### Time Series Panels (Full Width)
1. **Vibration RMS** - Root mean square acceleration over 1-second windows
   - Thresholds: Green < 1g, Yellow 1-2g, Red > 2g
   - Shows sustained vibration energy

2. **Peak Acceleration** - Maximum instantaneous acceleration spikes
   - Thresholds: Green < 1.5g, Yellow 1.5-3g, Red > 3g
   - Detects transient shocks

### Status Panels (Bottom Row)
3. **Battery Voltage** (Gauge) - Current LiPo battery level
   - Thresholds: Red < 3.5V, Yellow 3.5-3.8V, Green > 3.8V

4. **Device Temperature** (Gauge) - AXP192 PMIC temperature
   - Thresholds: Green < 40°C, Yellow 40-50°C, Red > 50°C

5. **WiFi Signal** (Stat) - RSSI signal strength
   - Thresholds: Red < -80dBm, Yellow -80 to -60dBm, Green > -60dBm

6. **Uptime** (Stat) - Device uptime in hours
   - Shows continuous operation time

## Prerequisites

### 1. Timestream Data Source

You must have the **Amazon Timestream** data source configured in Grafana Cloud:

**Configuration:**
- **Name:** `VibrationDB` (or match your datasource name)
- **Region:** `us-east-1`
- **Database:** `VibrationDB`
- **Table:** `Telemetry`

**IAM Permissions Required:**
```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "timestream:DescribeEndpoints",
        "timestream:Select",
        "timestream:SelectValues",
        "timestream:DescribeTable",
        "timestream:ListTables",
        "timestream:DescribeDatabase",
        "timestream:ListDatabases"
      ],
      "Resource": "*"
    }
  ]
}
```

See [main README](../README.md#grafana-cloud-setup) for full setup instructions.

### 2. Data Flowing from Device

The device must be:
- Connected to WiFi
- Publishing to AWS IoT Core
- IoT Rule routing data to Timestream

Verify data exists:
```sql
SELECT COUNT(*) as total_records
FROM "VibrationDB"."Telemetry"
WHERE device_id = '012333B76CAC4C3701'
```

## How to Import

### Method 1: Via Grafana UI

1. **Login to Grafana Cloud**
2. Go to **Dashboards** → **New** → **Import**
3. Click **Upload JSON/YAML file**
4. Select `core2_iot_vibration_dashboard.yaml` (or `.json`)
5. **Select Timestream data source** from dropdown
6. Click **Import**

**Note:** Both YAML and JSON formats work - use whichever Grafana accepts.

### Method 2: Via Grafana API (Advanced)

```bash
# Get your Grafana API key from grafana.com
export GRAFANA_API_KEY="your-api-key"
export GRAFANA_URL="https://your-org.grafana.net"

# Import dashboard
curl -X POST \
  -H "Authorization: Bearer $GRAFANA_API_KEY" \
  -H "Content-Type: application/json" \
  -d @core2_iot_vibration_dashboard.yaml \
  "$GRAFANA_URL/api/dashboards/db"
```

## Dashboard Settings

**Refresh Rate:** 5 seconds (matches device publish interval)

**Time Range:** Last 6 hours (configurable via time picker)

**Auto-refresh:** Enabled - dashboard updates every 5 seconds automatically

## Customization

### Change Device ID

To monitor a different device, edit queries in each panel:

Change:
```sql
WHERE device_id = '012333B76CAC4C3701'
```

To:
```sql
WHERE device_id = 'YOUR_DEVICE_ID'
```

### Adjust Thresholds

Edit any panel → **Thresholds** section:

**RMS Thresholds** (based on ISO 10816):
- 0-1g: Normal operation
- 1-2g: Elevated, monitor trends
- 2g+: High vibration, investigate

**Battery Thresholds:**
- < 3.5V: Critical, charge soon
- 3.5-3.8V: Low, charging recommended
- > 3.8V: Good

### Add Alerts

1. Edit a panel (e.g., Vibration RMS)
2. Go to **Alert** tab
3. Create alert rule:
   - **Condition:** `last() > 2.0 for 1m`
   - **Contact point:** Email, Slack, Discord, etc.

Example alert rules:
- **High Vibration:** RMS > 2.0g for 1 minute
- **Device Offline:** No data received for 5 minutes
- **Low Battery:** Battery < 3.5V

## Query Format

All queries use Timestream SQL with Grafana macros:

```sql
SELECT
  time,
  measure_value::double as value
FROM "VibrationDB"."Telemetry"
WHERE measure_name = 'rms_g'
  AND device_id = '012333B76CAC4C3701'
  AND time BETWEEN from_milliseconds($__from) AND from_milliseconds($__to)
ORDER BY time ASC
```

**Key points:**
- `$__from` and `$__to` are Grafana time range variables
- `from_milliseconds()` converts Grafana timestamps to Timestream format
- `measure_value::double` casts to correct type (use `::bigint` for integers)

## Troubleshooting

### "No data" in panels

**Check device is publishing:**
```bash
# AWS IoT MQTT test client
aws iot-data publish \
  --topic "dt/vibration/012333B76CAC4C3701/telemetry" \
  --cli-binary-format raw-in-base64-out \
  --payload '{"test": true}'
```

**Verify Timestream has data:**
```sql
SELECT *
FROM "VibrationDB"."Telemetry"
WHERE device_id = '012333B76CAC4C3701'
ORDER BY time DESC
LIMIT 10
```

### "Query failed" errors

- Verify Timestream data source is connected
- Check IAM permissions include `timestream:Select`
- Confirm database/table names match (case-sensitive!)

### Incorrect values

- Check `measure_name` in query matches Timestream column
- Verify data type casting (`::double` vs `::bigint`)
- Confirm device_id is correct

## Related Documentation

- [Main README](../README.md) - Full project documentation
- [AWS Setup](../README.md#aws-backend-setup-timestream) - Timestream configuration
- [Device Firmware](../src/) - Telemetry payload format
- [VIBRATION_DETECTION.md](../docs/VIBRATION_DETECTION.md) - RMS explanation

## Demo Tips

For a live demo:
1. Set refresh to 5s
2. Set time range to "Last 15 minutes"
3. Shake the device and watch RMS spike in real-time! 🤚
4. Show color-coded thresholds (green → yellow → red)
5. Point out gauge panels updating every 5 seconds

The dashboard is production-ready and suitable for webinar/conference demos! 🎉
