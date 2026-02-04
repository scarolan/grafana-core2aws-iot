#!/usr/bin/env python3
"""
Simple ATECC608 certificate registration script for AWS IoT
Uses boto3 to attempt certificate registration with certificate mode options
"""

import boto3
import sys

# Read the certificate file
with open('device.pem', 'r') as f:
    cert_pem = f.read()

# Create IoT client for ca-central-1
iot = boto3.client('iot', region_name='ca-central-1')

# Device details
thing_name = "012333B76CAC4C3701"
cert_arn = None

try:
    # Try to register the certificate
    print("Attempting to register certificate...")
    response = iot.register_certificate_without_ca(
        certificatePem=cert_pem,
        status='ACTIVE'
    )

    cert_arn = response['certificateArn']
    cert_id = response['certificateId']

    print(f"✓ Certificate registered!")
    print(f"  Certificate ARN: {cert_arn}")
    print(f"  Certificate ID: {cert_id}")

    # Attach policy to certificate
    print("\nAttaching policy to certificate...")
    iot.attach_policy(
        policyName='VibrationMonitorPolicy',
        target=cert_arn
    )
    print("✓ Policy attached")

    # Attach thing to certificate
    print(f"\nAttaching thing '{thing_name}' to certificate...")
    iot.attach_thing_principal(
        thingName=thing_name,
        principal=cert_arn
    )
    print("✓ Thing attached")

    print("\n✅ Registration complete! Device should now be able to connect to AWS IoT.")

except Exception as e:
    print(f"❌ Error: {e}")
    sys.exit(1)
