"""FastAPI application for the MVP read-only REST layer."""

from fastapi import FastAPI

from api.routes.v1 import register_v1_routes
from api.services.telemetry_service import (
    build_device_response,
    build_latest_telemetry_response,
    build_status_response,
    build_telemetry_history_response,
)
from registry.device_registry import DeviceRegistry
from storage.influx_client import InfluxClient


def create_app(
    device_registry: DeviceRegistry | None = None,
    influx_client: InfluxClient | None = None,
) -> FastAPI:
    app = FastAPI(title="Floating Sensor IoT Core API", version="0.1.0")
    registry = device_registry or DeviceRegistry()
    storage = influx_client or InfluxClient("localhost", 8086)

    register_v1_routes(app, registry, storage)

    return app