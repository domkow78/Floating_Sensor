"""Storage client and record mapping for MVP InfluxDB writes."""

from datetime import datetime

from influxdb_client import InfluxDBClient as _InfluxDBClient
from influxdb_client.client.write_api import SYNCHRONOUS

from config.settings import INFLUX_BUCKET, INFLUX_ORG, INFLUX_TOKEN, INFLUX_URL
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
    def __init__(
        self,
        host: str = INFLUX_URL,
        port: int = 8086,
        bucket: str = INFLUX_BUCKET,
        token: str = INFLUX_TOKEN,
        org: str = INFLUX_ORG,
    ):
        self.url = host if host.startswith("http") else f"http://{host}:{port}"
        self.bucket = bucket
        self.org = org
        self._client = _InfluxDBClient(url=self.url, token=token, org=org)
        self._write_api = self._client.write_api(write_options=SYNCHRONOUS)
        self._query_api = self._client.query_api()
        self.last_point = None

    def write(self, payload: dict | TelemetryRecord) -> bool:
        point = map_telemetry_to_point(payload, bucket=self.bucket)
        self.last_point = point
        from influxdb_client import Point
        p = Point(point["measurement"]).time(point["time"])
        for k, v in point["tags"].items():
            p = p.tag(k, v)
        for k, v in point["fields"].items():
            p = p.field(k, float(v))
        self._write_api.write(bucket=self.bucket, org=self.org, record=p)
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

        flux = f'''
from(bucket: "{self.bucket}")
  |> range(start: {from_timestamp}, stop: {to_timestamp})
  |> filter(fn: (r) => r._measurement == "{MEASUREMENT_NAME}")
  |> filter(fn: (r) => r.device_id == "{device_id}")
  |> pivot(rowKey: ["_time"], columnKey: ["_field"], valueColumn: "_value")
  |> limit(n: {limit})
'''
        tables = self._query_api.query(flux, org=self.org)
        points = []
        for table in tables:
            for record in table.records:
                point = {"timestamp": record.get_time().strftime("%Y-%m-%dT%H:%M:%SZ")}
                for field in VALUE_FIELDS:
                    if record.values.get(field) is not None:
                        point[field] = record.values[field]
                points.append(point)

        return {
            "device_id": device_id,
            "points": points,
            "count": len(points),
        }
