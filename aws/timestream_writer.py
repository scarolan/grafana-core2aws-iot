import json
import boto3
import os
from datetime import datetime

timestream = boto3.client('timestream-write', region_name='us-east-1')

DATABASE_NAME = 'VibrationDB'
TABLE_NAME = 'Telemetry'

def lambda_handler(event, context):
    """Write IoT telemetry to Timestream"""
    
    try:
        # Extract data from MQTT payload
        device_id = event.get('device_id')
        timestamp = event.get('timestamp', int(datetime.now().timestamp()))
        vibration = event.get('vibration', {})
        health = event.get('health', {})
        
        # Prepare dimensions (indexed columns)
        dimensions = [
            {'Name': 'device_id', 'Value': str(device_id)}
        ]
        
        # Prepare measures (time-series values)
        records = []
        
        # Vibration measures
        if vibration.get('rms_g') is not None:
            records.append({
                'MeasureName': 'rms_g',
                'MeasureValue': str(vibration['rms_g']),
                'MeasureValueType': 'DOUBLE',
                'Time': str(timestamp),
                'TimeUnit': 'SECONDS',
                'Dimensions': dimensions
            })
        
        if vibration.get('peak_g') is not None:
            records.append({
                'MeasureName': 'peak_g',
                'MeasureValue': str(vibration['peak_g']),
                'MeasureValueType': 'DOUBLE',
                'Time': str(timestamp),
                'TimeUnit': 'SECONDS',
                'Dimensions': dimensions
            })
        
        # Health measures
        for measure_name in ['battery_v', 'temp_c', 'imu_temp_c']:
            if health.get(measure_name) is not None:
                records.append({
                    'MeasureName': measure_name,
                    'MeasureValue': str(health[measure_name]),
                    'MeasureValueType': 'DOUBLE',
                    'Time': str(timestamp),
                    'TimeUnit': 'SECONDS',
                    'Dimensions': dimensions
                })
        
        for measure_name in ['rssi_dbm', 'uptime_sec', 'free_heap']:
            if health.get(measure_name) is not None:
                records.append({
                    'MeasureName': measure_name,
                    'MeasureValue': str(health[measure_name]),
                    'MeasureValueType': 'BIGINT',
                    'Time': str(timestamp),
                    'TimeUnit': 'SECONDS',
                    'Dimensions': dimensions
                })
        
        # Write to Timestream
        if records:
            result = timestream.write_records(
                DatabaseName=DATABASE_NAME,
                TableName=TABLE_NAME,
                Records=records
            )
            print(f"Successfully wrote {len(records)} records to Timestream")
            return {
                'statusCode': 200,
                'body': json.dumps(f'Wrote {len(records)} records')
            }
        else:
            print("No valid records to write")
            return {
                'statusCode': 400,
                'body': json.dumps('No valid records')
            }
            
    except Exception as e:
        print(f"Error: {str(e)}")
        raise

