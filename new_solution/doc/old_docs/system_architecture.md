# Architektura Systemu - Floating Sensor Hub

## 📋 Przegląd Ogólny

**Floating Sensor Hub** to system zbierania i przetwarzania danych z pojedynczego urządzenia IoT komunikującego się z centralnym hubem za pośrednictwem protokołu MQTT. W aktualnej wersji pomiary bazują na czujnikach BME688 i LSM6DSV (bez pomiaru impedancji). System przechowuje dane w bazie InfluxDB i udostępnia je poprzez webowy interfejs Streamlit.

---

## ⚙️ Architektura Sprzętu (Hardware)

### **Procesor Główny**
- **Mikrokontroler**: ESP32-S3-WROOM
- **Procesor**: Xtensa dual-core 32-bit @ 240 MHz
- **Pamięć RAM**: 512 KB SRAM
- **Flash**: 4 MB (konfiguralne OTA)
- **WiFi**: 802.11b/g/n (2.4 GHz)
- **Bluetooth**: 5.0 (BLE)
- **GPIO**: 45 pinów dla peryferiów

### **Czujniki (SPI Interface)**

#### **1. BME688 - Czujnik Środowiskowy**
```
Pomiary:
- Temperatura: -40°C do +85°C (±1°C)
- Wilgotność: 0% do 100% (±3%)
- Ciśnienie: 300 hPa do 1100 hPa (±1%)
- Indeks VOC (Volatile Organic Compounds): 0-500

Interfejs: SPI
Adresacja: 0x76 (I2C) / CS pin (SPI)
Pobór mocy: 1.8 - 2.1 mA (operacyjny)
```

#### **2. LSM6DSV - Akcelerometr & Żyroskop (IMU)**
```
Pomiary:
- Akcelerometr: ±2g, ±4g, ±8g, ±16g (selektywne zakresy)
- Żyroskop: ±125°/s, ±250°/s, ±500°/s, ±1000°/s, ±2000°/s

Interfejs: SPI
Adresacja: 0x6A / 0x6B (I2C) / CS pin (SPI)
Pobór mocy: 1.4 mA (operacyjny)

Zastosowania:
- Detekcja ruchu i orientacji
- Analiza wibracji (dryer drum monitoring)
- Kalibracja temperatury dla BME688
```

### **Zarządzanie Energią**

#### **Kontroler ładowania: BQ25616J**
```
Funkcje:
- Nabijanie LiIon/LiPo akumulatorów
- Maksymalny prąd ładowania: 3 A
- Wejście VBUS z USB-C na pin VBUS układu BQ25616J
- Generowanie linii VSYS (zasilanie pośrednie)
- Monitoring napięcia akumulatora
- Ochrona przed przełądowaniem i rozładowaniem

Interfejs: I2C (adres domyślny 0x6B)
Pobór mocy: ~15 mA (standby)
```

#### **Stabilizator 3.3V: TLV75733**
```
Funkcje:
- Stabilizacja napięcia 3.3V dla logiki i czujników
- Wejście z linii VSYS

Wejście: VSYS
Wyjście: 3.3V (ESP32-S3, BME688, LSM6DSV)
```

#### **Źródło Energii: LiIon 3.7V**
```
Typ: LiIon Lithium-Ion (1S1P)
Napięcie nominalne: 3.7V
Zakres: 3.0V (rozładowany) - 4.2V (naładowany)
Funkcje:
- Zasilanie BQ25616J (VBAT) i szyny VSYS
- 3.3V generowane przez TLV75733 z VSYS
- Monitoring napięcia dla danych diagnostycznych
- Typowy czas pracy: 24-48h (w zależności od konfiguracji)
```

### **Komunikacja: USB-C**
```
Funkcje:
- Ładowanie akumulatora
- Linia VBUS (5V) podłączona do wejścia VBUS układu BQ25616J
- Serial/Debug: UART0 (RX/TX)
- ESP32-S3 posiada wbudowany USB-JTAG kontroler
- Flashing firmware OTA

Złącze: USB-C (reversible)
Interfejs: USB 2.0 High-Speed
```

### **Schemat Połączeń SPI**

