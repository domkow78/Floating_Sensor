"""Versioned REST routes for the MVP API."""

from fastapi import Body, FastAPI, Query

from api.services.telemetry_service import (
    build_device_response,
    build_latest_telemetry_response,
    build_status_response,
    build_telemetry_history_response,
    ingest_telemetry,
)
from registry.device_registry import DeviceRegistry
from storage.influx_client import InfluxClient


def register_v1_routes(app: FastAPI, registry: DeviceRegistry, storage: InfluxClient) -> None:
    @app.get("/api/v1/status")
    def get_status() -> dict:
        return build_status_response()

    @app.get("/api/v1/device")
    def get_device(device_id: str | None = Query(default=None)) -> dict:
        return build_device_response(registry, device_id=device_id)

    @app.get("/api/v1/telemetry/latest")
    def get_latest_telemetry(device_id: str | None = Query(default=None)) -> dict:
        return build_latest_telemetry_response(registry, device_id=device_id)

    @app.get("/api/v1/telemetry/history")
    def get_telemetry_history(
        device_id: str = Query(),
        from_timestamp: str = Query(alias="from"),
        to_timestamp: str = Query(alias="to"),
        limit: int = Query(default=500),
    ) -> dict:
        return build_telemetry_history_response(
            storage,
            device_id=device_id,
            from_timestamp=from_timestamp,
            to_timestamp=to_timestamp,
            limit=limit,
        )

    @app.post("/api/v1/dev/ingest")
    def post_dev_ingest(payload: dict | None = Body(default=None)) -> dict:
        return ingest_telemetry(registry, storage, payload=payload)
