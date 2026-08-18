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