```
ESP32-S3 SPI Pins (Standard VSPI):
┌─────────────────────┐
│   ESP32-S3-WROOM   │
├─────────────────────┤
│  Pin 18 (CLK)    ──┼─── Wspólny CLK
│  Pin 23 (MOSI)   ──┼─── Wspólny MOSI
│  Pin 19 (MISO)   ──┼─── Wspólny MISO
│                     │
│  Pin 10 (CS₀)    ──┼─── BME688 CS
│  Pin 11 (CS₁)    ──┼─── LSM6DSV CS
│                     │
│  Pin 3 (RX0)     ──┼─── USB-C RX
│  Pin 1 (TX0)     ──┼─── USB-C TX
│                     │
│  Pin 4 (SDA)     ──┼─── BQ25616J SDA (I2C)
│  Pin 5 (SCL)     ──┼─── BQ25616J SCL (I2C)
└─────────────────────┘
       │ │
       │ └──────────────────────────────┐
       │                                 │
   ┌───┴───┐                    ┌───────┴──┐
   │ BME688│                    │LSM6DSV  │
  (SPI)    │                   (SPI)      │
   └───────┘                    └──────────┘
```

### **Schemat Zasilania**

```
   USB-C (VBUS)
             │
             ▼
      ┌─────────────┐
      │ BQ25616J    │
      │ (Charger)   │
      └──────┬──────┘
             │
   ┌───────┴───────┐
   │   VSYS        │
   └───────┬───────┘
      │
      ┌────▼────┐
      │TLV75733 │
      │  (LDO)  │
      └────┬────┘
      │ 3.3V
      ├─────────┬─────────┬─────────┐
      │         │         │         │
          ▼         ▼         ▼         ▼
       (ESP32)   (BME688)   (LSM6DSV) (inne)
        
   ┌────────┐
   │LiIon   │
   │3.7V    │
        └────────┘
```

---

## 🏗️ Architektura Systemu

```
┌─────────────────────────────────────┐
│   Floating Sensor Node (ESP32-S3)   │
│  ┌─────────────────────────────────┐│
│  │ Czujniki (SPI)                  ││
│  │ • BME688: T, H, P, VOC         ││
│  │ • LSM6DSV: Acc, Gyro           ││
│  │ • BQ25616J: Zarząd. energią    ││
│  │ • TLV75733: Regulator 3.3V     ││
│  └─────────────────────────────────┘│
│  └─────────────────────────────────┐│
│  │ Zasilanie                       ││
│  │ • LiIon 3.7V (akumulator)       ││
│  │ • USB-C ładowanie               ││
│  └─────────────────────────────────┘│
└──────────────┬──────────────────────┘
               │ WiFi / MQTT
               ▼
┌──────────────────────────────────────┐
│   MQTT Broker (Mosquitto)            │
│   - Topik: sensor/node_001/measurement│
│   - Topik: sensor/node_001/diagnostic │
│   - Topik: cmd/node_001/request       │
└──────────────┬───────────────────────┘
               │
               ▼
┌──────────────────────────────────────┐
│   Floating Sensor Hub (Python)       │
│   - Odbieranie komunikatów MQTT      │
│   - Parsowanie payloadu              │
│   - Przechowywanie danych            │
│   - Obsługa komend                   │
└──────────────┬───────────────────────┘
               │
               ▼
┌──────────────────────────────────────┐
│   InfluxDB v2.7 (Baza Szeregów Czasu)│
│   - Bucket: measurements             │
│   - Bucket: diagnostics              │
└──────────────┬───────────────────────┘
               │
               ▼
┌──────────────────────────────────────┐
│   Webowy Interfejs (Streamlit)       │
│   - Wizualizacja danych              │
│   - Wykresy i tabele                 │
│   - Analiza trendów                  │
└──────────────────────────────────────┘
```

---

## 🔄 Przepływ Danych

### 1. **Faza Zbierania Danych**
- Pojedynczy czujnik IoT zbiera pomiary (temperatura, wilgotność, ciśnienie, VOC oraz dane IMU)
- Dane pakowane są w JSON i wysyłane do MQTT brokera
- Czujnik wysyła dwie kategorie komunikatów:
  - **measurement** - dane pomiarowe
  - **diagnostic** - dane diagnostyczne (V, RSSI, FW, ostatnia komenda)

### 2. **Faza Przetwarzania w Hubie**
```
MQTT Message (JSON)
       │
       ▼
  PayloadParser
  (walidacja i parsowanie)
       │
       ▼
  InfluxStore
  (zapis do bazy danych)
```

