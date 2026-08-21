# Deployment

Ten dokument opisuje aktualny sposób uruchamiania lokalnego środowiska MVP dla projektu Floating Sensor.

Status bieżący:

- aktywna implementacja znajduje się w katalogu [new_solution/src](new_solution/src),
- dokumentacja architektoniczna jest w [new_solution/doc](new_solution/doc),
- stary prototyp pozostaje w [new_solution/src_ref](new_solution/src_ref) tylko jako materiał referencyjny.

---

## 1. Cel dokumentu

- uruchamiać lokalne środowisko developerskie,
- wspierać testowanie MVP bez zależności od starego prototypu,
- utrzymywać zgodność między dokumentacją, konfiguracją i kodem.

---

## 2. Obecna struktura projektu

```text
Floating_Sensor/
├── README.md
├── deployment.md
├── new_solution/
│   ├── doc/
│   ├── pcb/
│   ├── scripts/
│   ├── src/
│   └── src_ref/
├── old_solution/
├── old_solution_not_used/
└── ...
```

Aktualny katalog roboczy:

```text
new_solution/src/
├── api/
├── app/
├── config/
├── docker-compose.yml
├── Dockerfile
├── mosquitto/
├── mqtt/
├── nicegui/
├── processing/
├── registry/
├── requirements.txt
├── storage/
├── tests/
└── .venv/
```

---

## 3. Wymagania lokalne

- Python 3.11+
- pip
- venv
- Docker Desktop / Docker Engine z Docker Compose
- opcjonalnie: PowerShell do uruchamiania skryptów smoke test

---

## 4. Szybki start: Python + Docker

### 4.1 Przejdź do katalogu źródłowego

```powershell
cd "C:\Programs\WorkDirDev\## Git Hub\Floating_Sensor\new_solution\src"
```

### 4.2 Utwórz środowisko wirtualne

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
```

### 4.3 Zainstaluj zależności

```powershell
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
```

### 4.4 Uruchom podstawowe usługi infrastruktury

W katalogu [new_solution/src](new_solution/src) znajduje się plik docker-compose.yml przygotowany do uruchamiania brokerów i bazy czasu:

```powershell
docker compose up -d mosquitto influxdb
```

Możliwe jest też uruchomienie całego środowiska w jednym poleceniu:

```powershell
docker compose up --build -d
```

---

## 5. Uruchamianie aplikacji backendu

### 5.1 REST API

```powershell
python -m uvicorn api.main:app --host 0.0.0.0 --port 8000
```

Aplikacja korzysta z wejścia ASGI z pliku [new_solution/src/api/main.py](new_solution/src/api/main.py).

### 5.2 Punkt wejścia IoT Core

```powershell
python app/main.py
```

To uruchamia prosty pipeline MVP, który przetwarza przykładowy payload, publikuje dane do MQTT i zapisuje je do InfluxDB.

### 5.3 NiceGUI

```powershell
python nicegui/app.py
```

UI jest dostępne pod adresem:

```text
http://localhost:8080
```

---

## 6. Domyślne porty i usługi

| Usługa | Port | Opis |
|--------|------|------|
| MQTT broker (Mosquitto) | 1883 | transport telemetryki |
| InfluxDB | 8086 | magazyn danych szeregów czasowych |
| REST API | 8000 | endpointy MVP |
| NiceGUI | 8080 | panel użytkownika |

Konfiguracja jest definiowana przez zmienne środowiskowe w pliku [new_solution/src/config/settings.py](new_solution/src/config/settings.py) oraz w docker-compose.yml.

---

## 7. Konfiguracja środowiska

Najważniejsze zmienne w [new_solution/src/config/settings.py](new_solution/src/config/settings.py):

- `MQTT_BROKER_HOST`
- `MQTT_BROKER_PORT`
- `INFLUX_HOST`
- `INFLUX_PORT`
- `INFLUX_URL`
- `INFLUX_TOKEN`
- `INFLUX_ORG`
- `INFLUX_BUCKET`
- `DEVICE_ID`
- `API_BASE`

Domyślne wartości są ustawione tak, aby działać lokalnie w środowisku developerskim.

---

## 8. Smoke test i walidacja API

Po uruchomieniu API można wykonać testy szybkie z katalogu [new_solution/scripts](new_solution/scripts):

```powershell
cd "C:\Programs\WorkDirDev\## Git Hub\Floating_Sensor\new_solution"
.\scripts\check_api.ps1
```

Dodatkowe testy:

```powershell
.\scripts\check_api_latest.ps1 -DeviceId "FS-001"
.\scripts\check_api_history.ps1 -DeviceId "FS-001"
```

Wariant z wymuszeniem danych historycznych:

```powershell
.\scripts\check_api_history.ps1 -DeviceId "FS-001" -RequirePoints
```

Jeżeli dane nie zostały jeszcze zasiane, można uruchomić seed:

```powershell
.\scripts\seed_api_telemetry.ps1 -DeviceId "FS-001"
```

Dodatkowo w repozytorium dostępne są testy pytest:

```powershell
python -m pytest -q
```

---

## 9. Docker Compose

Aktualny plik [new_solution/src/docker-compose.yml](new_solution/src/docker-compose.yml) definiuje usługi:

- `mosquitto`
- `influxdb`
- `iot-core`
- `nicegui`

Zasada działania:

- podłączone usługi komunikują się przez sieć Docker `iot-network`,
- usługi są uruchamiane na podanych portach lokalnych,
- aplikacja powinna być odporna na chwilową niedostępność komponentów podczas startu.

---

## 10. Uwagi operacyjne

- [new_solution/src](new_solution/src) jest aktywnym kierunkiem rozwoju MVP,
- [new_solution/src_ref](new_solution/src_ref) jest zbiorem referencyjnym i nie należy rozwijać w nim nowych zmian,
- dokumentacja w [new_solution/doc](new_solution/doc) jest źródłem prawdy dla architektury,
- w razie zmiany kontraktu danych lub struktury usług aktualizować zarówno kod, jak i dokumentację.

---

## 11. Zalecany przepływ pracy

1. uruchom infrastrukturalne usługi w Dockerze,
2. zainstaluj zależności Pythona,
3. uruchom backend i UI,
4. wykonywać smoke testy po każdej zmianie kontraktu lub konfiguracji,
5. nie wprowadzaj zmian bez aktualizacji dokumentacji.

---

## 12. Podsumowanie

To wdrożenie ma charakter lokalnego środowiska deweloperskiego dla MVP. Jest zgodne z aktualną architekturą repozytorium i służy do iteracyjnego rozwoju systemu: od MQTT, przez IoT Core, po InfluxDB, REST API i NiceGUI.
