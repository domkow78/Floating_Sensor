# Rozdzial 14 - Tracker postepu wdrozenia MVP
**IoT Platform Architecture Specification v1.0**
**Status:** Working tracker

---

# 14.1 Cel dokumentu

Ten dokument sluzy do biezacego monitorowania postepu wdrozenia MVP.

Zawiera:
- status etapow z Rozdzialu 12,
- checklisty rzeczy domknietych i otwartych,
- ostatnia walidacje uruchomieniowa,
- kroki nastepne.

---

# 14.2 Status globalny (na dzis)

| Etap | Nazwa | Status |
|------|-------|--------|
| Etap 1 | Repozytorium | DONE |
| Etap 2 | Firmware | TODO |
| Etap 3 | MQTT | DONE |
| Etap 4 | IoT Core | PARTIAL |
| Etap 5 | InfluxDB | DONE |
| Etap 6 | REST API | PARTIAL |
| Etap 7 | NiceGUI | PARTIAL |
| Etap 8 | Docker | TODO |
| Etap 9 | Testy integracyjne | TODO |

Legenda:
- DONE: etap domkniety
- PARTIAL: etap rozpoczecy i czesciowo wdrozony
- TODO: etap jeszcze nierozpoczety / bez domkniecia

---

# 14.3 Etap 1 - Repozytorium (DONE)

## Domkniete
- [x] Rozdzielenie aktywnego kodu i prototypu referencyjnego (`src` vs `src_ref`).
- [x] Podstawowa struktura katalogow aktywnej implementacji.
- [x] Lokalny venv dla aktywnego komponentu Python.
- [x] Ignorowanie `.venv` w Git.

## Do monitorowania
- [ ] Ujednolicenie struktury pod finalny monorepo target (po MVP).

---

# 14.4 Etap 3 - MQTT (DONE)

## Domkniete
- [x] Konwencja topic `floatingsensor/<device_id>/<topic>` w kodzie.
- [x] Dozwolone topici MVP: `status`, `telemetry`, `diagnostics`.
- [x] Publikacja telemetry przez klienta MQTT.
- [x] Subskrypcja `floatingsensor/#` przez MqttGateway.
- [x] Gateway start/stop spinajacy broker -> pipeline -> storage -> registry.
- [x] Testy kontraktu topicow i publikacji.

## Otwarte
- [ ] Pelne status + diagnostics flow z urzadzenia.
- [ ] LWT w rzeczywistym scenariuszu runtime.

---

# 14.5 Etap 4 - IoT Core (PARTIAL)

## Domkniete
- [x] Processing Engine z walidacja i normalizacja payload.
- [x] Wspolny model `TelemetryRecord`.
- [x] Storage mapping do modelu zapisu Influx.
- [x] Device Registry (last seen + ostatnia telemetry).
- [x] App pipeline spinajacy processing -> mqtt -> storage -> registry.

## Otwarte
- [ ] Produkcyjny podzial na pelne moduły i interfejsy wewnetrzne.
- [ ] Runtime telemetry ingestion z rzeczywistego source stream.
- [ ] Rozszerzenie registry o status/offline timeout.

---

# 14.6 Etap 5 - InfluxDB (DONE)

## Domkniete
- [x] Definicja bucket/measurement/tag-field na poziomie kodu.
- [x] Zapis rekordu telemetry przez influxdb-client.
- [x] Realne zapytania historyczne Flux.
- [x] Testy integracyjne z realna instancja InfluxDB na RPi.
- [x] Weryfikacja zakresu czasu i limitow na danych rzeczywistych.

## Otwarte
- [ ] Brak.

---

# 14.7 Etap 6 - REST API (PARTIAL)

## Domkniete
- [x] Endpointy read-only:
  - [x] `GET /api/v1/status`
  - [x] `GET /api/v1/device`
  - [x] `GET /api/v1/telemetry/latest`
  - [x] `GET /api/v1/telemetry/history`
- [x] Walidacja query (`from`, `to`, `limit`) i model bledow `INVALID_QUERY`.
- [x] Rozdzielenie API na `routes`, `services`, `errors`.
- [x] ASGI entrypoint pod `uvicorn`.
- [x] Dev seed endpoint `POST /api/v1/dev/ingest` do lokalnych smoke-checkow.

## Otwarte
- [ ] Finalne wydzielenie schemas/models odpowiedzi pod produkcyjne API.
- [ ] Pelna walidacja wejscia request body dla endpointow write (poza MVP read-only).
- [ ] Ujednolicenie policy kodow HTTP i globalnego handlera bledow.

---

# 14.8 Etapy 7-9 (TODO)

## Etap 7 - NiceGUI (PARTIAL)
- [x] Strona Dashboard (odczyt telemetry z REST API).
- [x] Strona Simulator (publikacja MQTT jak ESP32).
- [ ] Historia pomiarow.
- [ ] Status systemu.
- [ ] Informacje o urzadzeniu.

## Etap 8 - Docker
- [ ] Compose spinajacy uslugi docelowe.
- [ ] Konfiguracja `.env` i porzadek sekretow.
- [ ] Weryfikacja uruchomienia jednym poleceniem.

## Etap 9 - Testy integracyjne
- [ ] End-to-end: Sensor -> MQTT -> IoT Core -> InfluxDB -> REST -> UI.
- [ ] Smoke test calosci po starcie stacka.
- [ ] Kryteria stabilnosci i regresji.

---

# 14.9 Ostatnia walidacja techniczna

## Wynik testow
- `python -m pytest tests/test_smoke.py -q` — PASS (22 testy, RPi)
- `python -m pytest tests/test_integration.py -q` — PASS (realny InfluxDB na RPi)

## Runtime smoke-checki
- [x] `check_api.ps1` (status)
- [x] `check_api_latest.ps1` (latest)
- [x] `check_api_history.ps1` (history)
- [x] `seed_api_telemetry.ps1` (zasiew telemetry do procesu API)

---

# 14.10 Najblizsze kroki (kolejnosc)

1. Rozbudowac NiceGUI o zakladki Historia, Status, Informacje.
2. Przygotowac Etap 8 (Docker) jako reproducible `up -> smoke`.
3. Domkniecie Etapu 4 IoT Core (status/diagnostics, offline timeout).

---

# 14.11 Jak aktualizowac ten tracker

Po kazdym zamknietym kroku:
1. Zmien status etapu w tabeli 14.2.
2. Oznacz checkboxy `Domkniete` / `Otwarte`.
3. Zaktualizuj sekcje 14.9 o najnowsza walidacje.
4. Zmien sekcje 14.10 na kolejne realne kroki.

Ten plik ma byc jedynym operacyjnym widokiem postepu wdrozenia MVP.
