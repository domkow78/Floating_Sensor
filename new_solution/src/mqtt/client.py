"""MQTT client for the MVP."""

import json

try:
    import paho.mqtt.client as mqtt
except ImportError:  # pragma: no cover - dependency is installed in normal runtime
    mqtt = None

try:
    from paho.mqtt.client import CallbackAPIVersion
except ImportError:  # pragma: no cover - older versions of paho-mqtt
    CallbackAPIVersion = None


ALLOWED_TOPICS = {"status", "telemetry", "diagnostics"}


def build_topic(device_id: str, topic: str) -> str:
    if not device_id:
        raise ValueError("device_id is required")
    if topic not in ALLOWED_TOPICS:
        raise ValueError(f"Unsupported topic: {topic}")
    return f"floatingsensor/{device_id}/{topic}"


class MqttClient:
    def __init__(self, host: str, port: int, client_id: str = "floating_sensor"):
        self.host = host
        self.port = port
        self.client_id = client_id
        self._mqtt_client = self._build_client() if mqtt is not None else None

    def _build_client(self):
        if CallbackAPIVersion is not None:
            return mqtt.Client(callback_api_version=CallbackAPIVersion.VERSION2, client_id=self.client_id)
        return mqtt.Client(client_id=self.client_id)

    def connect(self) -> bool:
        if self._mqtt_client is None:
            return False
        self._mqtt_client.connect(self.host, self.port, 60)
        return True

    def publish(self, topic: str, payload: str | dict) -> bool:
        if isinstance(payload, (dict, list)):
            payload = json.dumps(payload)

        if self._mqtt_client is None:
            print(f"Publishing to {topic}: {payload}")
            return True

        message_info = self._mqtt_client.publish(topic, payload, qos=1)
        return message_info.rc == 0

    def publish_telemetry(self, payload: dict) -> bool:
        topic = build_topic(payload.get("device_id", ""), "telemetry")
        return self.publish(topic, payload)

    def publish_status(self, payload: dict) -> bool:
        topic = build_topic(payload.get("device_id", ""), "status")
        return self.publish(topic, payload)

    def publish_diagnostics(self, payload: dict) -> bool:
        topic = build_topic(payload.get("device_id", ""), "diagnostics")
        return self.publish(topic, payload)
