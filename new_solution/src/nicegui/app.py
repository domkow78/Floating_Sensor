"""NiceGUI MVP application — Dashboard + ESP32 Simulator."""

import datetime
import math
import sys
import time
from pathlib import Path
from zoneinfo import ZoneInfo

sys.path.insert(0, str(Path(__file__).parent.parent))

import requests
from nicegui import run, ui

from config.settings import API_BASE, DEVICE_ID, MQTT_BROKER_HOST, MQTT_BROKER_PORT
from mqtt.client import MqttClient

_mqtt = None
try:
    _mqtt = MqttClient(host=MQTT_BROKER_HOST, port=MQTT_BROKER_PORT, client_id="nicegui-simulator")
    _mqtt.connect()
    _mqtt.start_loop()
except Exception:
    # UI-only mode: allow rendering without live MQTT infrastructure.
    _mqtt = None


# ── helpers ───────────────────────────────────────────────────────────────────

def _get(path: str) -> dict | None:
    try:
        r = requests.get(f"{API_BASE}{path}", timeout=0.8)
        r.raise_for_status()
        return r.json()
    except Exception:
        return None


async def _get_async(path: str) -> dict | None:
    return await run.io_bound(_get, path)


def _format_last_seen(ts: str | None) -> str:
    if not ts:
        return "—"
    try:
        # Accept both strict Z format and ISO timestamps with fractional seconds.
        if ts.endswith("Z"):
            dt_utc = datetime.datetime.fromisoformat(ts.replace("Z", "+00:00"))
        else:
            dt_utc = datetime.datetime.fromisoformat(ts)

        if dt_utc.tzinfo is None:
            dt_utc = dt_utc.replace(tzinfo=datetime.timezone.utc)
        dt_local = dt_utc.astimezone(ZoneInfo("Europe/Warsaw"))
        return dt_local.strftime("%Y-%m-%d %H:%M:%S %Z")
    except Exception:
        return ts


def _utc_now() -> datetime.datetime:
    return datetime.datetime.now(datetime.timezone.utc)


def _utc_z(dt: datetime.datetime) -> str:
    return dt.strftime("%Y-%m-%dT%H:%M:%SZ")


def _fmt_number(value, digits: int = 2) -> str:
    if value is None:
        return "—"
    try:
        return f"{float(value):.{digits}f}"
    except (TypeError, ValueError):
        return str(value)


