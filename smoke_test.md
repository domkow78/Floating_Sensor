# Smoke Test — Floating Sensor

Ten dokument opisuje szybki test sprawdzający, czy podstawowy przepływ aplikacji działa:
1. broker MQTT działa,
2. backend się uruchamia,
3. wiadomość MQTT dociera do aplikacji,
4. dane trafiają do InfluxDB.

## 1) Uruchom stack

Przejdź do katalogu:

```powershell
cd "c:\Programs\WorkDirDev\## Git Hub\Floating_Sensor\new_solution\src"
```

Uruchom kontenery:

```powershell
docker compose up -d --build
```

### Oczekiwane usługi
- `mosquitto`
- `influxdb`
- `app`

## 2) Sprawdź, czy kontenery działają

```powershell
docker compose ps
```

Szukasz statusu `running` dla wszystkich usług.

## 3) Podejrzyj logi backendu

```powershell
docker compose logs -f app
```

### Oczekiwane objawy
- połączenie z MQTT brokerem,
- subskrypcja topiców,
- brak błędów importów albo połączenia z InfluxDB.

## 4) Wyślij testową wiadomość MQTT

Najpierw test `diag`:

```powershell
docker exec -it mosquitto mosquitto_pub -h localhost -t "ws/rpi1/sensor1/diag" -m "{\"ts\":1717000000000,\"V\":3.91,\"rssi\":-61,\"fw\":\"0.9.0\",\"lm\":\"statusReq\"}"
```

Potem test `measurement`:

```powershell
docker exec -it mosquitto mosquitto_pub -h localhost -t "ws/rpi1/sensor1/measurement" -m "{\"ts\":1717000000000,\"t\":24.5,\"h\":48.2,\"p\":100812,\"acmm\":3,\"imp\":1200,\"impi\":1180,\"impc\":1170,\"ph\":12,\"phf\":50}"
```

## 5) Sprawdź reakcję aplikacji

W logach `app` powinieneś zobaczyć:
- odebranie `diag` albo `measurement`,
- zapis do InfluxDB,
- brak błędów parsowania.

## 6) Sprawdź zapis w InfluxDB

Najprościej:
- otwórz UI InfluxDB pod adresem `http://<IP_RASPBERRY>:8086`
- zaloguj się danymi z `docker-compose.yml`
- sprawdź bucket `measurements` i `diagnostics`

## 7) Wersja minimalna

Jeśli chcesz wykonać tylko szybki test:
1. uruchom `docker compose up -d --build`
2. wyślij `diag` na topic `ws/rpi1/sensor1/diag`
3. sprawdź logi `app`

To wystarczy, żeby potwierdzić, że rdzeń systemu działa.

## 8) Najczęstsze problemy

- **Brak połączenia z MQTT** — zły adres brokera albo nie działa `mosquitto`
- **Brak zapisu do InfluxDB** — zły token lub URL
- **Brak reakcji na topic** — zły format topicu; musi być `ws/<target>/<source>/<smallTopic>`
- **Błędy payloadu** — JSON nie zgadza się ze schematem z `config.yaml`
