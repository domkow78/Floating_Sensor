"""Smoke tests for the initial MVP structure."""

from mqtt.client import MqttClient
from processing.engine import normalize_payload


class DummyMessage:
    rc = 0


class DummyMqttClient:
    def __init__(self):
        self.last_topic = None
        self.last_payload = None

    def publish(self, topic: str, payload: str, qos: int = 1):
        self.last_topic = topic
        self.last_payload = payload
        return DummyMessage()


def test_normalize_payload():
    payload = {
        "device_id": "FS-001",
        "timestamp": "2026-08-05T10:15:00Z",
        "temperature": 22.4,
        "humidity": 48.2,
        "pressure": 1008.4,
    }

    result = normalize_payload(payload)

    assert result["device_id"] == "FS-001"
    assert result["temperature"] == 22.4
    assert result["pressure"] == 1008.4


def test_mqtt_publish_uses_topic_and_json_payload():
    client = MqttClient("localhost", 1883)
    client._mqtt_client = DummyMqttClient()

    payload = {"device_id": "FS-001", "temperature": 22.4}
    assert client.publish("floatingsensor/FS-001/telemetry", payload) is True
    assert client._mqtt_client.last_topic == "floatingsensor/FS-001/telemetry"
    assert '"device_id": "FS-001"' in client._mqtt_client.last_payload