def _render_simulator_controls() -> None:
    ui.label("Advanced simulator: sinusoidal telemetry profiles").classes("text-lg font-semibold mb-2")
    ui.label("Set ranges and period for each field, then start streaming.").classes("text-sm")

    with ui.row().classes("w-full items-end gap-3 mt-2"):
        interval_input = ui.number("Publish interval [s]", value=2.0, min=0.2, max=3600, step=0.1).classes("w-56")
        duration_input = ui.number("Duration [s] (0=infinite)", value=0, min=0, max=86400, step=1).classes("w-64")
        ui.button("Start advanced stream", on_click=lambda: start_advanced()).classes("ml-auto bg-green-500 text-white")
        ui.button("Stop", on_click=lambda: stop_advanced()).classes("bg-red-500 text-white")

    with ui.row().classes("items-center gap-3"):
        firmware_input = ui.input("Firmware", value="fs-sim-advanced").classes("w-56")
        location_input = ui.input("Location", value="lab").classes("w-64")

    field_defaults = {
        "temperature": (20.0, 28.0, 300.0),
        "humidity": (40.0, 60.0, 420.0),
        "pressure": (1002.0, 1015.0, 600.0),
        "gas_resistance": (10000.0, 30000.0, 480.0),
        "accel_x": (-0.3, 0.3, 30.0),
        "accel_y": (-0.3, 0.3, 35.0),
        "accel_z": (9.6, 9.9, 45.0),
    }

    # Layout slots: keep two empty cards after gas_resistance,
    # then place accel_x/accel_y/accel_z on the next row.
    field_slots: list[str | None] = [
        "temperature", "humidity", "pressure",
        "gas_resistance", None, None,
        "accel_x", "accel_y", "accel_z",
    ]

    ranges: dict[str, dict[str, ui.number]] = {}
    with ui.grid(columns=3).classes("w-full gap-2 mt-2"):
        for field_name in field_slots:
            if field_name is None:
                with ui.card().classes("w-full p-2"):
                    ui.label(" ").classes("text-sm")
                continue

            vmin, vmax, period = field_defaults[field_name]
            with ui.card().classes("w-full p-2"):
                ui.label(field_name).classes("font-semibold text-sm")
                with ui.row().classes("items-end gap-2 no-wrap"):
                    min_input = ui.number("min", value=vmin, step=0.1).classes("w-24")
                    max_input = ui.number("max", value=vmax, step=0.1).classes("w-24")
                    period_input = ui.number("period [s]", value=period, min=0, step=0.1).classes("w-28")
                min_input.props("dense outlined")
                max_input.props("dense outlined")
                period_input.props("dense outlined")
            ranges[field_name] = {
                "min": min_input,
                "max": max_input,
                "period": period_input,
            }

    sim_status = ui.label("Advanced simulator: idle")
    sim_last_payload = ui.label("Last payload: —").classes("text-sm")

    simulation_state = {
        "running": False,
        "started_at": 0.0,
        "last_publish_at": 0.0,
        "sent": 0,
    }

    def _sinusoidal_value(elapsed: float, min_value: float, max_value: float, period_s: float) -> float:
        low = min(min_value, max_value)
        high = max(min_value, max_value)
        center = (low + high) / 2.0
        amplitude = (high - low) / 2.0
        if period_s <= 0:
            return center
        phase = (2.0 * math.pi * elapsed) / period_s
        return center + amplitude * math.sin(phase)

    def _build_advanced_payload(elapsed: float) -> dict:
        payload = {
            "device_id": DEVICE_ID,
            "timestamp": datetime.datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ"),
        }
        for field_name, cfg in ranges.items():
            min_val = float(cfg["min"].value)
            max_val = float(cfg["max"].value)
            period_val = float(cfg["period"].value)
            payload[field_name] = round(
                _sinusoidal_value(elapsed, min_val, max_val, period_val),
                4,
            )

        firmware_val = (firmware_input.value or "").strip()
        location_val = (location_input.value or "").strip()
        if firmware_val:
            payload["firmware"] = firmware_val
        if location_val:
            payload["location"] = location_val
        return payload

    def _stop_simulation(reason: str) -> None:
        simulation_state["running"] = False
        sim_timer.active = False
        sim_status.set_text(reason)

    def _tick_simulation() -> None:
        if not simulation_state["running"]:
            return

        now = time.monotonic()
        elapsed = now - simulation_state["started_at"]
        duration_s = float(duration_input.value or 0)
        interval_s = max(0.2, float(interval_input.value or 2.0))

        if duration_s > 0 and elapsed >= duration_s:
            _stop_simulation("Advanced simulator: finished by duration")
            return

        if now - simulation_state["last_publish_at"] < interval_s:
            return

        payload = _build_advanced_payload(elapsed)
        simulation_state["last_publish_at"] = now
        simulation_state["sent"] += 1

        topic = f"floatingsensor/{DEVICE_ID}/telemetry"
        if _mqtt is None:
            sim_status.set_text(
                f"Advanced simulator: generated #{simulation_state['sent']} (UI-only mode)"
            )
        else:
            ok = _mqtt.publish(topic, payload)
            sim_status.set_text(
                f"Advanced simulator: published #{simulation_state['sent']}" if ok
                else "Advanced simulator: publish failed"
            )
        sim_last_payload.set_text(f"Last payload: {payload}")

    sim_timer = ui.timer(0.2, _tick_simulation, active=False)

    def start_advanced() -> None:
        simulation_state["running"] = True
        simulation_state["started_at"] = time.monotonic()
        simulation_state["last_publish_at"] = 0.0
        simulation_state["sent"] = 0
        sim_timer.active = True
        sim_status.set_text("Advanced simulator: running")

    def stop_advanced() -> None:
        _stop_simulation("Advanced simulator: stopped")

