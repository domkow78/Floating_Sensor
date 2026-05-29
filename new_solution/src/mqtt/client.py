"""
MQTT client wrapper using paho-mqtt.
Runs paho's network loop in a dedicated daemon thread.
Incoming messages are dispatched to registered handlers by smallTopic.
"""

import json
import logging
import os
import threading
import time

import paho.mqtt.client as paho

from mqtt import topics

logger = logging.getLogger(__name__)


class MqttClient:
    def __init__(self, config: dict):
        cfg = config["mqtt"]
        self._broker = os.environ.get("MQTT_BROKER", cfg["broker"])
        self._port = int(cfg["port"])
        self._hub_id = cfg["hub_id"]
        self._client_id = cfg.get("client_id", "floating_sensor_hub")
        self._keepalive = int(cfg.get("keepalive", 60))
        self._reconnect_delay = int(cfg.get("reconnect_delay", 5))

        self._handlers: dict[str, list] = {}  # small_topic -> [callable]
        self._connected = False
        self._lock = threading.Lock()

        self._client = paho.Client(
            client_id=self._client_id,
            callback_api_version=paho.CallbackAPIVersion.VERSION2,
        )
        self._client.on_connect = self._on_connect
        self._client.on_disconnect = self._on_disconnect
        self._client.on_message = self._on_message

    # ------------------------------------------------------------------ #
    # Public API                                                           #
    # ------------------------------------------------------------------ #

    def register_handler(self, small_topic: str, handler) -> None:
        """Register a callback for a specific smallTopic.
        handler(sensor_id: str, payload: dict | str) -> None
        """
        self._handlers.setdefault(small_topic, []).append(handler)

    def publish(self, target: str, small_topic: str, payload, qos: int = 1) -> None:
        """Publish a message to ws/<target>/<hub_id>/<small_topic>."""
        topic = topics.build(target, self._hub_id, small_topic)
        if isinstance(payload, (dict, list)):
            payload = json.dumps(payload)
        self._client.publish(topic, payload, qos=qos)
        logger.debug("Published → %s : %s", topic, payload)

    def start(self) -> None:
        """Connect and start the background network loop thread."""
        self._thread = threading.Thread(
            target=self._run_loop, name="mqtt-loop", daemon=True
        )
        self._thread.start()
        logger.info("MQTT thread started (broker=%s:%d)", self._broker, self._port)

    def stop(self) -> None:
        self._client.disconnect()
        logger.info("MQTT client disconnected")

    @property
    def is_connected(self) -> bool:
        return self._connected

    @property
    def hub_id(self) -> str:
        return self._hub_id

    # ------------------------------------------------------------------ #
    # Internal                                                             #
    # ------------------------------------------------------------------ #

    def _run_loop(self) -> None:
        while True:
            try:
                logger.info("Connecting to MQTT broker %s:%d …", self._broker, self._port)
                self._client.connect(self._broker, self._port, self._keepalive)
                self._client.loop_forever(retry_first_connection=True)
            except Exception as exc:
                logger.warning("MQTT connection error: %s — retrying in %ds", exc, self._reconnect_delay)
                time.sleep(self._reconnect_delay)

    def _on_connect(self, client, userdata, flags, reason_code, properties) -> None:
        if reason_code == 0:
            self._connected = True
            sub_filter = topics.subscribe_filter(self._hub_id)
            client.subscribe(sub_filter, qos=0)
            logger.info("MQTT connected. Subscribed to: %s", sub_filter)
        else:
            logger.error("MQTT connect failed, reason code: %s", reason_code)

    def _on_disconnect(self, client, userdata, flags, reason_code, properties) -> None:
        self._connected = False
        logger.warning("MQTT disconnected (reason=%s). Will reconnect …", reason_code)

    def _on_message(self, client, userdata, msg) -> None:
        parsed = topics.parse(msg.topic)
        if parsed is None:
            logger.debug("Ignoring message on unexpected topic: %s", msg.topic)
            return

        sensor_id = parsed["source"]
        small_topic = parsed["small_topic"]

        # Decode payload
        raw = msg.payload.decode("utf-8", errors="replace").strip()
        try:
            payload = json.loads(raw)
        except json.JSONDecodeError:
            payload = raw  # plain string payloads (e.g. registerMe)

        logger.debug("← %s | sensor=%s | payload=%s", small_topic, sensor_id, payload)

        handlers = self._handlers.get(small_topic, [])
        if not handlers:
            logger.debug("No handler for smallTopic '%s'", small_topic)
        for handler in handlers:
            try:
                handler(sensor_id, payload)
            except Exception as exc:
                logger.exception("Handler error for '%s': %s", small_topic, exc)
