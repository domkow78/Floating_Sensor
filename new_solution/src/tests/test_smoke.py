"""Smoke tests for the initial MVP structure."""

import pytest
from fastapi import HTTPException
from fastapi import FastAPI

from api.app import (
    build_device_response,
    build_latest_telemetry_response,
    build_telemetry_history_response,
    build_status_response,
    create_app,
)
from api.services.telemetry_service import ingest_telemetry
from api.main import app as asgi_app
from app.main import build_sample_payload, run_pipeline
from mqtt.client import MqttClient, build_topic
from processing.engine import PayloadValidationError, normalize_payload
from processing.models import TelemetryRecord
from registry.device_registry import DeviceRegistry
from storage.influx_client import InfluxClient, map_telemetry_to_point


class DummyMessage:
    rc = 0


class DummyMqttClient:
    def __init__(self):
        self.last_topic = None
        self.last_payload = None

    def publish(self, topic: str, payload: str, qos: int = 1):
        self.last_topic = topic
        self.last_payload = payload
        return DummyMessage()

    def publish_telemetry(self, payload: dict) -> bool:
        self.last_topic = f"floatingsensor/{payload['device_id']}/telemetry"
        self.last_payload = '{"device_id": "FS-001", "timestamp": "2026-08-05T10:15:00Z", "temperature": 22.4, "humidity": 48.2, "pressure": 1008.4}'
        return True


class DummyInfluxClient:
    def __init__(self):
        self.last_payload = None

    def write(self, payload: dict) -> bool:
        self.last_payload = payload
        return True

    def query_history(self, device_id: str, from_timestamp: str, to_timestamp: str, limit: int = 500) -> dict:
        points = []
        if self.last_payload is not None:
            record = self.last_payload if isinstance(self.last_payload, dict) else self.last_payload.to_dict()
            points.append({"timestamp": record.get("timestamp"), "temperature": record.get("temperature")})
        return {"device_id": device_id, "points": points, "count": len(points)}


def test_normalize_payload():
    payload = {
        "device_id": "FS-001",
        "timestamp": "2026-08-05T10:15:00Z",
        "temperature": 22.4,
        "humidity": 48.2,
        "pressure": 1008.4,
    }

    result = normalize_payload(payload)

    assert result.device_id == "FS-001"
    assert result.temperature == 22.4
    assert result.pressure == 1008.4


def test_normalize_payload_rejects_missing_required_field():
    payload = {
        "device_id": "FS-001",
        "temperature": 22.4,
    }

    with pytest.raises(PayloadValidationError):
        normalize_payload(payload)


def test_build_topic_uses_mvp_contract():
    assert build_topic("FS-001", "telemetry") == "floatingsensor/FS-001/telemetry"


def test_build_topic_rejects_unknown_topic():
    with pytest.raises(ValueError):
        build_topic("FS-001", "command")


def test_mqtt_publish_uses_topic_and_json_payload():
    client = MqttClient("localhost", 1883)
    client._mqtt_client = DummyMqttClient()

    payload = {"device_id": "FS-001", "temperature": 22.4}
    assert client.publish_telemetry(payload) is True
    assert client._mqtt_client.last_topic == "floatingsensor/FS-001/telemetry"
    assert '"device_id": "FS-001"' in client._mqtt_client.last_payload


def test_map_telemetry_to_point_uses_documented_influx_shape():
    payload = TelemetryRecord(
        device_id="FS-001",
        timestamp="2026-08-05T10:15:00Z",
        temperature=22.4,
        humidity=48.2,
        pressure=1008.4,
        firmware="1.0.0",
    )

    point = map_telemetry_to_point(payload)

    assert point["bucket"] == "floating_sensor"
    assert point["measurement"] == "environment"
    assert point["tags"]["device_id"] == "FS-001"
    assert point["tags"]["firmware"] == "1.0.0"
    assert point["fields"]["temperature"] == 22.4
    assert point["time"] == "2026-08-05T10:15:00Z"


def test_map_telemetry_to_point_rejects_missing_timestamp():
    with pytest.raises(ValueError):
        map_telemetry_to_point({"device_id": "FS-001", "temperature": 22.4})


def test_influx_client_write_stores_mapped_point():
    client = DummyInfluxClient()
    payload = {
        "device_id": "FS-001",
        "timestamp": "2026-08-05T10:15:00Z",
        "temperature": 22.4,
    }

    assert client.write(payload) is True
    assert client.last_payload is not None


def test_build_sample_payload_uses_configured_shape():
    payload = build_sample_payload()

    assert payload["device_id"] == "FS-001"
    assert payload["timestamp"] == "2026-08-05T10:15:00Z"


def test_device_registry_stores_last_telemetry():
    registry = DeviceRegistry()
    telemetry = TelemetryRecord(
        device_id="FS-001",
        timestamp="2026-08-05T10:15:00Z",
        temperature=22.4,
    )

    state = registry.update_telemetry(telemetry)

    assert state.device_id == "FS-001"
    assert state.last_seen == "2026-08-05T10:15:00Z"
    assert registry.get_device("FS-001") == state


def test_api_status_endpoint_returns_documented_shape():
    payload = build_status_response()

    assert payload["status"] == "success"
    assert payload["data"]["iot_core"] == "ok"
    assert payload["data"]["mqtt"] == "ok"
    assert payload["data"]["influxdb"] == "ok"


