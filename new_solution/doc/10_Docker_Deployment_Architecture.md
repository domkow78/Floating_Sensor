
# Rozdział 10 – Architektura wdrożeniowa Docker
**IoT Platform Architecture Specification v1.0**

---

# 10.1 Cel

Docker stanowi standardowe środowisko uruchomieniowe platformy IoT.

Wszystkie komponenty backendowe uruchamiane są jako odizolowane kontenery, co zapewnia
powtarzalność środowiska, prostotę wdrażania oraz łatwość rozwoju.

Firmware ESP32 nie jest częścią środowiska Docker.

---

# 10.2 Decyzje architektoniczne

Przyjęto następujące decyzje:

- Docker jest jedynym sposobem uruchamiania komponentów backendowych.
- Zarządzanie środowiskiem odbywa się przez Docker Compose.
- Każdy komponent posiada własny kontener.
- Wszystkie kontenery komunikują się przez dedykowaną sieć Docker.
- Konfiguracja odbywa się przez zmienne środowiskowe.

---

# 10.3 Architektura wdrożeniowa

```text
                    Floating Sensor
                           │
                        WiFi / MQTT
                           │
                           ▼
                 +--------------------+
                 |     Mosquitto      |
                 +---------+----------+
                           │
                           ▼
                 +--------------------+
                 |      IoT Core      |
                 +----+-----------+---+
                      |           |
                      | REST      | Storage
                      ▼           ▼
              +-------------+  +-----------+
              |  NiceGUI    |  | InfluxDB  |
              +-------------+  +-----------+
```

---

# 10.4 Kontenery

| Kontener | Funkcja |
|----------|---------|
| mosquitto | Broker MQTT |
| iot-core | Backend systemu |
| influxdb | Baza danych |
| nicegui | Interfejs użytkownika |

Każdy kontener realizuje jedną odpowiedzialność.

---

# 10.5 Sieć Docker

Tworzona jest dedykowana sieć:

```
iot-network
```

Zasady:

- komunikacja wyłącznie wewnątrz sieci,
- komunikacja po nazwach usług,
- brak zależności od adresów IP.

---

# 10.6 Wolumeny

Dane trwałe przechowywane są poza kontenerami.

Proponowane wolumeny:

| Wolumen | Przeznaczenie |
|---------|---------------|
| influxdb-data | Dane pomiarowe |
| mosquitto-config | Konfiguracja brokera |
| mosquitto-data | Dane brokera |
| mosquitto-log | Logi MQTT |

Kontenery powinny pozostać bezstanowe.

---

# 10.7 Konfiguracja

Każdy komponent korzysta z pliku `.env`.

Przykładowe zmienne:

```
MQTT_HOST
MQTT_PORT

INFLUX_HOST
INFLUX_PORT

API_PORT

NICEGUI_PORT
```

Kod aplikacji nie powinien zawierać wartości konfiguracyjnych zapisanych na stałe.

---

# 10.8 Kolejność uruchamiania

```text
InfluxDB
      │
Mosquitto
      │
IoT Core
      │
NiceGUI
```

Docker Compose powinien definiować zależności `depends_on`.

Jednocześnie aplikacje powinny być odporne na chwilową niedostępność usług podczas startu.

---

# 10.9 Struktura katalogów

```text
docker/
│
├── compose.yaml
├── .env
│
├── mosquitto/
│   └── config/
│
├── influxdb/
│
├── iot-core/
│
└── nicegui/
```

Struktura odzwierciedla architekturę systemu.

---

# 10.10 Porty

| Usługa | Port |
|--------|------|
| MQTT | 1883 |
| InfluxDB | 8086 |
| IoT Core API | 8000 |
| NiceGUI | 8080 |

Numery portów mogą zostać zmienione wyłącznie przez konfigurację.

---

# 10.11 Zależności

- NiceGUI komunikuje się wyłącznie z IoT Core.
- IoT Core komunikuje się z Mosquitto oraz InfluxDB.
- Mosquitto nie komunikuje się z InfluxDB.
- InfluxDB nie komunikuje się z NiceGUI.

Takie rozdzielenie upraszcza testowanie i rozwój systemu.

---

# 10.12 Środowiska

Architektura przewiduje możliwość uruchomienia:

- Development
- Test
- Production

Różnice pomiędzy środowiskami powinny wynikać wyłącznie z konfiguracji.

---

# 10.13 Rozwój po MVP

Architektura umożliwia późniejsze dodanie:

- Grafana,
- reverse proxy (NGINX),
- TLS,
- monitoringu,
- backupów,
- orkiestracji (Docker Swarm / Kubernetes).

Obecny model wdrożenia pozwala na rozwój bez przebudowy istniejącej infrastruktury.