### 3. **Faza Przechowywania**
- Dane zapisywane są w InfluxDB w dwóch bucketa:
  - `measurements` - dane pomiarowe (temperatura, wilgotność, ciśnienie, itp.)
  - `diagnostics` - dane diagnostyczne (napięcie, RSSI, firmware)

### 4. **Faza Wizualizacji**
- Streamlit odczytuje dane z InfluxDB
- Wyświetla wykresy, tabele i statystyki
- Umożliwia interaktywną analizę danych

---

## 📦 Komponenty Systemu

### **1. FloatingSensorHub (main.py)**
Główna klasa aplikacji odpowiadająca za:
- Inicjalizację wszystkich komponentów
- Zarządzanie cyklem życia aplikacji (start/stop)
- Rejestrowanie handlera zdarzeń
- Synchronizacja komponentów

```python
class FloatingSensorHub:
    - PayloadParser     # parsowanie JSON
    - InfluxStore       # przechowywanie danych
    - MqttClient        # komunikacja MQTT
    - Commander         # obsługa komend
```

### **2. MQTT Client (mqtt/client.py)**
- Nawiązanie połączenia z brokerem Mosquitto
- Subskrybowanie tematów:
   - `sensor/node_001/measurement`
   - `sensor/node_001/diagnostic`
- Publikowanie komend do czujników
- Obsługa reconnect z opóźnieniem eksponencjalnym

### **3. Payload Parser (parser/payload_parser.py)**
- Walidacja przychodzących komunikatów JSON
- Mapowanie pól na jednostki (°C, %, Pa, V, dBm, itp.)
- Sprawdzenie wymaganych pól
- Konwersja typów danych (int, float, string)

### **4. InfluxStore (storage/influx_store.py)**
- Połączenie z bazą InfluxDB
- Zapis pomiarów do bucketa `measurements`
- Zapis danych diagnostycznych do bucketa `diagnostics`
- Zarządzanie tagami (measurement_type)

### **5. Commander (commands/commander.py)**
- Przygotowanie komend dla czujników
- Publikowanie żądań do czujników
- Śledzenie stanu komend

---

## 📊 Schemat Danych

### **Payload Pomiarowy (measurement)**

```yaml
{
  "ts": 1720000000000,          # Timestamp [ms]
  
  # BME688 - Czujnik Środowiskowy
  "t": 23.5,                    # Temperature [°C]
  "h": 45.2,                    # Humidity [%]
  "p": 101325,                  # Pressure [Pa]
  "voc": 145,                   # VOC Index [0-500]
  
  # LSM6DSV - Akcelerometr & Żyroskop
  "acc_x": -0.15,               # Acceleration X [g]
  "acc_y": 0.08,                # Acceleration Y [g]
  "acc_z": 1.02,                # Acceleration Z [g]
  "gyro_x": 2.5,                # Gyroscope X [°/s]
  "gyro_y": -1.2,               # Gyroscope Y [°/s]
  "gyro_z": 0.8                 # Gyroscope Z [°/s]
}
```

### **Payload Diagnostyczny (diagnostic)**

```yaml
{
  "ts": 1720000000000,          # Timestamp [ms]
  
  # Zarządzanie energią
  "batt_v": 4.15,               # Battery voltage [V]
  "batt_soc": 87,               # State of Charge [%]
  "charge_status": "charging",  # Charging status
  
  # Łączność
  "rssi": -65,                  # WiFi RSSI [dBm]
  "ip": "192.168.1.100",        # IP address
  
  # Systemowe
  "fw": "v2.1.0",               # Firmware version
  "uptime": 3600000,            # Uptime [ms]
  "heap_free": 204800,          # Free heap [bytes]
  "cpu_temp": 45.2              # CPU temperature [°C]
}
```

---

## 🔌 Topiki MQTT

### **Publikowanie (Czujniki → Hub)**

| Topik | Format | Opis |
|-------|--------|------|
| `sensor/node_001/measurement` | JSON | Dane pomiarowe |
| `sensor/node_001/diagnostic` | JSON | Dane diagnostyczne |

### **Subskrybowanie (Hub → Czujniki)**

| Topik | Format | Opis |
|-------|--------|------|
| `cmd/node_001/request` | JSON | Żądanie wykonania komendy |
| `cmd/node_001/response` | JSON | Odpowiedź na komendę |

---

## 🐳 Infrastruktura Docker

System uruchamiany jest w kontenerach Docker za pomocą docker-compose:

### **Usługi:**

1. **mosquitto** (port 1883)
   - MQTT Broker
   - Konfiguracja: `./mosquitto/mosquitto.conf`
   - Persistence włączony

