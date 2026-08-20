"""ASGI entrypoint for running the MVP REST API with uvicorn."""

import logging

from contextlib import asynccontextmanager

from fastapi import FastAPI

from api.app import create_app
from mqtt.gateway import MqttGateway
from registry.device_registry import DeviceRegistry
from storage.influx_client import InfluxClient

logging.basicConfig(level=logging.INFO)

_registry = DeviceRegistry()
_storage = InfluxClient()
_gateway = MqttGateway(registry=_registry, storage=_storage)


@asynccontextmanager
async def lifespan(application: FastAPI):
    _gateway.start()
    yield
    _gateway.stop()


app: FastAPI = create_app(device_registry=_registry, influx_client=_storage, lifespan=lifespan)
