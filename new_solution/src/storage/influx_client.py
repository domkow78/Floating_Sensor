"""Storage client and record mapping for MVP InfluxDB writes."""

from datetime import datetime

from processing.models import TelemetryRecord


MEASUREMENT_NAME = "environment"
DEFAULT_BUCKET = "floating_sensor"
TAG_FIELDS = ("device_id", "firmware", "location")
VALUE_FIELDS = (
    "temperature",
    "humidity",
    "pressure",
    "gas_resistance",
    "accel_x",
    "accel_y",
    "accel_z",
)


def map_telemetry_to_point(payload: dict | TelemetryRecord, bucket: str = DEFAULT_BUCKET) -> dict:
    if isinstance(payload, TelemetryRecord):
        payload = payload.to_dict()

    if not payload.get("device_id"):
        raise ValueError("device_id is required for storage")
    if not payload.get("timestamp"):
        raise ValueError("timestamp is required for storage")

    tags = {
        field: payload[field]
        for field in TAG_FIELDS
        if field in payload and payload[field] is not None
    }
    fields = {
        field: payload[field]
        for field in VALUE_FIELDS
        if field in payload and payload[field] is not None
    }

    return {
        "bucket": bucket,
        "measurement": MEASUREMENT_NAME,
        "tags": tags,
        "fields": fields,
        "time": payload["timestamp"],
    }


class InfluxClient:
    def __init__(self, host: str, port: int, bucket: str = DEFAULT_BUCKET):
        self.host = host
        self.port = port
        self.bucket = bucket
        self.last_point = None

    def write(self, payload: dict | TelemetryRecord) -> bool:
        point = map_telemetry_to_point(payload, bucket=self.bucket)
        self.last_point = point
        print(f"Writing to InfluxDB: {point}")
        return True

    def query_history(
        self,
        device_id: str,
        from_timestamp: str,
        to_timestamp: str,
        limit: int = 500,
    ) -> dict:
        datetime.fromisoformat(from_timestamp.replace("Z", "+00:00"))
        datetime.fromisoformat(to_timestamp.replace("Z", "+00:00"))

        points = []
        if self.last_point and self.last_point["tags"].get("device_id") == device_id:
            point = {
                "timestamp": self.last_point["time"],
                **self.last_point["fields"],
            }
            points.append(point)

        return {
            "device_id": device_id,
            "points": points[:limit],
            "count": len(points[:limit]),
        }