def test_api_device_endpoint_uses_registry_state():
    registry = DeviceRegistry()
    registry.update_telemetry(
        TelemetryRecord(
            device_id="FS-001",
            timestamp="2026-08-05T10:15:00Z",
            firmware="1.0.0",
            temperature=22.4,
        )
    )
    payload = build_device_response(registry)

    assert payload["status"] == "success"
    assert payload["data"]["device_id"] == "FS-001"
    assert payload["data"]["firmware_version"] == "1.0.0"
    assert payload["data"]["last_seen"] == "2026-08-05T10:15:00Z"


def test_api_device_endpoint_returns_404_when_missing():
    with pytest.raises(HTTPException) as exc_info:
        build_device_response(DeviceRegistry(), device_id="FS-404")

    assert exc_info.value.status_code == 404
    assert exc_info.value.detail["status"] == "error"
    assert exc_info.value.detail["code"] == "RESOURCE_NOT_FOUND"


def test_api_latest_telemetry_endpoint_uses_registry_state():
    registry = DeviceRegistry()
    registry.update_telemetry(
        TelemetryRecord(
            device_id="FS-001",
            timestamp="2026-08-05T10:15:00Z",
            temperature=22.4,
            humidity=48.2,
            pressure=1008.4,
        )
    )

    payload = build_latest_telemetry_response(registry)

    assert payload["status"] == "success"
    assert payload["data"]["device_id"] == "FS-001"
    assert payload["data"]["timestamp"] == "2026-08-05T10:15:00Z"
    assert payload["data"]["temperature"] == 22.4


def test_api_latest_telemetry_endpoint_returns_404_when_missing():
    with pytest.raises(HTTPException) as exc_info:
        build_latest_telemetry_response(DeviceRegistry(), device_id="FS-404")

    assert exc_info.value.status_code == 404
    assert exc_info.value.detail["status"] == "error"
    assert exc_info.value.detail["code"] == "RESOURCE_NOT_FOUND"


def test_api_telemetry_history_returns_documented_shape():
    influx_client = DummyInfluxClient()
    influx_client.write(
        TelemetryRecord(
            device_id="FS-001",
            timestamp="2026-08-05T10:15:00Z",
            temperature=22.4,
            humidity=48.2,
            pressure=1008.4,
        )
    )

    payload = build_telemetry_history_response(
        influx_client,
        device_id="FS-001",
        from_timestamp="2026-08-05T10:10:00Z",
        to_timestamp="2026-08-05T10:20:00Z",
        limit=500,
    )

    assert payload["status"] == "success"
    assert payload["data"]["device_id"] == "FS-001"
    assert payload["data"]["count"] == 1
    assert payload["data"]["points"][0]["timestamp"] == "2026-08-05T10:15:00Z"
    assert payload["data"]["points"][0]["temperature"] == 22.4


def test_api_telemetry_history_rejects_invalid_from_query():
    with pytest.raises(HTTPException) as exc_info:
        build_telemetry_history_response(
            DummyInfluxClient(),
            device_id="FS-001",
            from_timestamp="invalid",
            to_timestamp="2026-08-05T10:20:00Z",
        )

    assert exc_info.value.status_code == 400
    assert exc_info.value.detail["status"] == "error"
    assert exc_info.value.detail["code"] == "INVALID_QUERY"


def test_api_telemetry_history_rejects_invalid_limit():
    with pytest.raises(HTTPException) as exc_info:
        build_telemetry_history_response(
            DummyInfluxClient(),
            device_id="FS-001",
            from_timestamp="2026-08-05T10:10:00Z",
            to_timestamp="2026-08-05T10:20:00Z",
            limit=5001,
        )

    assert exc_info.value.status_code == 400
    assert exc_info.value.detail["status"] == "error"
    assert exc_info.value.detail["code"] == "INVALID_QUERY"


def test_api_app_registers_status_and_device_routes():
    app = create_app()
    route_paths = {route.path for route in app.routes}

    assert "/api/v1/status" in route_paths
    assert "/api/v1/device" in route_paths
    assert "/api/v1/telemetry/latest" in route_paths
    assert "/api/v1/telemetry/history" in route_paths
    assert "/api/v1/dev/ingest" in route_paths


def test_api_dev_ingest_updates_registry_and_storage():
    registry = DeviceRegistry()
    storage = DummyInfluxClient()

    response = ingest_telemetry(
        registry,
        storage,
        payload={
            "device_id": "FS-001",
            "timestamp": "2026-08-05T10:30:00Z",
            "temperature": 23.1,
            "humidity": 47.0,
            "pressure": 1009.2,
        },
    )

    assert response["status"] == "success"
    assert response["data"]["ingested"] is True
    assert registry.get_device("FS-001") is not None
    assert storage.last_payload is not None


def test_api_asgi_entrypoint_exposes_fastapi_app():
    assert isinstance(asgi_app, FastAPI)
    route_paths = {route.path for route in asgi_app.routes}
    assert "/api/v1/status" in route_paths


def test_run_pipeline_normalizes_publishes_and_writes():
    mqtt_client = DummyMqttClient()
    influx_client = DummyInfluxClient()
    device_registry = DeviceRegistry()
    payload = {
        "device_id": "FS-001",
        "timestamp": "2026-08-05T10:15:00Z",
        "temperature": 22.4,
        "humidity": 48.2,
        "pressure": 1008.4,
    }

    result = run_pipeline(
        payload,
        mqtt_client=mqtt_client,
        influx_client=influx_client,
        device_registry=device_registry,
    )

    assert result.device_id == "FS-001"
    assert mqtt_client.last_topic == "floatingsensor/FS-001/telemetry"
    assert '"temperature": 22.4' in mqtt_client.last_payload
    assert influx_client.last_payload == result
    assert device_registry.get_device("FS-001") is not None
    assert device_registry.get_device("FS-001").last_telemetry == result
