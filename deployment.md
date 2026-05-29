# Deployment — Floating Sensor

Ten dokument opisuje uruchamianie nowego backendu z katalogu `new_solution/src`:
- w kontenerach (Docker Compose),
- lokalnie w trybie developerskim.

> Status projektu (May 2026): minimalny backend entrypoint `main.py` jest już dostępny i uruchamia warstwę MQTT, parser payloadów, rejestr sensorów oraz zapis do InfluxDB. Dedykowany interfejs UI nadal pozostaje do dodania.

---

## 1) Wymagania

### Wersja kontenerowa
- Docker Desktop / Docker Engine
- Docker Compose v2

### Wersja developerska
- Python 3.11+
- `pip`
- (opcjonalnie) `venv`
- Dostęp do broker MQTT i InfluxDB (lokalnie lub w kontenerach)

---

## 2) Struktura i pliki konfiguracyjne

Lokalizacja rozwiązania:
- `new_solution/src`

Kluczowe pliki:
- `new_solution/src/docker-compose.yml`
- `new_solution/src/Dockerfile`
- `new_solution/src/config.yaml`
- `new_solution/src/requirements.txt`
- `new_solution/src/mosquitto/mosquitto.conf`

---

## 3) Uruchamianie w kontenerze (Docker Compose)

Przejdź do katalogu:

```powershell
cd "c:\Programs\WorkDirDev\## Git Hub\Floating_Sensor\new_solution\src"
```

### 3.1 Start stacka

```powershell
docker compose up -d --build
```

Uruchamiane usługi:
- `mosquitto` — broker MQTT (`1883`)
- `influxdb` — baza time-series (`8086`)
- `app` — aplikacja Python (port `8501`)

### 3.2 Weryfikacja

```powershell
docker compose ps
```

```powershell
docker compose logs -f app
```

```powershell
docker compose logs -f mosquitto
```

```powershell
docker compose logs -f influxdb
```

### 3.3 Zatrzymanie

```powershell
docker compose down
```

Aby usunąć także wolumeny danych:

```powershell
docker compose down -v
```

---

## 4) Konfiguracja środowiska (zalecane)

Aktualnie przykładowe hasła/tokeny są wpisane w `docker-compose.yml`. W środowisku docelowym:

1. Przenieś sekrety do pliku `.env` (lub Docker Secrets).
2. Nie commituj tokenów produkcyjnych do repozytorium.
3. Ustaw minimum:
   - `INFLUXDB_TOKEN`
   - `DOCKER_INFLUXDB_INIT_PASSWORD`

Przykładowy `.env`:

```env
INFLUXDB_TOKEN=replace-me
INFLUXDB_INIT_PASSWORD=replace-me
```

Następnie użyj ich w `docker-compose.yml` przez `${...}`.

---

## 5) Uruchamianie w wersji developerskiej (lokalnie)

Przejdź do katalogu:

```powershell
cd "c:\Programs\WorkDirDev\## Git Hub\Floating_Sensor\new_solution\src"
```

### 5.1 Utworzenie i aktywacja venv

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
```

### 5.2 Instalacja zależności

```powershell
pip install -r requirements.txt
```

### 5.3 Zmienne środowiskowe (PowerShell)

```powershell
$env:MQTT_BROKER="localhost"
$env:INFLUXDB_URL="http://localhost:8086"
$env:INFLUXDB_TOKEN="replace-me"
```

### 5.4 Start usług zależnych (opcjonalnie z Dockera)

Jeśli nie masz lokalnie MQTT/InfluxDB, uruchom same zależności:

```powershell
docker compose up -d mosquitto influxdb
```

### 5.5 Start aplikacji

Aktualnie dostępny backend uruchomisz poleceniem:

```powershell
python main.py
```

W przyszłości wariant UI może być uruchamiany np. jako:

```powershell
streamlit run app.py
```

> Uwaga: obecnie wspierany jest backendowy entrypoint `main.py`. `app.py` to jedynie przyszły wariant dla UI.

---

## 6) Test połączenia MQTT (manualny)

Publikacja testowa do konwencji topiców:

```powershell
docker exec -it mosquitto mosquitto_pub -h localhost -t "ws/rpi1/sensor1/diag" -m '{"ts":1717000000000,"V":3.91,"rssi":-61,"fw":"0.9.0"}'
```

Nasłuch:

```powershell
docker exec -it mosquitto mosquitto_sub -h localhost -t "ws/#" -v
```

---

## 7) Najczęstsze problemy

1. **`app` nie startuje**
   - Sprawdź logi: `docker compose logs -f app`
   - Zweryfikuj, czy obraz zawiera aktualny plik `main.py`.

2. **Brak połączenia z InfluxDB**
   - Sprawdź token i URL (`INFLUXDB_URL`, `INFLUXDB_TOKEN`).
   - Sprawdź zdrowie usługi: `docker compose ps`.

3. **Brak połączenia MQTT**
   - Zweryfikuj broker i port (`MQTT_BROKER`, `1883`).
   - Sprawdź logi `mosquitto`.

4. **Porty zajęte (`1883`, `8086`, `8501`)**
   - Zmień mapowanie portów w `docker-compose.yml`.

---

## 8) Rekomendowane kolejne kroki

1. Dodać dedykowany interfejs UI (`app.py` / Streamlit).
2. Ujednolicić sposób uruchamiania backend + UI.
3. Przenieść sekrety do `.env` / secrets managera.
4. Dodać healthchecki usług i prosty smoke test po starcie.
