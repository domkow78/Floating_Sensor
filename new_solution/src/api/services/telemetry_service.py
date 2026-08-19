"""Read-only API service builders for MVP telemetry endpoints."""

from datetime import datetime

from api.errors.http_errors import invalid_query, resource_not_found
from processing.engine import normalize_payload
from registry.device_registry import DeviceRegistry
from storage.influx_client import InfluxClient


DEFAULT_DEV_PAYLOAD = {
    "device_id": "FS-001",
    "timestamp": "2026-08-05T10:15:00Z",
    "temperature": 22.4,
    "humidity": 48.2,
    "pressure": 1008.4,
}


def build_status_response() -> dict:
    return {
        "status": "success",
        "data": {
            "iot_core": "ok",
            "mqtt": "ok",
            "influxdb": "ok",
            "timestamp": "2026-08-05T10:16:00Z",
        },
    }


def _resolve_device_state(registry: DeviceRegistry, device_id: str | None = None):
    if device_id:
        device_state = registry.get_device(device_id)
    else:
        devices = registry.all_devices()
        device_state = devices[0] if len(devices) == 1 else None

    if device_state is None:
        raise resource_not_found()

    return device_state


def build_device_response(registry: DeviceRegistry, device_id: str | None = None) -> dict:
    device_state = _resolve_device_state(registry, device_id=device_id)
    telemetry = device_state.last_telemetry
    return {
        "status": "success",
        "data": {
            "device_id": device_state.device_id,
            "state": "online",
            "firmware_version": telemetry.firmware or "unknown",
            "last_seen": device_state.last_seen,
        },
    }


def build_latest_telemetry_response(
    registry: DeviceRegistry,
    device_id: str | None = None,
) -> dict:
    device_state = _resolve_device_state(registry, device_id=device_id)
    telemetry = device_state.last_telemetry
    return {
        "status": "success",
        "data": telemetry.to_dict(),
    }


def build_telemetry_history_response(
    influx_client: InfluxClient,
    device_id: str,
    from_timestamp: str,
    to_timestamp: str,
    limit: int = 500,
) -> dict:
    try:
        datetime.fromisoformat(from_timestamp.replace("Z", "+00:00"))
        datetime.fromisoformat(to_timestamp.replace("Z", "+00:00"))
    except ValueError as exc:
        raise invalid_query("Parameter 'from' and 'to' must be ISO 8601 UTC timestamp.") from exc

    if limit < 1 or limit > 5000:
        raise invalid_query("Parameter 'limit' must be between 1 and 5000.")

    data = influx_client.query_history(device_id, from_timestamp, to_timestamp, limit=limit)
    return {
        "status": "success",
        "data": data,
    }


def ingest_telemetry(
    registry: DeviceRegistry,
    influx_client: InfluxClient,
    payload: dict | None = None,
) -> dict:
    telemetry_record = normalize_payload(payload or DEFAULT_DEV_PAYLOAD)
    registry.update_telemetry(telemetry_record)
    influx_client.write(telemetry_record)
    return {
        "status": "success",
        "data": {
            "ingested": True,
            "telemetry": telemetry_record.to_dict(),
        },
    }
