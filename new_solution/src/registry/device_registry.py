"""In-memory device registry for MVP device state."""

from dataclasses import dataclass

from processing.models import TelemetryRecord


@dataclass(slots=True)
class DeviceState:
    device_id: str
    last_seen: str
    last_telemetry: TelemetryRecord


class DeviceRegistry:
    def __init__(self):
        self._devices: dict[str, DeviceState] = {}

    def update_telemetry(self, telemetry_record: TelemetryRecord) -> DeviceState:
        state = DeviceState(
            device_id=telemetry_record.device_id,
            last_seen=telemetry_record.timestamp,
            last_telemetry=telemetry_record,
        )
        self._devices[telemetry_record.device_id] = state
        return state

    def get_device(self, device_id: str) -> DeviceState | None:
        return self._devices.get(device_id)

    def all_devices(self) -> list[DeviceState]:
        return list(self._devices.values())