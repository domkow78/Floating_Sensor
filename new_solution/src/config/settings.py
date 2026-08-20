"""Application settings for the MVP."""

MQTT_BROKER_HOST = "localhost"
MQTT_BROKER_PORT = 1883

INFLUX_HOST = "localhost"
INFLUX_PORT = 8086
INFLUX_URL = f"http://{INFLUX_HOST}:{INFLUX_PORT}"
INFLUX_TOKEN = "fs-dev-token"
INFLUX_ORG = "floating_sensor"
INFLUX_BUCKET = "floating_sensor"

DEVICE_ID = "FS-001"
