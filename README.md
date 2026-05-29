# Floating Sensor

A remote environmental sensor designed to operate inside an industrial dryer drum or stationary at any location.

## 📋 Project Overview

### General Description
**Floating Sensor** is an autonomous electronic device equipped with a set of environmental and motion sensors, communicating wirelessly with a host device (Raspberry Pi) via the MQTT protocol.

### Operating Modes
- **Mobile** – inside an industrial dryer drum (rotating)
- **Stationary** – mounted at any fixed location

## 🔧 System Architecture

```
┌─────────────────────┐         WiFi/MQTT        ┌─────────────────────┐
│   Floating Sensor   │ ◄─────────────────────► │    Raspberry Pi     │
│       (ESP32)       │                          │   (Central Hub)     │
└─────────────────────┘                          └─────────────────────┘
                                                           │
                                                           ▼
                                                 ┌─────────────────────┐
                                                 │     Database        │
                                                 │ (InfluxDB/PostgreSQL)│
                                                 └─────────────────────┘
                                                           │
                                                           ▼
                                                 ┌─────────────────────┐
                                                 │   Visualization     │
                                                 │ (Grafana/Streamlit) │
                                                 └─────────────────────┘
```

## 🎛️ Hardware - Floating Sensor

### Microcontroller
- **ESP32** – main controller with built-in WiFi

### Environmental Sensors
| Sensor | Parameter | Notes |
|--------|-----------|-------|
| **BME280** | Temperature, Pressure, Relative Humidity | I2C, address 0x76/0x77 |
| **SHT3x** | Temperature, Humidity | Alternative sensor |

### Motion Sensors
| Sensor | Function |
|--------|----------|
| **Gyroscope** | Angular velocity measurement (drum rotation) |
| **Accelerometer** | Linear acceleration measurement, motion detection |

### Power Supply
- Li-Ion battery with MCP73831 charging circuit
- DW01A battery protection circuit
- ADP3338 LDO regulator (3.3V)
- Battery voltage measurement via resistor divider

### HW Version History
- **V9.0** – Current version, double-sided PCB
- **V8.x** – New motion sensor VBS030600, ESD protection (SRV05)
- **V7.x** – IC input protection, improved programming stability
- **V6.x** – New battery voltage divider, DW01A protection

## 📡 Communication

### Protocol
- **MQTT** – lightweight publish/subscribe protocol
- **WiFi** – wireless connectivity to the hub

### MQTT Topic Structure
```
ws/<target>/<source>/<smallTopic>
```
Example: `ws/rpi1/sensorID1/measurements`

## 🖥️ Backend - Raspberry Pi

### Components
| Component | Function |
|-----------|----------|
| **MQTT Broker** | Mosquitto – receiving data from sensors |
| **Database** | InfluxDB (time-series) or PostgreSQL |
| **Visualization** | Grafana / Streamlit |
| **Monitoring** | Prometheus (optional) |
| **Automation** | Node-RED (optional) |

### Data Flow
1. Floating Sensor publishes measurements via MQTT
2. Raspberry Pi (MQTT broker) receives data
3. Data is stored in InfluxDB/PostgreSQL
4. Grafana/Streamlit visualizes data in real time

## 📁 Project Structure (current repository)

```
Floating_Sensor/
├── README.md
├── doc/                               # Additional documentation
├── new_solution/
│   ├── pcb/                           # New hardware PCB files (diagram + gerber)
│   └── src/                           # New Python-based hub/backend stack
│       ├── commands/                  # MQTT command API sent to sensors
│       ├── mosquitto/                 # Mosquitto broker configuration
│       ├── mqtt/                      # MQTT topic helpers + MQTT client wrapper
│       ├── parser/                    # Schema-based payload parser
│       ├── registry/                  # Thread-safe runtime sensor registry
│       ├── storage/                   # InfluxDB write/query layer
│       ├── config.yaml                # Main runtime schema/config
│       ├── docker-compose.yml         # Compose stack: app + mosquitto + influxdb
│       ├── Dockerfile                 # App image definition
│       └── requirements.txt           # Python dependencies
└── old_solution_not_used/             # Legacy ESP-IDF firmware + deployment artifacts
```

## 🔍 Analysis of `new_solution/src` (May 2026)

### Implemented modules

- **MQTT communication layer**
    - Topic builder/parser with convention `ws/<target>/<source>/<smallTopic>`
    - MQTT wrapper with reconnect loop, handler registration, publish API
- **Command layer (`Commander`)**
    - Sensor lifecycle commands (`registerCall`, `startMeas`, `stopMeas`, `statusReq`, `sleepNow`)
    - Runtime configuration commands (`measFreq`, `sendInterval`, `setAcmm`, `pm`, `phase`, clears)
- **Payload parsing**
    - Schema-driven field casting and required-field validation from `config.yaml`
    - Forward-compatible passthrough of unknown fields
- **Sensor runtime registry**
    - In-memory, thread-safe state for active sensors, last measurement and diagnostics
- **Storage layer**
    - InfluxDB v2 write path for measurement and diagnostic payloads
    - Generic Flux query method
- **Container stack**
    - Mosquitto + InfluxDB + app service in Docker Compose

### Configuration currently defined

- MQTT endpoint, client identity and reconnect behavior
- InfluxDB org/buckets/token source (env var)
- Sensor activity timeout
- Message schema for:
    - `measurement`: `ts`, `t`, `h`, `p`, `acmm`, `imp`, `impi`, `impc`, `ph`, `phf`
    - `diag`: `ts`, `V`, `rssi`, `fw`, `lm`

### Gaps identified

- `main.py` now provides a minimal backend entrypoint, but there is still no dedicated UI application file.
- `streamlit` is listed in dependencies and `8501` is exposed, but no UI entry file is present yet.
- `docker-compose.yml` contains example credentials/tokens in plain text (development only; should be moved to env/secrets for production).

## ✅ Current Status

`new_solution` now has a minimal runnable backend entrypoint (`main.py`) that connects MQTT, parsing, sensor registry and InfluxDB persistence. A dedicated UI layer is still missing.

## 🚀 Technologies

| Layer | Technology |
|-------|------------|
| Firmware (legacy) | C/C++, ESP-IDF v4.4 |
| New backend/hub | Python 3.11 |
| Communication | WiFi, MQTT (paho-mqtt) |
| Storage | InfluxDB 2.x (influxdb-client) |
| Containerization | Docker, Docker Compose |
| Visualization (planned/in progress) | Streamlit, Grafana |
| Automation/flows (legacy assets) | Node-RED |
| Hardware | ESP32, BME280/SHT3x, motion sensors |

## 📜 License

Code in this repository is in the public domain (or licensed under CC0).