2. **influxdb** (port 8086)
   - Czasowa baza danych
   - Inicjalizacja z admin/adminpassword
   - Token: `my-super-secret-token`
   - Organizacja: `floating_sensor`
   - Buckety: `measurements`, `diagnostics`

3. **app** (port 8501)
   - Główna aplikacja Python
   - Streamlit webowy interfejs
   - Zbudowana z `Dockerfile`
   - Środowisko:
     - `INFLUXDB_TOKEN`
     - `MQTT_BROKER`
     - `INFLUXDB_URL`

### **Sieć:**
- Sieć `sensor_net` dla komunikacji między kontenerami
- Host dostępny na portach: 1883 (MQTT), 8086 (InfluxDB), 8501 (UI)

---

## 📝 Konfiguracja (config.yaml)

Plik `config.yaml` zawiera:

- **MQTT**: adres brokera, port, timeout
- **InfluxDB**: URL, token, organizacja, buckety
- **Schemat Payloadu**: definicja pól pomiarowych i diagnostycznych

### **Schemat Payloadu (Zaktualizowany)**

```yaml
payload_schema:
  measurement:
    # BME688
    - { key: "t",     type: float, unit: "°C",   required: true,  description: "Temperature" }
    - { key: "h",     type: float, unit: "%",    required: true,  description: "Humidity" }
    - { key: "p",     type: int,   unit: "Pa",   required: true,  description: "Pressure" }
    - { key: "voc",   type: int,   unit: "-",    required: false, description: "VOC Index" }
    
    # LSM6DSV
    - { key: "acc_x", type: float, unit: "g",    required: true,  description: "Acceleration X" }
    - { key: "acc_y", type: float, unit: "g",    required: true,  description: "Acceleration Y" }
    - { key: "acc_z", type: float, unit: "g",    required: true,  description: "Acceleration Z" }
    - { key: "gyro_x",type: float, unit: "°/s",  required: false, description: "Gyroscope X" }
    - { key: "gyro_y",type: float, unit: "°/s",  required: false, description: "Gyroscope Y" }
    - { key: "gyro_z",type: float, unit: "°/s",  required: false, description: "Gyroscope Z" }
  
  diag:
    - { key: "ts",          type: int,   unit: "ms",    required: true,  description: "Timestamp" }
    - { key: "batt_v",      type: float, unit: "V",     required: true,  description: "Battery voltage" }
    - { key: "batt_soc",    type: int,   unit: "%",     required: false, description: "Battery SoC" }
    - { key: "charge_status",type: str, unit: "-",      required: false, description: "Charging status" }
    - { key: "rssi",        type: int,   unit: "dBm",   required: false, description: "WiFi RSSI" }
    - { key: "ip",          type: str,   unit: "-",     required: false, description: "IP address" }
    - { key: "fw",          type: str,   unit: "-",     required: false, description: "Firmware version" }
    - { key: "uptime",      type: int,   unit: "ms",    required: false, description: "Uptime" }
    - { key: "heap_free",   type: int,   unit: "bytes", required: false, description: "Free heap" }
    - { key: "cpu_temp",    type: float, unit: "°C",    required: false, description: "CPU temperature" }
```

---

## 🔄 Cykl Życia Aplikacji

```
1. Inicjalizacja
   ├─ Załadowanie config.yaml
   ├─ Połączenie z InfluxDB
   └─ Połączenie z MQTT brokerem

2. Działanie
   ├─ Słuchanie komunikatów MQTT
   ├─ Parsowanie i walidacja danych
   └─ Zapis do InfluxDB

3. Obsługa Sygnałów
   ├─ SIGTERM
   ├─ SIGINT (Ctrl+C)
   └─ Graceful shutdown

4. Zamknięcie
   ├─ Rozłączenie MQTT
   └─ Zamknięcie połączenia InfluxDB
```

---

## 🛡️ Bezpieczeństwo

### **Rekomendacje:**

1. **Zmień domyślne hasła:**
   - InfluxDB: zmień `adminpassword`
   - Token powinien być bezpiecznym stringiem

2. **Zmienne Środowiskowe:**
   - Nie przechowuj poufnych danych w `docker-compose.yml`
   - Użyj `.env` file lub secrets management

3. **MQTT:**
   - Rozważ aktywowanie autentykacji
   - Wdrożenie SSL/TLS dla produkcji

