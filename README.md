# Floating Sensor

Zdalny czujnik środowiskowy przeznaczony do pracy w bębnie suszarki przemysłowej lub stacjonarnie w dowolnej lokalizacji.

## 📋 Założenia projektu

### Opis ogólny
**Floating Sensor** to autonomiczny układ elektroniczny wyposażony w zestaw czujników środowiskowych i ruchowych, komunikujący się bezprzewodowo z urządzeniem nadrzędnym (Raspberry Pi) poprzez protokół MQTT.

### Tryby pracy
- **Mobilny** – wewnątrz bębna suszarki przemysłowej (rotujący)
- **Stacjonarny** – zamontowany w dowolnym miejscu

## 🔧 Architektura systemu

```
┌─────────────────────┐         WiFi/MQTT        ┌─────────────────────┐
│   Floating Sensor   │ ◄─────────────────────► │    Raspberry Pi     │
│       (ESP32)       │                          │   (Hub centralny)   │
└─────────────────────┘                          └─────────────────────┘
                                                           │
                                                           ▼
                                                 ┌─────────────────────┐
                                                 │   Baza danych       │
                                                 │ (InfluxDB/PostgreSQL)│
                                                 └─────────────────────┘
                                                           │
                                                           ▼
                                                 ┌─────────────────────┐
                                                 │   Wizualizacja      │
                                                 │ (Grafana/Streamlit) │
                                                 └─────────────────────┘
```

## 🎛️ Hardware - Floating Sensor

### Mikrokontroler
- **ESP32** – główny układ sterujący z wbudowanym WiFi

### Czujniki środowiskowe
| Czujnik | Parametr | Uwagi |
|---------|----------|-------|
| **BME280** | Temperatura, Ciśnienie, Wilgotność względna | I2C, adres 0x76/0x77 |
| **SHT3x** | Temperatura, Wilgotność | Alternatywny czujnik |

### Czujniki ruchu
| Czujnik | Funkcja |
|---------|---------|
| **Żyroskop** | Pomiar prędkości kątowej (obroty bębna) |
| **Akcelerometr** | Pomiar przyspieszenia liniowego, detekcja ruchu |

### Zasilanie
- Akumulator Li-Ion z układem ładowania MCP73831
- Układ ochrony baterii DW01A
- Stabilizator LDO ADP3338 (3.3V)
- Pomiar napięcia baterii przez dzielnik rezystorowy

### Historia wersji HW
- **V9.0** – Aktualna wersja, dwustronna PCB
- **V8.x** – Nowy czujnik ruchu VBS030600, ochrona ESD (SRV05)
- **V7.x** – Ochrona wejść IC, poprawa stabilności programowania
- **V6.x** – Nowy dzielnik napięcia baterii, ochrona DW01A

## 📡 Komunikacja

### Protokół
- **MQTT** – lekki protokół publish/subscribe
- **WiFi** – łączność bezprzewodowa z hub-em

### Struktura topików MQTT
```
ws/<target>/<source>/<smallTopic>
```
Przykład: `ws/rpi1/sensorID1/measurements`

## 🖥️ Backend - Raspberry Pi

### Komponenty
| Komponent | Funkcja |
|-----------|---------|
| **MQTT Broker** | Mosquitto – odbieranie danych z czujników |
| **Baza danych** | InfluxDB (time-series) lub PostgreSQL |
| **Wizualizacja** | Grafana / Streamlit |
| **Monitoring** | Prometheus (opcjonalnie) |
| **Automatyzacja** | Node-RED (opcjonalnie) |

### Przepływ danych
1. Floating Sensor publikuje pomiary przez MQTT
2. Raspberry Pi (broker MQTT) odbiera dane
3. Dane zapisywane do InfluxDB/PostgreSQL
4. Grafana/Streamlit wizualizuje dane w czasie rzeczywistym

## 📁 Struktura projektu

```
Floating_Sensor/
├── old_solution/           # Firmware ESP32 (ESP-IDF)
│   ├── main/               # Kod główny aplikacji
│   ├── components/         # Biblioteki (BME280, SHT3x, ESP-DSP)
│   ├── hardware/           # Dokumentacja sprzętowa
│   ├── deploySmartSensor/  # Skrypty wdrożeniowe
│   ├── grafana/            # Dashboardy Grafana
│   └── node-red/           # Przepływy Node-RED
├── new_solution/           # Nowa wersja (w rozwoju)
├── doc/                    # Dokumentacja
└── src/                    # Źródła dodatkowe
```

## 🚀 Technologie

| Warstwa | Technologia |
|---------|-------------|
| Firmware | C/C++, ESP-IDF v4.4 |
| Komunikacja | WiFi, MQTT |
| Baza danych | InfluxDB / PostgreSQL |
| Wizualizacja | Grafana / Streamlit |
| Monitoring | Prometheus (opcja) |
| Automatyzacja | Node-RED |
| Hardware | ESP32, BME280, Żyroskop, Akcelerometr |

## 📜 Licencja

Kod w repozytorium jest w domenie publicznej (lub na licencji CC0).