# ── Dashboard page ─────────────────────────────────────────────────────────────

@ui.page("/")
def dashboard():
    ui.add_css('''
    .dashboard-field .q-field__control:before,
    .dashboard-field .q-field__control:after {
        border-bottom: none !important;
    }
    .dashboard-field .q-field__bottom {
        display: none !important;
        min-height: 0 !important;
        padding: 0 !important;
    }
    .dashboard-field .q-field__control {
        border-radius: 12px !important;
        background: #e9eef5 !important;
        border: 1px solid #9aa8bc !important;
        box-shadow: 0 6px 14px rgba(25, 35, 52, 0.18) !important;
    }
    .dashboard-field .q-field__native,
    .dashboard-field .q-field__label {
        color: #1f2a3a !important;
    }
    ''')

    ui.label("IoT Environment - Dashboard").classes("text-2xl font-bold mb-4")

    with ui.tabs().classes("w-full") as tabs:
        dashboard_tab = ui.tab("Dashboard")
        history_tab = ui.tab("History")
        status_tab = ui.tab("Status")
        info_tab = ui.tab("Info")
        simulator_tab = ui.tab("Simulator")

    with ui.tab_panels(tabs, value=dashboard_tab).classes("w-full"):
        with ui.tab_panel(dashboard_tab):
            ui.label("Dashboard").classes("text-lg font-semibold mb-2")
            with ui.column().classes("gap-2"):
                dash_status_input = ui.input("Status", value="—").classes("w-80 dashboard-field")
                dash_device_id_input = ui.input("Device ID", value=DEVICE_ID).classes("w-80 dashboard-field")
                dash_timestamp_input = ui.input("Timestamp (Europe/Warsaw)", value="—").classes("w-80 dashboard-field")
                dash_temperature_input = ui.input("Temperature [C]", value="—").classes("w-80 dashboard-field")
                dash_humidity_input = ui.input("Humidity [%]", value="—").classes("w-80 dashboard-field")
                dash_pressure_input = ui.input("Pressure [hPa]", value="—").classes("w-80 dashboard-field")
                dash_gas_input = ui.input("Gas resistance", value="—").classes("w-80 dashboard-field")
                dash_accel_x_input = ui.input("Accel X", value="—").classes("w-80 dashboard-field")
                dash_accel_y_input = ui.input("Accel Y", value="—").classes("w-80 dashboard-field")
                dash_accel_z_input = ui.input("Accel Z", value="—").classes("w-80 dashboard-field")
                dash_firmware_input = ui.input("Firmware", value="—").classes("w-80 dashboard-field")
                dash_location_input = ui.input("Location", value="—").classes("w-80 dashboard-field")

            for field in (
                dash_status_input,
                dash_device_id_input,
                dash_timestamp_input,
                dash_temperature_input,
                dash_humidity_input,
                dash_pressure_input,
                dash_gas_input,
                dash_accel_x_input,
                dash_accel_y_input,
                dash_accel_z_input,
                dash_firmware_input,
                dash_location_input,
            ):
                field.props("readonly filled dense")

        with ui.tab_panel(history_tab):
            ui.label("Telemetry history").classes("text-lg font-semibold mb-2")
            ui.label("Displayed time zone: Europe/Warsaw").classes("text-sm text-gray-600 mb-1")
            ui.label("Range defines how many hours back from now are queried. Limit defines max returned records.").classes("text-sm text-gray-600 mb-2")
            with ui.row().classes("items-center gap-2"):
                hours_input = ui.number("Range back from now [h]", value=6, min=1, max=168, step=1).classes("w-56")
                limit_input = ui.number("Max records (limit)", value=100, min=1, max=5000, step=1).classes("w-64")
                history_refresh_button = ui.button("Refresh history")
            history_status_label = ui.label("History: —")
            history_table = ui.table(
                columns=[
                    {"name": "timestamp", "label": "Timestamp (Europe/Warsaw)", "field": "timestamp", "align": "left", "style": "min-width: 220px"},
                    {"name": "temperature", "label": "Temp [C]", "field": "temperature", "align": "right", "style": "min-width: 95px"},
                    {"name": "humidity", "label": "Hum [%]", "field": "humidity", "align": "right", "style": "min-width: 95px"},
                    {"name": "pressure", "label": "Pressure [hPa]", "field": "pressure", "align": "right", "style": "min-width: 120px"},
                    {"name": "gas_resistance", "label": "Gas", "field": "gas_resistance", "align": "right", "style": "min-width: 110px"},
                    {"name": "accel_x", "label": "Accel X", "field": "accel_x", "align": "right", "style": "min-width: 90px"},
                    {"name": "accel_y", "label": "Accel Y", "field": "accel_y", "align": "right", "style": "min-width: 90px"},
                    {"name": "accel_z", "label": "Accel Z", "field": "accel_z", "align": "right", "style": "min-width: 90px"},
                ],
                rows=[],
                row_key="timestamp",
                pagination={"rowsPerPage": 10},
            ).classes("w-full")

        with ui.tab_panel(status_tab):
            ui.label("System status").classes("text-lg font-semibold mb-2")
            api_status_label = ui.label("API: —")
            mqtt_status_label = ui.label("MQTT: —")
            influx_status_label = ui.label("InfluxDB: —")
            status_ts_label = ui.label("Status timestamp: —")
            device_state_label = ui.label("Device state: —")

        with ui.tab_panel(info_tab):
            ui.label("Device information").classes("text-lg font-semibold mb-2")
            info_device_id_label = ui.label("Device ID: —")
            info_firmware_label = ui.label("Firmware: —")
            info_last_seen_label = ui.label("Last seen: —")

        with ui.tab_panel(simulator_tab):
            _render_simulator_controls()

    with ui.row().classes("items-center gap-2 mt-3"):
        loading_spinner = ui.spinner(size="sm")
        loading_spinner.visible = False
        loading_label = ui.label("").classes("text-sm text-gray-600")

    refresh_state = {
        "busy": False,
        "last_click": 0.0,
    }

    async def refresh_dashboard() -> None:
        data = await _get_async(f"/telemetry/latest?device_id={DEVICE_ID}")
        if data and data.get("status") == "success":
            d = data["data"]
            dash_status_input.set_value("online")
            dash_device_id_input.set_value(str(d.get("device_id", DEVICE_ID)))
            dash_timestamp_input.set_value(_format_last_seen(d.get("timestamp")))
            dash_temperature_input.set_value(str(d.get("temperature", "—")))
            dash_humidity_input.set_value(str(d.get("humidity", "—")))
            dash_pressure_input.set_value(str(d.get("pressure", "—")))
            dash_gas_input.set_value(str(d.get("gas_resistance", "—")))
            dash_accel_x_input.set_value(str(d.get("accel_x", "—")))
            dash_accel_y_input.set_value(str(d.get("accel_y", "—")))
            dash_accel_z_input.set_value(str(d.get("accel_z", "—")))
            dash_firmware_input.set_value(str(d.get("firmware", "—")))
            dash_location_input.set_value(str(d.get("location", "—")))
        else:
            dash_status_input.set_value("no data")
            dash_timestamp_input.set_value("—")
            dash_temperature_input.set_value("—")
            dash_humidity_input.set_value("—")
            dash_pressure_input.set_value("—")
            dash_gas_input.set_value("—")
            dash_accel_x_input.set_value("—")
            dash_accel_y_input.set_value("—")
            dash_accel_z_input.set_value("—")
            dash_firmware_input.set_value("—")
            dash_location_input.set_value("—")

    async def refresh_history() -> None:
        hours = int(hours_input.value or 6)
        limit = int(limit_input.value or 100)
        to_ts = _utc_z(_utc_now())
        from_ts = _utc_z(_utc_now() - datetime.timedelta(hours=hours))

        data = await _get_async(
            f"/telemetry/history?device_id={DEVICE_ID}&from={from_ts}&to={to_ts}&limit={limit}"
        )
        if data and data.get("status") == "success":
            payload = data.get("data", [])
            if isinstance(payload, dict):
                raw_points = payload.get("points", [])
            elif isinstance(payload, list):
                raw_points = payload
            else:
                raw_points = []

            rows = []
            for item in raw_points:
                if not isinstance(item, dict):
                    continue
                rows.append(
                    {
                        "timestamp": _format_last_seen(item.get("timestamp")),
                        "temperature": _fmt_number(item.get("temperature"), 2),
                        "humidity": _fmt_number(item.get("humidity"), 2),
                        "pressure": _fmt_number(item.get("pressure"), 2),
                        "gas_resistance": _fmt_number(item.get("gas_resistance"), 2),
                        "accel_x": _fmt_number(item.get("accel_x"), 3),
                        "accel_y": _fmt_number(item.get("accel_y"), 3),
                        "accel_z": _fmt_number(item.get("accel_z"), 3),
                    }
                )
            history_table.rows = rows
            history_table.update()
            history_status_label.set_text(f"History: {len(rows)} records")
        else:
            history_status_label.set_text("History: request failed")

    async def refresh_status() -> None:
        status_data = await _get_async("/status")
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
            device_state_label.set_text("Device state: unavailable")
            info_device_id_label.set_text("Device ID: —")
            info_firmware_label.set_text("Firmware: —")
            info_last_seen_label.set_text("Last seen: —")
            return

        device_data = await _get_async(f"/device?device_id={DEVICE_ID}")
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

    async def refresh_all() -> None:
        await refresh_dashboard()
        await refresh_status()

    async def refresh_active_tab() -> None:
        active_tab = tabs.value
        if active_tab == dashboard_tab:
            await refresh_dashboard()
            return
        if active_tab == history_tab:
            await refresh_history()
            return
        if active_tab == status_tab or active_tab == info_tab:
            await refresh_status()

    async def run_refresh(action: str, refresh_coro, show_loading: bool = False) -> None:
        now = time.monotonic()
        if refresh_state["busy"]:
            return
        if now - refresh_state["last_click"] < 0.35:
            return

        refresh_state["busy"] = True
        refresh_state["last_click"] = now
        if show_loading:
            loading_spinner.visible = True
            loading_label.set_text(f"Loading: {action}...")
        try:
            await refresh_coro()
        finally:
            if show_loading:
                loading_spinner.visible = False
                loading_label.set_text("")
            refresh_state["busy"] = False

    async def on_history_refresh() -> None:
        await run_refresh("history", refresh_history, show_loading=True)

    async def on_refresh_all() -> None:
        await run_refresh("all", refresh_all, show_loading=True)

    async def on_timer_refresh() -> None:
        await run_refresh("active tab", refresh_active_tab)

    async def on_dashboard_timer_refresh() -> None:
        await run_refresh("dashboard", refresh_dashboard)

    history_refresh_button.on_click(on_history_refresh)
    ui.button("Refresh all", on_click=on_refresh_all).classes("mt-4")

    ui.timer(1.0, on_dashboard_timer_refresh)
    ui.timer(10.0, on_timer_refresh)


# ── Simulator page ─────────────────────────────────────────────────────────────

@ui.page("/simulator")
def simulator():
    ui.label("ESP32 Simulator").classes("text-2xl font-bold mb-4")
    ui.link("← Dashboard", "/").classes("text-blue-500 mb-4")
    _render_simulator_controls()


ui.run(host="0.0.0.0", port=8080, title="IoT Environment - Dashboard")
