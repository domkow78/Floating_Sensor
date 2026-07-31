
# Rozdział 12 – Plan implementacji
**IoT Platform Architecture Specification v1.0**

---

# 12.1 Cel

Plan implementacji definiuje kolejność budowy platformy IoT oraz kryteria zakończenia
poszczególnych etapów.

Implementacja prowadzona jest iteracyjnie – każdy etap kończy się działającym,
testowalnym fragmentem systemu.

---

# 12.2 Zasady realizacji

Projekt realizowany jest zgodnie z następującymi zasadami:

- implementacja zgodnie z dokumentacją architektoniczną,
- małe, zakończone etapy,
- po każdym etapie testy,
- dokumentacja aktualizowana równolegle z kodem,
- każda funkcjonalność posiada kryterium zakończenia.

---

# 12.3 Roadmapa MVP

## Etap 1 – Repozytorium

Zakres:

- utworzenie repozytorium,
- przygotowanie struktury katalogów,
- konfiguracja Git,
- konfiguracja PlatformIO,
- konfiguracja środowiska Python.

Rezultat:

Gotowe środowisko developerskie.

---

## Etap 2 – Firmware

Zakres:

- uruchomienie ESP32,
- konfiguracja WiFi,
- obsługa BME688,
- obsługa LSM6DSV,
- moduł konfiguracji,
- logger.

Rezultat:

Stabilny odczyt danych z czujników.

---

## Etap 3 – MQTT

Zakres:

- konfiguracja Mosquitto,
- implementacja MQTT Client,
- publikacja telemetry,
- publikacja status,
- LWT.

Rezultat:

Widoczne komunikaty MQTT.

---

## Etap 4 – IoT Core

Zakres:

- MQTT Gateway,
- Processing Engine,
- Storage Service,
- Device Registry.

Rezultat:

Dane odbierane i przetwarzane przez backend.

---

## Etap 5 – InfluxDB

Zakres:

- konfiguracja bazy,
- zapis danych,
- odczyt danych,
- testy wydajności podstawowej.

Rezultat:

Historia pomiarów zapisana w bazie.

---

## Etap 6 – REST API

Zakres:

- implementacja endpointów,
- modele danych,
- walidacja,
- obsługa błędów.

Rezultat:

Stabilne API zgodne ze specyfikacją.

---

## Etap 7 – NiceGUI

Zakres:

- Dashboard,
- Historia,
- Status,
- Informacje.

Rezultat:

Pełny interfejs użytkownika.

---

## Etap 8 – Docker

Zakres:

- kontenery,
- compose,
- sieci,
- wolumeny,
- plik .env.

Rezultat:

Uruchomienie platformy jednym poleceniem.

---

## Etap 9 – Testy integracyjne

Zakres:

Sprawdzenie pełnego przepływu:

Floating Sensor
→ MQTT
→ IoT Core
→ InfluxDB
→ REST API
→ NiceGUI

Rezultat:

Stabilna praca całego systemu.

---

# 12.4 Kamienie milowe

| Milestone | Kryterium |
|-----------|-----------|
| M1 | Repozytorium gotowe |
| M2 | Firmware działa |
| M3 | MQTT działa |
| M4 | IoT Core działa |
| M5 | InfluxDB działa |
| M6 | REST API działa |
| M7 | NiceGUI działa |
| M8 | Docker działa |
| M9 | MVP zakończone |

---

# 12.5 Artefakty

Każdy etap powinien dostarczyć:

- kod źródłowy,
- testy,
- aktualizację dokumentacji,
- przykładową konfigurację,
- instrukcję uruchomienia.

---

# 12.6 Walidacja

Po zakończeniu każdego etapu należy zweryfikować:

- zgodność z dokumentacją,
- poprawność działania,
- brak regresji,
- poprawność konfiguracji.

---

# 12.7 Definicja Done

Etap uznaje się za zakończony, jeżeli:

- implementacja została ukończona,
- testy zakończyły się powodzeniem,
- dokumentacja została zaktualizowana,
- kod spełnia przyjęte standardy,
- możliwe jest przejście do kolejnego etapu.

---

# 12.8 Kryterium zakończenia MVP

Projekt MVP zostaje uznany za zakończony, gdy:

- Floating Sensor wykonuje stabilne pomiary,
- dane trafiają do MQTT,
- IoT Core odbiera i zapisuje dane,
- InfluxDB przechowuje historię,
- REST API udostępnia dane,
- NiceGUI prezentuje informacje użytkownikowi,
- całość uruchamiana jest przez Docker Compose.

---

# 12.9 Rozwój po MVP

Po zakończeniu MVP rozpoczyna się etap rozwoju platformy.

Priorytety:

1. Grafana
2. OTA
3. Obsługa wielu urządzeń
4. Alarmy
5. Panel administracyjny
6. Integracje zewnętrzne

Rozszerzenia będą realizowane na bazie zweryfikowanej architektury wersji 1.0.
