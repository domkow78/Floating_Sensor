import logging
import signal
import threading
import time
from pathlib import Path

import yaml

from commands.commander import Commander
from mqtt.client import MqttClient
from parser.payload_parser import PayloadParser
from registry.sensor_registry import SensorRegistry
from storage.influx_store import InfluxStore

logger = logging.getLogger(__name__)


class FloatingSensorHub:
    def __init__(self, config: dict):
        self._config = config
        self._parser = PayloadParser(config["payload_schema"])
        self._registry = SensorRegistry(timeout=int(config["sensor"].get("timeout", 120)))
        self._store = InfluxStore(config)
        self._mqtt = MqttClient(config)
        self._commander = Commander(self._mqtt)
        self._stop_event = threading.Event()

    @property
    def commander(self) -> Commander:
        return self._commander

    def start(self) -> None:
        self._register_handlers()
        self._mqtt.start()
        logger.info("Floating Sensor Hub started")

    def stop(self) -> None:
        if self._stop_event.is_set():
            return
        self._stop_event.set()
        logger.info("Stopping Floating Sensor Hub...")
        try:
            self._mqtt.stop()
        finally:
            self._store.close()
        logger.info("Floating Sensor Hub stopped")

    def wait_forever(self) -> None:
        while not self._stop_event.is_set():
            time.sleep(1)

    def _register_handlers(self) -> None:
        self._mqtt.register_handler("registerMe", self._handle_register)
        self._mqtt.register_handler("measurement", self._handle_measurement)
        self._mqtt.register_handler("diag", self._handle_diag)

    def _handle_register(self, sensor_id: str, payload) -> None:
        self._registry.on_register(sensor_id, payload)
        logger.info("Sensor registered: %s", sensor_id)

    def _handle_measurement(self, sensor_id: str, payload) -> None:
        parsed = self._parser.parse("measurement", payload)
        self._registry.on_measurement(sensor_id, parsed)
        if parsed is None:
            logger.warning("Ignored invalid measurement payload from %s", sensor_id)
            return
        self._store.write_measurement(sensor_id, parsed)
        logger.debug("Measurement stored for sensor %s", sensor_id)

    def _handle_diag(self, sensor_id: str, payload) -> None:
        parsed = self._parser.parse("diag", payload)
        self._registry.on_diag(sensor_id, parsed)
        if parsed is None:
            logger.warning("Ignored invalid diag payload from %s", sensor_id)
            return
        self._store.write_diag(sensor_id, parsed)
        logger.debug("Diag stored for sensor %s", sensor_id)


def load_config(config_path: Path) -> dict:
    with config_path.open("r", encoding="utf-8") as config_file:
        return yaml.safe_load(config_file)


def configure_logging() -> None:
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s | %(levelname)s | %(name)s | %(message)s",
    )


def main() -> None:
    configure_logging()
    config_path = Path(__file__).resolve().parent / "config.yaml"
    config = load_config(config_path)
    hub = FloatingSensorHub(config)

    def _shutdown_handler(signum, _frame) -> None:
        logger.info("Received signal %s", signum)
        hub.stop()

    for sig in (signal.SIGINT, signal.SIGTERM):
        signal.signal(sig, _shutdown_handler)

    try:
        hub.start()
        hub.commander.register_call()
        hub.wait_forever()
    except KeyboardInterrupt:
        logger.info("Keyboard interrupt received")
        hub.stop()
    except Exception:
        logger.exception("Fatal hub error")
        hub.stop()
        raise


if __name__ == "__main__":
    main()
