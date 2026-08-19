"""ASGI entrypoint for running the MVP REST API with uvicorn."""

from fastapi import FastAPI

from api.app import create_app

app: FastAPI = create_app()
