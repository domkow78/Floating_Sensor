# Floating Sensor Hub -- Podsumowanie założeń projektu (wersja robocza)

## Cel projektu

Floating Sensor Hub jest platformą IoT przeznaczoną do budowy
autonomicznych czujników środowiskowych. Pierwszą implementacją będzie
pływający czujnik ("Floating Sensor"), jednak architektura ma umożliwiać
budowę kolejnych urządzeń opartych na tym samym ekosystemie.

## Założenia architektury

System został podzielony na niezależne warstwy:

-   Firmware ESP32-S3
-   MQTT Broker (Mosquitto)
-   Sensor Hub (Backend)
-   InfluxDB
-   NiceGUI
-   Grafana

Każdy element odpowiada za własny zakres funkcjonalności.

    ESP32
       │
     MQTT
       │
    Mosquitto
       │
    Sensor Hub (Backend)
       ├── REST API
       ├── WebSocket (etap późniejszy)
       ├── Device Manager
       ├── Command Manager
       ├── Alarm Manager
       └── Influx Repository
       │
       ▼
    InfluxDB
       │
       ├── NiceGUI (REST API)
       └── Grafana (bezpośrednio)

## Warstwa urządzenia

### Mikrokontroler

-   ESP32-S3

### Czujniki pierwszej wersji

-   BME688
-   LSM6DSV

### Komunikacja

-   WiFi
-   MQTT

### Zasilanie

-   Akumulator Li-Ion/LiPo
-   Ładowarka BQ25616J
-   Stabilizator TLV75733

## Sensor Hub

Sensor Hub stanowi centralny backend systemu.

Odpowiada za:

-   komunikację MQTT,
-   odbieranie danych,
-   walidację payloadów,
-   zapis do InfluxDB,
-   REST API,
-   zarządzanie urządzeniami,
-   obsługę komend,
-   logikę biznesową.

Sensor Hub jest jedynym komponentem komunikującym się z urządzeniami.

## REST API

Interfejs NiceGUI komunikuje się wyłącznie z Sensor Hub poprzez REST
API.

Nie komunikuje się bezpośrednio z:

-   MQTT,
-   InfluxDB.

Przykładowe endpointy:

-   GET /api/devices
-   GET /api/device/{id}
-   GET /api/device/{id}/measurements
-   POST /api/device/{id}/command

## NiceGUI

NiceGUI pełni rolę aplikacji użytkownika.

Zakres:

-   Dashboard
-   Lista urządzeń
-   Diagnostyka
-   Konfiguracja
-   OTA (kolejny etap)
-   Logi
-   Administracja

## Grafana

Grafana pozostaje narzędziem do analizy danych historycznych.

Łączy się bezpośrednio z InfluxDB.

Przeznaczenie:

-   wykresy,
-   trendy,
-   dashboardy,
-   raporty,
-   alarmy.

## InfluxDB

InfluxDB przechowuje wszystkie dane pomiarowe oraz diagnostyczne.

## Docker

Każdy komponent działa jako oddzielny kontener.

Planowane kontenery:

-   mosquitto
-   sensor-hub
-   influxdb
-   nicegui
-   grafana

## Etapy realizacji

### Etap 1 (MVP)

-   ESP32-S3
-   BME688
-   LSM6DSV
-   MQTT
-   Mosquitto
-   Sensor Hub
-   REST API
-   InfluxDB
-   NiceGUI
-   Docker

### Etap 2

-   Grafana
-   WebSocket
-   OTA
-   Alarmy
-   Device Registry
-   Konfiguracja urządzeń

### Etap 3

-   Role użytkowników
-   Harmonogramy
-   Raporty
-   Eksport danych
-   Obsługa wielu urządzeń

### Etap 4

-   AI
-   Predykcja awarii
-   Wykrywanie anomalii
-   Integracja z Home Assistant
-   Integracja z Node-RED

## Główne decyzje projektowe

1.  NiceGUI zastępuje Streamlit jako główny interfejs użytkownika.
2.  Sensor Hub jest backendem systemu.
3.  REST API stanowi jedyny interfejs pomiędzy backendem a GUI.
4.  MQTT służy wyłącznie do komunikacji z urządzeniami.
5.  Grafana odpowiada za analizę danych historycznych.
6.  InfluxDB jest centralnym magazynem danych pomiarowych.
7.  Wszystkie komponenty będą uruchamiane jako niezależne kontenery
    Docker.
8.  Architektura od początku przygotowana jest do obsługi wielu
    urządzeń.
