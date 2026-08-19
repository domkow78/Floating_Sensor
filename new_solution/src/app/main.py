"""Entry point for the new IoT Core MVP."""

from config.settings import DEVICE_ID, INFLUX_HOST, INFLUX_PORT, MQTT_BROKER_HOST, MQTT_BROKER_PORT
from mqtt.client import MqttClient
from processing.engine import normalize_payload
from processing.models import TelemetryRecord
from registry.device_registry import DeviceRegistry
from storage.influx_client import InfluxClient


def build_sample_payload() -> dict:
    return {
        "device_id": DEVICE_ID,
        "timestamp": "2026-08-05T10:15:00Z",
        "temperature": 22.4,
        "humidity": 48.2,
        "pressure": 1008.4,
    }


def run_pipeline(
    payload: dict,
    mqtt_client: MqttClient | None = None,
    influx_client: InfluxClient | None = None,
    device_registry: DeviceRegistry | None = None,
) -> TelemetryRecord:
    mqtt_client = mqtt_client or MqttClient(MQTT_BROKER_HOST, MQTT_BROKER_PORT)
    influx_client = influx_client or InfluxClient(INFLUX_HOST, INFLUX_PORT)
    device_registry = device_registry or DeviceRegistry()

    telemetry_record = normalize_payload(payload)
    mqtt_client.publish_telemetry(telemetry_record.to_dict())
    influx_client.write(telemetry_record)
    device_registry.update_telemetry(telemetry_record)
    return telemetry_record


def main() -> None:
    payload = build_sample_payload()
    telemetry_record = run_pipeline(payload)
    print(f"Floating Sensor IoT Core MVP processed payload for {telemetry_record.device_id}")


if __name__ == "__main__":
    main()
