"""
Commander — the single API for sending commands to sensors via MQTT.
Used by the Streamlit UI.

All commands are published to:  ws/<sensor_id>/<hub_id>/<small_topic>
"""

import logging
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from mqtt.client import MqttClient

logger = logging.getLogger(__name__)


class Commander:
    def __init__(self, mqtt_client: "MqttClient"):
        self._mqtt = mqtt_client

    # ------------------------------------------------------------------ #
    # Sensor lifecycle                                                     #
    # ------------------------------------------------------------------ #

    def register_call(self) -> None:
        """Broadcast registerCall to all sensors."""
        self._mqtt.publish("all", "registerCall", "", qos=1)
        logger.info("Sent registerCall broadcast")

    def start_meas(self, sensor_id: str, count: int = -1) -> None:
        """Start measurement task on a sensor. count=-1 means infinite."""
        self._publish(sensor_id, "startMeas", {"count": count})

    def stop_meas(self, sensor_id: str) -> None:
        """Stop measurement task on a sensor."""
        self._publish(sensor_id, "stopMeas", {})

    def request_status(self, sensor_id: str) -> None:
        """Request a diag/status message from a sensor."""
        self._publish(sensor_id, "statusReq", {})

    def sleep_now(self, sensor_id: str) -> None:
        """Send sensor to deep sleep immediately."""
        self._publish(sensor_id, "sleepNow", {})

    # ------------------------------------------------------------------ #
    # Configuration                                                        #
    # ------------------------------------------------------------------ #

    def set_meas_freq(self, sensor_id: str, freq_s: int) -> None:
        """Set measurement period in seconds."""
        self._publish(sensor_id, "measFreq", {"freq": freq_s})

    def set_send_interval(self, sensor_id: str, interval_s: int) -> None:
        """Set deferred-send interval in seconds."""
        self._publish(sensor_id, "sendInterval", {"sendInterval": interval_s})

    def set_acmm(
        self,
        sensor_id: str,
        freq: int,
        agg_over: int,
        ph_lo_f: int,
        ph_hi_f: int,
    ) -> None:
        """Configure ACMM parameters."""
        self._publish(sensor_id, "setAcmm", {
            "freq": freq,
            "aggOver": agg_over,
            "phLoF": ph_lo_f,
            "phHiF": ph_hi_f,
        })

    def set_power_management(self, sensor_id: str, enabled: bool) -> None:
        self._publish(sensor_id, "pm", {"value": "on" if enabled else "off"})

    def set_phase(self, sensor_id: str, enabled: bool) -> None:
        self._publish(sensor_id, "phase", {"value": "on" if enabled else "off"})

    def clear_acmm(self, sensor_id: str) -> None:
        self._publish(sensor_id, "clearAcmm", {"initValue": 0})

    def clear_imp(self, sensor_id: str) -> None:
        self._publish(sensor_id, "clearImp", {"initValue": 0})

    # ------------------------------------------------------------------ #
    # Internal                                                             #
    # ------------------------------------------------------------------ #

    def _publish(self, sensor_id: str, small_topic: str, payload: dict) -> None:
        self._mqtt.publish(sensor_id, small_topic, payload, qos=1)
        logger.info("CMD → %s | %s | %s", sensor_id, small_topic, payload)
