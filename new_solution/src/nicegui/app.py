"""NiceGUI MVP application — Dashboard + ESP32 Simulator."""

import datetime
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

import requests
from nicegui import ui

from config.settings import MQTT_BROKER_HOST, MQTT_BROKER_PORT
from mqtt.client import MqttClient

API_BASE = "http://localhost:8000/api/v1"
DEVICE_ID = "FS-001"

_mqtt = MqttClient(host=MQTT_BROKER_HOST, port=MQTT_BROKER_PORT, client_id="nicegui-simulator")
_mqtt.connect()
_mqtt.start_loop()


# ── helpers ───────────────────────────────────────────────────────────────────

def _get(path: str) -> dict | None:
    try:
        r = requests.get(f"{API_BASE}{path}", timeout=3)
        r.raise_for_status()
        return r.json()
    except Exception:
        return None


# ── Dashboard page ─────────────────────────────────────────────────────────────

@ui.page("/")
def dashboard():
    ui.label("Floating Sensor — Dashboard").classes("text-2xl font-bold mb-4")
    ui.link("Simulator →", "/simulator").classes("text-blue-500 mb-4")

    status_label = ui.label("Status: —")
    temp_label   = ui.label("Temperature: —")
    hum_label    = ui.label("Humidity: —")
    pres_label   = ui.label("Pressure: —")
    ts_label     = ui.label("Last seen: —")

    def refresh():
        data = _get(f"/telemetry/latest?device_id={DEVICE_ID}")
        if data and data.get("status") == "success":
            d = data["data"]
            temp_label.set_text(f"Temperature: {d.get('temperature', '—')} °C")
            hum_label.set_text(f"Humidity: {d.get('humidity', '—')} %")
            pres_label.set_text(f"Pressure: {d.get('pressure', '—')} hPa")
            ts_label.set_text(f"Last seen: {d.get('timestamp', '—')}")
            status_label.set_text("Status: online")
        else:
            status_label.set_text("Status: no data")

    ui.button("Refresh", on_click=refresh)
    ui.timer(10.0, refresh)
    refresh()


# ── Simulator page ─────────────────────────────────────────────────────────────

@ui.page("/simulator")
def simulator():
    ui.label("ESP32 Simulator").classes("text-2xl font-bold mb-4")
    ui.link("← Dashboard", "/").classes("text-blue-500 mb-4")

    temp  = ui.number("Temperature (°C)", value=22.4, min=-40, max=85, step=0.1)
    hum   = ui.number("Humidity (%)",     value=48.2, min=0,   max=100, step=0.1)
    pres  = ui.number("Pressure (hPa)",   value=1008.4, min=800, max=1200, step=0.1)
    result_label = ui.label("")

    def send():
        payload = {
            "device_id": DEVICE_ID,
            "timestamp": datetime.datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ"),
            "temperature": temp.value,
            "humidity": hum.value,
            "pressure": pres.value,
        }
        topic = f"floatingsensor/{DEVICE_ID}/telemetry"
        ok = _mqtt.publish(topic, payload)
        result_label.set_text(
            f"✓ Published to {topic}" if ok else "✗ Publish failed"
        )

    ui.button("Send telemetry", on_click=send).classes("bg-green-500 text-white")
    result_label


ui.run(host="0.0.0.0", port=8080, title="Floating Sensor")