4. **Sieci:**
   - Użyj `docker network` zamiast expose na host
   - Uwaga na porty publiczne w produkcji

---

## 🔧 Inicjalizacja Sprzętu (Startup Sequence)

### **Kolejność inicjalizacji na ESP32-S3:**

```
1. Boot ESP32-S3
   │
   ├─ Inicjalizacja UART0 (USB-C logging)
   │
   ├─ Inicjalizacja SPI Bus
   │  ├─ Setup: CLK (Pin 18), MOSI (Pin 23), MISO (Pin 19)
   │  └─ Frequency: 10 MHz (standard dla czujników)
   │
   ├─ Inicjalizacja BME688 (SPI CS Pin 10)
   │  ├─ Soft reset
   │  ├─ Konfiguracja: T, H, P, VOC readings
   │  └─ Status: OK
   │
   ├─ Inicjalizacja LSM6DSV (SPI CS Pin 11)
   │  ├─ Soft reset
   │  ├─ Konfiguracja: Accelerometer ±8g, Gyroscope ±250°/s
   │  └─ Status: OK
   │
   ├─ Inicjalizacja I2C Bus (4, 5)
   │  └─ Frequency: 400 kHz
   │
   ├─ Inicjalizacja BQ25616J (I2C, 0x6B)
   │  ├─ Readout: Battery voltage, SoC, Charging status
   │  └─ Status: OK
   │
   ├─ Inicjalizacja WiFi
   │  ├─ Connect to AP
   │  └─ Status: OK / Retry
   │
   └─ Inicjalizacja MQTT
      └─ Subscribe: cmd/node_001/request
         Publish: sensor/node_001/measurement
                  sensor/node_001/diagnostic

2. Main Loop
   │
   ├─ [TIMER] Czytaj sensory (125 Hz)
   │  ├─ BME688: T, H, P, VOC
   │  └─ LSM6DSV: Acc, Gyro (raw data, FIFO)
   │
   ├─ [TIMER] Agreguj dane (10 Hz)
   │  ├─ Average/Filter IMU readings
   │  ├─ Czytaj BQ25616J: Battery status
   │  └─ Pack JSON payload
   │
   ├─ [TIMER] Publikuj dane (1 Hz)
   │  ├─ MQTT: measurement + diagnostic
   │  └─ WiFi: retry if needed
   │
   └─ [EVENT] Handle MQTT commands
      └─ Execute: calibrate, sleep, update config
```

### **Drivers i Biblioteki**

```
ESP-IDF (v5.0+)
├─ esp_wifi.h      - WiFi connectivity
├─ driver/spi_master.h - SPI Interface
├─ driver/i2c_master.h - I2C Interface
├─ driver/uart.h    - USB Serial
├─ esp_adc.h        - ADC (battery monitoring)
│
Arduino ESP32 (alternative)
├─ WiFi.h
├─ SPI.h
└─ Wire.h (I2C)

Third-party Drivers
├─ BME68x-Sensor-API (Bosch official)
├─ LSM6DSV_StmSensorsMotionLibrary (STM official)
└─ INA219 (if used for current monitoring)
```

---

## 📈 Rozszerzalność

System jest zaprojektowany do łatwego rozszerzania:

1. **Nowe Typy Czujników** → Update `config.yaml`
2. **Nowe Komenda** → Dodaj w `commands/`
3. **Nowy Interfejs UI** → Dodaj w nowym module
4. **Integracje** → Rozszerzenie `InfluxStore` lub nowe handlery MQTT

---

## � Rozszerzalność

System jest zaprojektowany do łatwego rozszerzania:

1. **Nowe Czujniki** → Dodaj nowy na SPI/I2C, update `config.yaml`
2. **Nowe Komenda** → Dodaj w `commands/`
3. **Nowy Interfejs UI** → Dodaj w nowym module
4. **Integracje** → Rozszerzenie `InfluxStore` lub nowe handlery MQTT

---

## ⚡ Specyfikacja Zasilania

### **Pobór Mocy (typowe scenariusze)**

| Scenariusz | Prąd | Napięcie | Moc | Czas |
|-----------|------|----------|-----|------|
| **Sleep (Deep)** | 15 µA | 3.7V | 55.5 µW | - |
| **WiFi Idle** | 80 mA | 3.7V | 296 mW | ~15h (2000 mAh) |
| **WiFi TX** | 200 mA | 3.7V | 740 mW | ~2.7h |
| **WiFi TX + IMU** | 220 mA | 3.7V | 814 mW | ~2.4h |
| **WiFi TX + All Sensors** | 250 mA | 3.7V | 925 mW | ~2.1h |
| **Charging (BQ25616J)** | 500 mA - 3000 mA | 5V (USB) | 2500 mW | 2-6h |

