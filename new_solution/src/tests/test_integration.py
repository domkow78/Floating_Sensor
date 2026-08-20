"""Integration tests requiring a live InfluxDB instance on localhost:8086."""

import pytest

from processing.models import TelemetryRecord
from storage.influx_client import InfluxClient

DEVICE_ID = "FS-TEST-001"
TIMESTAMP = "2026-08-20T10:00:00Z"


@pytest.fixture
def influx():
    return InfluxClient()


def test_influx_write_real(influx):
    record = TelemetryRecord(
        device_id=DEVICE_ID,
        timestamp=TIMESTAMP,
        temperature=21.5,
        humidity=50.0,
        pressure=1010.0,
    )
    assert influx.write(record) is True


def test_influx_query_history_real(influx):
    record = TelemetryRecord(
        device_id=DEVICE_ID,
        timestamp=TIMESTAMP,
        temperature=21.5,
        humidity=50.0,
        pressure=1010.0,
    )
    influx.write(record)

    result = influx.query_history(
        device_id=DEVICE_ID,
        from_timestamp="2026-08-20T09:00:00Z",
        to_timestamp="2026-08-20T11:00:00Z",
        limit=10,
    )

    assert result["device_id"] == DEVICE_ID
    assert isinstance(result["points"], list)
    assert result["count"] >= 1
    assert result["points"][0]["temperature"] == 21.5
