"""MQTT Gateway: subscribes to broker topics and feeds the processing pipeline."""

import json
import logging

from config.settings import MQTT_BROKER_HOST, MQTT_BROKER_PORT
from mqtt.client import MqttClient
from processing.engine import PayloadValidationError, normalize_payload
from registry.device_registry import DeviceRegistry
from storage.influx_client import InfluxClient

logger = logging.getLogger(__name__)

SUBSCRIBE_TOPIC = "floatingsensor/#"


class MqttGateway:
    def __init__(
        self,
        registry: DeviceRegistry,
        storage: InfluxClient,
        host: str = MQTT_BROKER_HOST,
        port: int = MQTT_BROKER_PORT,
    ):
        self._registry = registry
        self._storage = storage
        self._client = MqttClient(host=host, port=port, client_id="iot-core-gateway")

    def _on_message(self, client, userdata, message) -> None:
        topic = message.topic
        try:
            payload = json.loads(message.payload.decode())
        except (json.JSONDecodeError, UnicodeDecodeError):
            logger.warning("Invalid JSON on topic %s", topic)
            return

        # only process telemetry; status/diagnostics handled separately later
        if not topic.endswith("/telemetry"):
            return

        try:
            record = normalize_payload(payload)
            self._registry.update_telemetry(record)
            self._storage.write(record)
            logger.info("Ingested telemetry for %s", record.device_id)
        except PayloadValidationError as exc:
            logger.warning("Payload validation failed on %s: %s", topic, exc)
        except Exception as exc:
            logger.error("Pipeline error on %s: %s", topic, exc)

    def start(self) -> None:
        self._client.connect()
        self._client.subscribe(SUBSCRIBE_TOPIC, self._on_message)
        self._client.start_loop()
        logger.info("MQTT Gateway started, listening on %s", SUBSCRIBE_TOPIC)

    def stop(self) -> None:
        self._client.stop_loop()
        logger.info("MQTT Gateway stopped")
