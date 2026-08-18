# Rozdzial 13 - Mapa migracji i kontrakty MVP
**IoT Platform Architecture Specification v1.0**  
**Status:** Draft v0.1

---

# 13.1 Cel dokumentu

Dokument laczy dwa cele:

- zdefiniowanie migracji z obecnego prototypu do architektury docelowej,
- doprecyzowanie kontraktow MQTT i REST API dla MVP.

To jest dokument roboczy do wspolnego dopracowania.

---

# 13.2 Decyzja strategiczna

## Decyzja: prototyp zastapic, nie rozwijac bezposrednio

Obecny kod w `new_solution/src` traktujemy jako **prototyp referencyjny**.
Nie jest baza docelowej implementacji 1:1.

## Co zachowujemy z prototypu

- wzorce techniczne: reconnect MQTT, parser schematu payload, izolacja warstwy storage,
- obserwacje praktyczne: jakie pola telemetryczne i diagnostyczne sa realnie wysylane,
- testowe konfiguracje Docker i lokalne dane uruchomieniowe.

## Co zastępujemy

- strukture monolityczna "hub/app",
- niejawny kontrakt MQTT oparty o male topici,
- brak wydzielonego REST API,
- zaleznosci i komentarze po poprzednim kierunku UI (Streamlit).

---

# 13.3 Roznice: prototyp vs architektura docelowa

| Obszar | Prototyp (`new_solution/src`) | Architektura docelowa MVP |
|-------|--------------------------------|----------------------------|
| Rola aplikacji | Hub MQTT + zapis do Influx | IoT Core z modulami: MQTT Gateway, Processing, Storage, API |
| UI | Brak realnej implementacji; slady Streamlit | NiceGUI jako oddzielna usluga |
| API | Brak publicznego REST | REST `/api/v1/...` |
| Topics MQTT | `ws/<target>/<source>/<small_topic>` | `floatingsensor/<device_id>/<topic>` |
| Docker | 3 kontenery: mosquitto, influxdb, app | 4 kontenery: mosquitto, iot-core, influxdb, nicegui |
| Modele danych | Krotkie klucze payload (`t`, `h`, `p`, ...) | Jawny model telemetry/status/diagnostics |

---

# 13.4 Mapa migracji (etapy techniczne)

## Etap A - Zamrozenie prototypu

Zakres:
- oznaczenie `new_solution/src` jako `legacy-prototype`,
- brak nowych funkcji,
- tylko odczyt i wykorzystanie jako zrodlo wiedzy.

Rezultat:
- stabilny punkt odniesienia,
- zero mieszania architektury docelowej z legacy.

## Etap B - Szkielet docelowego monorepo w `new_solution`

Zakres:
- katalogi: `iot-core`, `nicegui`, `docker`, `tests`, `docs` (mapowane do obecnego ukladu),
- bazowe pliki uruchomieniowe i README komponentow,
- wspolny model konfiguracji `.env` + `.env.example`.

Rezultat:
- gotowa przestrzen pod implementacje etapow 4-8 z Rozdzialu 12.

## Etap C - IoT Core MVP

Zakres:
- wydzielenie modulow: `mqtt_gateway`, `processing_engine`, `storage_service`, `device_registry`,
- przeniesienie logiki z prototypu jako inspiracji (bez kopiowania struktury 1:1),
- kontrakty wewnetrzne na modelach danych.

Rezultat:
- backend zgodny z rozdzialami 6 i 8.

## Etap D - REST API MVP

Zakres:
- endpointy `status`, `device`, `telemetry/latest`, `telemetry/history`,
- ujednolicony format sukcesu i bledu,
- walidacja parametrow history query.

Rezultat:
- stabilny interfejs dla NiceGUI.

## Etap E - NiceGUI MVP

Zakres:
- oddzielna usluga,
- warstwa services tylko przez REST,
- strony Dashboard, Historia, Status, Informacje.

Rezultat:
- frontend zgodny z rozdzialem 9.

## Etap F - Docker integracyjny

Zakres:
- compose z 4 uslugami,
- wolumeny i siec zgodnie z rozdzialem 10,
- smoke test "up -> telemetry -> UI".

Rezultat:
- uruchomienie platformy jednym poleceniem.

---

# 13.5 Kontrakt MQTT MVP (draft)

## Konwencja topic

`floatingsensor/<device_id>/<topic>`

Dozwolone `topic` w MVP:
- `status`
- `telemetry`
- `diagnostics`

## Wspolne zasady

- format payload: JSON,
- `device_id` wymagany w payload i zgodny z topic,
- `timestamp` w ISO 8601 UTC (`YYYY-MM-DDTHH:MM:SSZ`),
- QoS: 1,
- retain: `status=true`, `telemetry=false`, `diagnostics=false`.

