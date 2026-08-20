"""Application settings for the MVP."""

import os

MQTT_BROKER_HOST = os.getenv("MQTT_BROKER_HOST", "localhost")
MQTT_BROKER_PORT = int(os.getenv("MQTT_BROKER_PORT", "1883"))

INFLUX_HOST     = os.getenv("INFLUX_HOST", "localhost")
INFLUX_PORT     = int(os.getenv("INFLUX_PORT", "8086"))
INFLUX_URL      = os.getenv("INFLUX_URL", f"http://{INFLUX_HOST}:{INFLUX_PORT}")
INFLUX_TOKEN    = os.getenv("INFLUX_TOKEN", "fs-dev-token")
INFLUX_ORG     = os.getenv("INFLUX_ORG", "floating_sensor")
INFLUX_BUCKET   = os.getenv("INFLUX_BUCKET", "floating_sensor")

DEVICE_ID = os.getenv("DEVICE_ID", "FS-001")
API_BASE  = os.getenv("API_BASE", "http://localhost:8000/api/v1")
