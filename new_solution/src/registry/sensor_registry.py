"""
Thread-safe registry of active sensors.
Tracks last-seen time and buffers the most recent measurement and diag payloads.
A sensor is considered inactive after 'sensor.timeout' seconds without a message.
"""

import logging
import threading
import time
from dataclasses import dataclass, field
from typing import Any

logger = logging.getLogger(__name__)


@dataclass
class SensorState:
    sensor_id: str
    last_seen: float = field(default_factory=time.time)
    last_measurement: dict[str, Any] | None = None
    last_diag: dict[str, Any] | None = None
    registered: bool = False


class SensorRegistry:
    def __init__(self, timeout: int = 120):
        self._timeout = timeout
        self._sensors: dict[str, SensorState] = {}
        self._lock = threading.Lock()

    # ------------------------------------------------------------------ #
    # Public API                                                           #
    # ------------------------------------------------------------------ #

    def on_register(self, sensor_id: str, _payload) -> None:
        """Called when a 'registerMe' message is received."""
        with self._lock:
            if sensor_id not in self._sensors:
                self._sensors[sensor_id] = SensorState(sensor_id=sensor_id)
                logger.info("New sensor registered: %s", sensor_id)
            state = self._sensors[sensor_id]
            state.last_seen = time.time()
            state.registered = True

    def on_measurement(self, sensor_id: str, data: dict | None) -> None:
        """Update last measurement for a sensor."""
        if data is None:
            return
        with self._lock:
            state = self._get_or_create(sensor_id)
            state.last_seen = time.time()
            state.last_measurement = data

    def on_diag(self, sensor_id: str, data: dict | None) -> None:
        """Update last diag for a sensor."""
        if data is None:
            return
        with self._lock:
            state = self._get_or_create(sensor_id)
            state.last_seen = time.time()
            state.last_diag = data

    def get_all(self) -> list[SensorState]:
        """Return a snapshot of all sensor states."""
        with self._lock:
            return list(self._sensors.values())

    def get_active(self) -> list[SensorState]:
        """Return sensors seen within the timeout window."""
        now = time.time()
        with self._lock:
            return [
                s for s in self._sensors.values()
                if (now - s.last_seen) < self._timeout
            ]

    def get_sensor_ids(self) -> list[str]:
        """Return IDs of currently active sensors."""
        return [s.sensor_id for s in self.get_active()]

    def get(self, sensor_id: str) -> SensorState | None:
        with self._lock:
            return self._sensors.get(sensor_id)

    # ------------------------------------------------------------------ #
    # Internal                                                             #
    # ------------------------------------------------------------------ #

    def _get_or_create(self, sensor_id: str) -> SensorState:
        if sensor_id not in self._sensors:
            self._sensors[sensor_id] = SensorState(sensor_id=sensor_id)
        return self._sensors[sensor_id]
