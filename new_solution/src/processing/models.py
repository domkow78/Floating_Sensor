"""Domain models for MVP telemetry processing."""

from dataclasses import dataclass


@dataclass(slots=True)
class TelemetryRecord:
    device_id: str
    timestamp: str
    temperature: float | None = None
    humidity: float | None = None
    pressure: float | None = None
    gas_resistance: float | None = None
    accel_x: float | None = None
    accel_y: float | None = None
    accel_z: float | None = None
    firmware: str | None = None
    location: str | None = None

    def to_dict(self) -> dict:
        return {
            key: value
            for key, value in {
                "device_id": self.device_id,
                "timestamp": self.timestamp,
                "temperature": self.temperature,
                "humidity": self.humidity,
                "pressure": self.pressure,
                "gas_resistance": self.gas_resistance,
                "accel_x": self.accel_x,
                "accel_y": self.accel_y,
                "accel_z": self.accel_z,
                "firmware": self.firmware,
                "location": self.location,
            }.items()
            if value is not None
        }
