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

## 📁 Project Structure

```
Floating_Sensor/
├── old_solution/           # ESP32 Firmware (ESP-IDF)
│   ├── main/               # Main application code
│   ├── components/         # Libraries (BME280, SHT3x, ESP-DSP)
│   ├── hardware/           # Hardware documentation
│   ├── deploySmartSensor/  # Deployment scripts
│   ├── grafana/            # Grafana dashboards
│   └── node-red/           # Node-RED flows
├── new_solution/           # New version (in development)
├── doc/                    # Documentation
└── src/                    # Additional sources
```

## 🚀 Technologies

| Layer | Technology |
|-------|------------|
| Firmware | C/C++, ESP-IDF v4.4 |
| Communication | WiFi, MQTT |
| Database | InfluxDB / PostgreSQL |
| Visualization | Grafana / Streamlit |
| Monitoring | Prometheus (optional) |
| Automation | Node-RED |
| Hardware | ESP32, BME280, Gyroscope, Accelerometer |

## 📜 License

Code in this repository is in the public domain (or licensed under CC0).