### **Kalkulacja Czasu Pracy**

Przy standardowej konfiguracji (publikacja 1 Hz, wszystkie sensory aktywne):

```
Akumulator: 2000 mAh @ 3.7V
Średni prąd operacyjny: 120 mA (WiFi + sensory)
Czas pracy: 2000 mAh / 120 mA ≈ 16.7 godzin

Ze spadkami WiFi, sleep w nocy: ~24-48 godzin
```

### **Optymalizacja Mocy**

```
1. Light Sleep: ~10 mA (łączy się szybko)
2. Duty Cycle: Publikuj co 5-10s zamiast co 1s
3. WiFi Sleep: Tą robi BQ25616J - automatycznie
4. IMU FIFO: Zbieraj dane, wyślij raz dziennie
5. Deep Sleep: Hibernacja między pomiarami (mikrosekund)
```

---

## 📋 BOM (Bill of Materials)

| Komponent | Model | Ilość | Uwagi |
|-----------|-------|-------|-------|
| **MCU** | ESP32-S3-WROOM | 1 | Główny procesor |
| **Czujnik Temp/Wilg/Ciś** | BME688 | 1 | SPI, 0x76 |
| **Czujnik Ruchu** | LSM6DSV | 1 | SPI, 6-DOF IMU |
| **Kontroler Ładowania** | BQ25616J | 1 | I2C, 0x6B |
| **Regulator 3.3V** | TLV75733 | 1 | Zasilany z VSYS |
| **Akumulator** | LiIon 3.7V 2000mAh | 1 | 1S1P |
| **Złącze USB** | USB Type-C | 1 | Power + Serial |
| **Rezonator** | 32 MHz | 2 | Dla WiFi/BT |
| **Kondensatory** | 100nF/1µF | - | Decoupling |
| **Rezystory** | - | - | Pull-ups (I2C) |
| **PCB** | - | 1 | Custom design |

---

## 🔄 Komunikacja Danych (szczegółowo)

### **Fase zbierania z czujników ESP32:**

```python
# Odczyt BME688 (SPI)
bme688.read_data()
→ {'t': 23.5, 'h': 45.2, 'p': 101325, 'voc': 145}

# Odczyt LSM6DSV (SPI, FIFO mode)
lsm6dsv.read_fifo()
→ [{'acc': [x,y,z], 'gyro': [x,y,z]}, ...]  # N samples

# Agregacja/Filtracja
→ average/median filters
→ {'acc_x': -0.15, 'acc_y': 0.08, ..., 'gyro_z': 0.8}

# Odczyt BQ25616J (I2C)
bq25616j.read_status()
→ {'batt_v': 4.15, 'batt_soc': 87, 'charge_status': 'charging'}

# Pakowanie JSON
payload = {
  'ts': get_timestamp_ms(),
  'measurement': {...},  # sensor data
  'diagnostic': {...}    # battery + WiFi
}

# Publikacja MQTT
mqtt.publish('sensor/node_001/measurement', json.dumps(payload))
mqtt.publish('sensor/node_001/diagnostic', json.dumps(diagnostic))
```

---

## 🚀 Uruchomienie

### **Hub (Raspberry Pi / Docker)**

```bash
# Przejdź do katalogu aplikacji
cd new_solution/src

# Uruchom całą infrastrukturę
docker-compose up -d

# Sprawdź logi
docker-compose logs -f app

# Zatrzymaj system
docker-compose down
```

Webowy interfejs dostępny: `http://localhost:8501`

### **Czujnik (ESP32-S3)**

```bash
# Using ESP-IDF
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyUSB0 flash
idf.py -p /dev/ttyUSB0 monitor

# Using Arduino IDE
1. Board: ESP32-S3-WROOM
2. Upload Speed: 921600
3. Partition Scheme: Huge App
4. Select Port: COMx (Windows) / /dev/ttyUSBx (Linux)
5. Click Upload
```

### **Konfiguracja WiFi na Czujniku**

```c
// W firmware ESP32:
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const char* mqtt_server = "192.168.1.100";  // IP Hub

WiFi.begin(ssid, password);
while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
}
Serial.println("WiFi connected!");
Serial.println(WiFi.localIP());
```