## Payload: telemetry

Pola wymagane:
- `device_id` (string)
- `timestamp` (string, ISO 8601 UTC)

Pola opcjonalne:
- `temperature` (float, degC)
- `humidity` (float, %)
- `pressure` (float, hPa)
- `gas_resistance` (float)
- `accel_x` (float)
- `accel_y` (float)
- `accel_z` (float)

Przyklad:

{
  "device_id": "FS-001",
  "timestamp": "2026-08-05T10:15:00Z",
  "temperature": 22.4,
  "humidity": 48.2,
  "pressure": 1008.4
}

## Payload: status

Pola wymagane:
- `device_id` (string)
- `timestamp` (string)
- `state` (string enum: `online`, `offline`, `booting`, `error`)

Pola opcjonalne:
- `firmware_version` (string)
- `ip` (string)
- `uptime_s` (int)

## Payload: diagnostics

Pola wymagane:
- `device_id` (string)
- `timestamp` (string)

Pola opcjonalne:
- `battery_v` (float)
- `rssi_dbm` (int)
- `last_error` (string)
- `free_heap` (int)

## LWT

Topic:
- `floatingsensor/<device_id>/status`

Payload LWT:

{
  "device_id": "FS-001",
  "timestamp": "2026-08-05T10:15:00Z",
  "state": "offline"
}

---

# 13.6 Kontrakt REST API MVP (draft)

## Prefix

`/api/v1`

## GET /status

Opis:
- status platformy i uslug zaleznych.

200 response:

{
  "status": "success",
  "data": {
    "iot_core": "ok",
    "mqtt": "ok",
    "influxdb": "ok",
    "timestamp": "2026-08-05T10:16:00Z"
  }
}

## GET /device

Query:
- `device_id` (opcjonalny w MVP z 1 urzadzeniem; docelowo wymagany)

200 response:

{
  "status": "success",
  "data": {
    "device_id": "FS-001",
    "state": "online",
    "firmware_version": "1.0.0",
    "last_seen": "2026-08-05T10:15:59Z"
  }
}

## GET /telemetry/latest

Query:
- `device_id` (opcjonalny w MVP z 1 urzadzeniem)

200 response:

{
  "status": "success",
  "data": {
    "device_id": "FS-001",
    "timestamp": "2026-08-05T10:15:00Z",
    "temperature": 22.4,
    "humidity": 48.2,
    "pressure": 1008.4
  }
}

## GET /telemetry/history

Query:
- `device_id` (opcjonalny w MVP)
- `from` (ISO 8601)
- `to` (ISO 8601)
- `limit` (int, default 500, max 5000)

200 response:

{
  "status": "success",
  "data": {
    "device_id": "FS-001",
    "points": [
      {
        "timestamp": "2026-08-05T10:10:00Z",
        "temperature": 22.1,
        "humidity": 48.0,
        "pressure": 1008.1
      }
    ],
    "count": 1
  }
}

## Model bledu

Przyklad:

{
  "status": "error",
  "code": "INVALID_QUERY",
  "message": "Parameter 'from' must be ISO 8601 UTC timestamp."
}

---

# 13.7 Mapowanie legacy payload -> model docelowy

| Legacy klucz | Docelowe pole | Uwagi |
|-------------|----------------|-------|
| `ts` | `timestamp` | konwersja ms epoch -> ISO 8601 UTC |
| `t` | `temperature` | float |
| `h` | `humidity` | float |
| `p` | `pressure` | float (hPa lub Pa - do finalnej decyzji) |
| `V` | `battery_v` | diagnostics |
| `rssi` | `rssi_dbm` | diagnostics |
| `fw` | `firmware_version` | status/diagnostics |

To mapowanie pozwoli tymczasowo wspierac firmware wysylajace stare payloady podczas przejscia.

---

# 13.8 Ryzyka i decyzje do zamkniecia

1. Jednostka cisnienia w payload: `hPa` czy `Pa`.
2. Czy `device_id` ma byc opcjonalny w query juz w MVP, czy od razu wymagany.
3. Czy `diagnostics` zapisujemy do osobnego measurement/bucket, czy wspolnego z telemetry.
4. Czy akceptujemy rownolegle legacy topici `ws/...` przez okres przejsciowy.
5. Jak definiujemy timeout "offline" dla statusu urzadzenia.

---

# 13.9 Rekomendacja operacyjna

Start implementacji od "clean target" jest poprawny.

Rekomendacja:
- legacy zostaje jako material referencyjny,
- nowa implementacja powstaje od nowej struktury zgodnej z rozdzialami 6-10,
- komponenty z legacy sa przenoszone tylko przez kontrolowane refaktory i testy.

To minimalizuje ryzyko utrwalenia kompromisow prototypu.
