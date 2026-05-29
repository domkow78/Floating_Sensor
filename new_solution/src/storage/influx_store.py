"""
InfluxDB v2 storage layer.
Writes parsed measurement and diag payloads as time-series points.

Measurements in InfluxDB:
  - sensor_measurement  (from 'measurement' MQTT topic)
  - sensor_diag         (from 'diag' MQTT topic)

Tags:   sensor_id
Fields: all numeric/string values from the payload (except 'ts')
Time:   from payload field 'ts' (milliseconds → nanoseconds)
"""

import logging
import os
from datetime import datetime, timezone

from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS

logger = logging.getLogger(__name__)


class InfluxStore:
    def __init__(self, config: dict):
        cfg = config["influxdb"]
        url = os.environ.get("INFLUXDB_URL", cfg["url"])
        token = os.environ.get("INFLUXDB_TOKEN", cfg.get("token", ""))
        self._org = cfg["org"]
        self._bucket_meas = cfg["bucket_measurements"]
        self._bucket_diag = cfg.get("bucket_diag", "diagnostics")

        self._client = InfluxDBClient(url=url, token=token, org=self._org)
        self._write_api = self._client.write_api(write_options=SYNCHRONOUS)
        logger.info("InfluxDB store initialised (url=%s, org=%s)", url, self._org)

    # ------------------------------------------------------------------ #
    # Public API                                                           #
    # ------------------------------------------------------------------ #

    def write_measurement(self, sensor_id: str, data: dict) -> None:
        """Write a parsed 'measurement' payload to InfluxDB."""
        self._write(
            measurement_name="sensor_measurement",
            bucket=self._bucket_meas,
            sensor_id=sensor_id,
            data=data,
        )

    def write_diag(self, sensor_id: str, data: dict) -> None:
        """Write a parsed 'diag' payload to InfluxDB."""
        self._write(
            measurement_name="sensor_diag",
            bucket=self._bucket_diag,
            sensor_id=sensor_id,
            data=data,
        )

    def query(self, flux: str) -> list:
        """Execute a Flux query and return list of records."""
        try:
            query_api = self._client.query_api()
            result = query_api.query(flux, org=self._org)
            records = []
            for table in result:
                for record in table.records:
                    records.append(record.values)
            return records
        except Exception as exc:
            logger.error("InfluxDB query error: %s", exc)
            return []

    def close(self) -> None:
        self._client.close()

    # ------------------------------------------------------------------ #
    # Internal                                                             #
    # ------------------------------------------------------------------ #

    def _write(
        self,
        measurement_name: str,
        bucket: str,
        sensor_id: str,
        data: dict,
    ) -> None:
        try:
            point = Point(measurement_name).tag("sensor_id", sensor_id)

            # Timestamp: field 'ts' is milliseconds → convert to nanoseconds
            ts_ms = data.get("ts")
            if ts_ms is not None:
                point = point.time(int(ts_ms) * 1_000_000, WritePrecision.NANOSECONDS)
            else:
                point = point.time(
                    datetime.now(timezone.utc), WritePrecision.NANOSECONDS
                )

            # All fields except 'ts'
            for key, value in data.items():
                if key == "ts":
                    continue
                if isinstance(value, (int, float)):
                    point = point.field(key, value)
                elif isinstance(value, str):
                    point = point.field(key, value)
                # skip None or unknown types

            self._write_api.write(bucket=bucket, org=self._org, record=point)
            logger.debug(
                "Wrote %s → %s | sensor=%s", measurement_name, bucket, sensor_id
            )
        except Exception as exc:
            logger.error(
                "InfluxDB write error (%s, sensor=%s): %s",
                measurement_name, sensor_id, exc,
            )
