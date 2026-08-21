# Floating Sensor

Floating Sensor is an IoT platform for environmental and motion monitoring based on an ESP32-based sensor node and a modular backend stack.

The current project direction follows the architecture defined in the documentation set under [new_solution/doc](new_solution/doc): a clean MVP implementation, with the previous code kept only as a reference prototype in [new_solution/src_ref](new_solution/src_ref).

---

## Project goal

The project aims to build a complete data flow:

Floating Sensor -> MQTT -> Mosquitto -> IoT Core -> InfluxDB -> REST API -> NiceGUI

The system is designed to be modular, testable, and ready for iterative MVP delivery.

---

## Current architecture

### 1. Device layer

- Floating Sensor device based on ESP32
- sensor acquisition via I2C
- WiFi connectivity
- MQTT publishing of telemetry, status and diagnostics

### 2. Message transport

- Mosquitto as the MQTT broker
- topic convention:

  floatingsensor/<device_id>/<topic>

- supported MVP topics:
  - status
  - telemetry
  - diagnostics

### 3. IoT Core

The central backend is structured as a modular Python application in [new_solution/src](new_solution/src). The main responsibilities are:

- MQTT gateway
- data processing and validation
- storage service
- device registry
- REST API layer

### 4. Storage

- InfluxDB for time-series measurement history
- single source of truth for measurement data
- access only through the backend storage service

### 5. Presentation layer

- NiceGUI as the web UI layer
- data access only via REST API
- no direct communication with MQTT or InfluxDB

Current UI scope includes:
- Dashboard tab with latest telemetry preview
- History tab (Influx-backed, newest records first)
- Status and Info tabs
- Advanced Simulator tab for synthetic telemetry generation

---

## Repository structure

```text
Floating_Sensor/
├── README.md
├── .gitignore
├── deployment.md
├── smoke_test.md
├── new_solution/
│   ├── doc/                         # architecture and implementation docs
│   │   ├── 01_...
│   │   ├── ...
│   │   └── 13_...
│   ├── src/                         # new clean implementation target
│   │   ├── app/
│   │   ├── config/
│   │   ├── mqtt/
│   │   ├── processing/
│   │   ├── storage/
│   │   ├── tests/
│   │   ├── requirements.txt
│   │   └── ...
│   ├── src_ref/                    # legacy/prototype reference, frozen
│   ├── pcb/
│   └── ...
├── old_solution_not_used/
└── old_solution/
```

Important rule:

- [new_solution/src_ref](new_solution/src_ref) is a reference prototype only
- [new_solution/src](new_solution/src) is the active target for new implementation

---

## Current implementation status

The new implementation is in early MVP stage. The active source tree contains a minimal working structure for:

- configuration
- MQTT client
- processing logic
- storage abstraction
- smoke tests

Operationally verified behavior:
- telemetry from NiceGUI simulator is published to MQTT topic `floatingsensor/<device_id>/telemetry`
- IoT Core ingests payloads and persists records to InfluxDB
- API history endpoint reads telemetry from InfluxDB
- NiceGUI History tab displays API history records (including accel and gas fields)

The project is deliberately being built iteratively, following the roadmap from the architecture docs.

---

## Run local UI

From [new_solution/src](new_solution/src):

1. Create and activate virtual environment.
2. Install dependencies from `requirements.txt`.
3. Run NiceGUI:

  `python .\nicegui\app.py`

Notes:
- UI can run in UI-only mode when MQTT is unavailable (simulator generation still works, publish is skipped).
- For end-to-end flow, MQTT broker, IoT Core API, and InfluxDB must be running.

---

## Time handling

- Stored telemetry timestamps are UTC (ISO with `Z`).
- If device payload does not include `timestamp`, backend generates UTC timestamp.
- UI displays localized time in `Europe/Warsaw` for Dashboard and History.

---

## History tab semantics

- `Range back from now [h]` defines query window size backward from current UTC time.
- `Max records (limit)` defines the maximum number of returned records.
- Results are ordered newest-first.

---

## Verify InfluxDB writes

IoT Core logs include explicit write confirmation:

- `InfluxDB write OK: bucket=... measurement=... device_id=... timestamp=...`
- `Ingested telemetry and persisted to InfluxDB for ... at ...`

These logs can be used as runtime confirmation that telemetry is persisted.

---

## Active source tree

Current source layout in [new_solution/src](new_solution/src):

```text
src/
├── app/
│   └── main.py
├── config/
│   └── settings.py
├── mqtt/
│   └── client.py
├── processing/
│   └── engine.py
├── storage/
│   └── influx_client.py
├── tests/
│   └── test_smoke.py
├── requirements.txt
└── .venv/                       # local virtual environment, excluded from Git
```

This structure is intentionally minimal and designed to grow with the MVP requirements.

---

## Technology stack

| Layer | Technology |
|------|------------|
| Hardware | ESP32, environmental sensors, motion sensors |
| Messaging | MQTT, Mosquitto |
| Core backend | Python |
| Data processing | custom processing layer |
| Time-series storage | InfluxDB |
| API | FastAPI |
| UI | NiceGUI |
| Runtime | Docker / Docker Compose |
| Testing | pytest |

---

## Documentation

The architecture and project decisions are stored in [new_solution/doc](new_solution/doc), including:

- architecture overview
- MQTT contract
- IoT Core design
- REST API draft
- InfluxDB model
- NiceGUI design
- Docker deployment model
- migration strategy and MVP contracts

This documentation is treated as the source of truth for the implementation direction.

---

## Development workflow

1. Build a small, testable implementation step.
2. Validate behavior with pytest.
3. Update documentation when interfaces or contracts change.
4. Keep the prototype reference separate from the clean implementation.

---

## Roadmap

Planned MVP flow:

1. firmware and MQTT publishing
2. IoT Core MQTT gateway
3. processing and validation
4. storage in InfluxDB
5. REST API endpoints
6. NiceGUI dashboard
7. Docker Compose integration
8. end-to-end smoke test

---

## Notes

- The old implementation remains available in [new_solution/src_ref](new_solution/src_ref) only as a reference.
- The active implementation target is [new_solution/src](new_solution/src).
- The system is being developed iteratively, not by copying the legacy structure 1:1.

---

## License

Project documentation and code are maintained in the repository as the working project state. The exact licensing model should be confirmed before public distribution.
