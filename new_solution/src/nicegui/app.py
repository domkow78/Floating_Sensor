"""NiceGUI MVP application — Dashboard + ESP32 Simulator."""

import datetime
import sys
from pathlib import Path
from zoneinfo import ZoneInfo

sys.path.insert(0, str(Path(__file__).parent.parent))

import requests
from nicegui import ui

from config.settings import API_BASE, DEVICE_ID, MQTT_BROKER_HOST, MQTT_BROKER_PORT
from mqtt.client import MqttClient

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


def _format_last_seen(ts: str | None) -> str:
    if not ts:
        return "—"
    try:
        dt_utc = datetime.datetime.strptime(ts, "%Y-%m-%dT%H:%M:%SZ").replace(
            tzinfo=datetime.timezone.utc
        )
        dt_local = dt_utc.astimezone(ZoneInfo("Europe/Warsaw"))
        return dt_local.strftime("%Y-%m-%d %H:%M:%S %Z")
    except Exception:
        return ts


def _utc_now() -> datetime.datetime:
    return datetime.datetime.now(datetime.timezone.utc)


def _utc_z(dt: datetime.datetime) -> str:
    return dt.strftime("%Y-%m-%dT%H:%M:%SZ")


# ── Dashboard page ─────────────────────────────────────────────────────────────

@ui.page("/")
def dashboard():
    ui.label("Floating Sensor — Dashboard").classes("text-2xl font-bold mb-4")
    ui.link("Simulator →", "/simulator").classes("text-blue-500 mb-4")

    tabs = ui.tabs().classes("w-full")
    dashboard_tab = ui.tab("Dashboard")
    history_tab = ui.tab("Historia")
    status_tab = ui.tab("Status")
    info_tab = ui.tab("Informacje")

    with ui.tab_panels(tabs, value=dashboard_tab).classes("w-full"):
        with ui.tab_panel(dashboard_tab):
            status_label = ui.label("Status: —")
            temp_label = ui.label("Temperature: —")
            hum_label = ui.label("Humidity: —")
            pres_label = ui.label("Pressure: —")
            ts_label = ui.label("Last seen: —")

        with ui.tab_panel(history_tab):
            ui.label("Historia telemetry").classes("text-lg font-semibold mb-2")
            with ui.row().classes("items-center gap-2"):
                hours_input = ui.number("Zakres (h)", value=6, min=1, max=168, step=1)
                limit_input = ui.number("Limit", value=100, min=1, max=5000, step=1)
                history_refresh_button = ui.button("Refresh history")
            history_status_label = ui.label("History: —")
            history_table = ui.table(
                columns=[
                    {"name": "timestamp", "label": "Timestamp", "field": "timestamp", "align": "left"},
                    {"name": "temperature", "label": "Temp [C]", "field": "temperature", "align": "right"},
                    {"name": "humidity", "label": "Hum [%]", "field": "humidity", "align": "right"},
                    {"name": "pressure", "label": "Pressure [hPa]", "field": "pressure", "align": "right"},
                ],
                rows=[],
                row_key="timestamp",
                pagination={"rowsPerPage": 10},
            ).classes("w-full")

        with ui.tab_panel(status_tab):
            ui.label("Status systemu").classes("text-lg font-semibold mb-2")
            api_status_label = ui.label("API: —")
            mqtt_status_label = ui.label("MQTT: —")
            influx_status_label = ui.label("InfluxDB: —")
            status_ts_label = ui.label("Status timestamp: —")
            device_state_label = ui.label("Device state: —")

        with ui.tab_panel(info_tab):
            ui.label("Informacje o urzadzeniu").classes("text-lg font-semibold mb-2")
            info_device_id_label = ui.label("Device ID: —")
            info_firmware_label = ui.label("Firmware: —")
            info_last_seen_label = ui.label("Last seen: —")

    def refresh_dashboard() -> None:
        data = _get(f"/telemetry/latest?device_id={DEVICE_ID}")
        if data and data.get("status") == "success":
            d = data["data"]
            temp_label.set_text(f"Temperature: {d.get('temperature', '—')} °C")
            hum_label.set_text(f"Humidity: {d.get('humidity', '—')} %")
            pres_label.set_text(f"Pressure: {d.get('pressure', '—')} hPa")
            ts_label.set_text(f"Last seen: {_format_last_seen(d.get('timestamp'))}")
            status_label.set_text("Status: online")
        else:
            status_label.set_text("Status: no data")

    def refresh_history() -> None:
        hours = int(hours_input.value or 6)
        limit = int(limit_input.value or 100)
        to_ts = _utc_z(_utc_now())
        from_ts = _utc_z(_utc_now() - datetime.timedelta(hours=hours))

        data = _get(
            f"/telemetry/history?device_id={DEVICE_ID}&from={from_ts}&to={to_ts}&limit={limit}"
        )
        if data and data.get("status") == "success":
            rows = []
            for item in data.get("data", []):
                rows.append(
                    {
                        "timestamp": _format_last_seen(item.get("timestamp")),
                        "temperature": item.get("temperature", "—"),
                        "humidity": item.get("humidity", "—"),
                        "pressure": item.get("pressure", "—"),
                    }
                )
            history_table.rows = rows
            history_table.update()
            history_status_label.set_text(f"History: {len(rows)} records")
        else:
            history_status_label.set_text("History: request failed")

    def refresh_status() -> None:
        status_data = _get("/status")
        if status_data and status_data.get("status") == "success":
            d = status_data.get("data", {})
            api_status_label.set_text(f"API: {d.get('iot_core', '—')}")
            mqtt_status_label.set_text(f"MQTT: {d.get('mqtt', '—')}")
            influx_status_label.set_text(f"InfluxDB: {d.get('influxdb', '—')}")
            status_ts_label.set_text(f"Status timestamp: {_format_last_seen(d.get('timestamp'))}")
        else:
            api_status_label.set_text("API: unavailable")
            mqtt_status_label.set_text("MQTT: unavailable")
            influx_status_label.set_text("InfluxDB: unavailable")
            status_ts_label.set_text("Status timestamp: —")

        device_data = _get(f"/device?device_id={DEVICE_ID}")
        if device_data and device_data.get("status") == "success":
            d = device_data.get("data", {})
            device_state_label.set_text(f"Device state: {d.get('state', '—')}")
            info_device_id_label.set_text(f"Device ID: {d.get('device_id', '—')}")
            info_firmware_label.set_text(f"Firmware: {d.get('firmware_version', '—')}")
            info_last_seen_label.set_text(f"Last seen: {_format_last_seen(d.get('last_seen'))}")
        else:
            device_state_label.set_text("Device state: unavailable")
            info_device_id_label.set_text("Device ID: —")
            info_firmware_label.set_text("Firmware: —")
            info_last_seen_label.set_text("Last seen: —")

    history_refresh_button.on_click(refresh_history)
    ui.button("Refresh all", on_click=lambda: (refresh_dashboard(), refresh_status())).classes("mt-4")

    ui.timer(10.0, lambda: (refresh_dashboard(), refresh_status()))
    refresh_dashboard()
    refresh_status()
    refresh_history()


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