---

## 📚 Zależności

### **Zależności Hub (Raspberry Pi / Server)**

Szczegółowe zależności Python w `requirements.txt`:

- `paho-mqtt>=2.0.0` - Klient MQTT
- `influxdb-client>=1.36.0` - Interfejs InfluxDB
- `pyyaml>=6.0` - Parsowanie YAML
- `streamlit>=1.35.0` - Webowy interfejs
- `pandas>=2.0.0` - Analiza danych
- `plotly>=5.18.0` - Wizualizacja

### **Zależności ESP32-S3 (Firmware)**

```
Framework: ESP-IDF 5.0+ lub Arduino ESP32 Core 2.0+

Essential Libraries:
- WiFi (esp_wifi.h / WiFi.h)
- SPI (driver/spi_master.h / SPI.h)
- I2C (driver/i2c_master.h / Wire.h)
- MQTT (esp_mqtt_client.h lub PubSubClient)

Sensor Drivers:
- BME68x-Sensor-API (Bosch official)
  https://github.com/BoschSensortec/BME68x-Sensor-API
  
- LSM6DSV (STMicroelectronics)
  https://github.com/STMicroelectronics/LSM6DSV_STdriverSoftware
  
- BQ25616J (Texas Instruments)
  https://github.com/TexasInstruments/BQ25616J-driver

JSON Library:
- ArduinoJson 6.20+ (dla packowania payloadu)
```

---

## 🔍 Debugowanie

### **Monitorowanie Czujnika (Serial Monitor)**

```
[WiFi] Connecting...
[WiFi] Connected! IP: 192.168.1.100
[SPI] BME688 initialized
[I2C] LSM6DSV initialized
[I2C] BQ25616J initialized
[MQTT] Connecting to broker...
[MQTT] Connected! Client ID: sensor_001

[DATA] T=23.5°C H=45.2% P=101325Pa VOC=145
[IMU] Acc=[−0.15g, 0.08g, 1.02g] Gyro=[2.5°/s, −1.2°/s, 0.8°/s]
[BATT] V=4.15V SoC=87% Status=charging
[MQTT] Published measurement
```

### **Sprawdzenie Komunikacji MQTT**

```bash
# Subskrybuj wszystkie topiki czujnika
mosquitto_sub -h localhost -t "sensor/node_001/+" -v

# Output:
# sensor/node_001/measurement {"ts":1720000000000,"t":23.5,"h":45.2,...}
# sensor/node_001/diagnostic {"ts":1720000000000,"batt_v":4.15,...}
```

### **Sprawdzenie InfluxDB**

```bash
# Wejdź do WebUI: http://localhost:8086

# Query Flux:
from(bucket:"measurements")
  |> range(start:-1h)
  |> filter(fn: (r) => r._measurement == "environment")
```

---

## ⚠️ Bezpieczeństwo

### **Hardware Security**

1. **Akumulator**: Nie stawiaj krótko, nie wystawiaj na ekstremalne temperatury
2. **USB-C**: Użyj zatwierdzony zasilacz 5V/2A+
3. **SPI Linie**: Krótkie linie, ekranowanie dla długich tras
4. **ESD**: Uziem się przed pracy z PCB

### **Software Security**

1. **Zmień domyślne hasła:**
   - InfluxDB: zmień `adminpassword`
   - Token powinien być bezpiecznym stringiem

2. **Zmienne Środowiskowe:**
   - Nie przechowuj poufnych danych w `docker-compose.yml`
   - Użyj `.env` file lub secrets management

3. **MQTT:**
   - Rozważ aktywowanie autentykacji
   - Wdrożenie SSL/TLS dla produkcji

4. **Sieci:**
   - Użyj `docker network` zamiast expose na host
   - Uwaga na porty publiczne w produkcji

---

## 📚 Przydatne Linki

- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [BME688 Datasheet](https://www.bosch-sensortec.com/bst/products/all_products/bme688)
- [LSM6DSV Datasheet](https://www.st.com/en/mems-and-sensors/lsm6dsv.html)
- [BQ25616J Datasheet](https://www.ti.com/product/BQ25616J)
- [MQTT Protocol](https://mqtt.org/)
- [InfluxDB Docs](https://docs.influxdata.com/influxdb/latest/)
- [Streamlit Docs](https://docs.streamlit.io/)

