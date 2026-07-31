
# Rozdział 8 – InfluxDB
**IoT Platform Architecture Specification v1.0**

---

# 8.1 Cel

InfluxDB jest centralnym repozytorium danych pomiarowych platformy IoT.

Baza odpowiada wyłącznie za trwałe przechowywanie i udostępnianie danych czasowych.
Cała logika biznesowa pozostaje w IoT Core.

---

# 8.2 Założenia projektowe

W wersji MVP:

- jedna instancja InfluxDB,
- jeden bucket,
- jedno urządzenie (Floating Sensor),
- dane typu Time Series,
- zapis wyłącznie przez IoT Core.

InfluxDB nie komunikuje się bezpośrednio z MQTT ani z urządzeniem.

---

# 8.3 Architektura

```text
Floating Sensor
        │
      MQTT
        │
        ▼
    IoT Core
        │
        ▼
   Storage Service
        │
        ▼
     InfluxDB
        │
        ▼
      REST API
        │
        ▼
      NiceGUI
```

Storage Service stanowi jedyną warstwę komunikacji z bazą danych.

---

# 8.4 Model danych

## Bucket

```
floating_sensor
```

W wersji MVP wykorzystywany jest jeden bucket dla wszystkich danych.

---

## Measurement

```
environment
```

Measurement reprezentuje pojedynczy zapis pomiarowy.

---

## Tags

| Tag | Opis |
|-----|------|
| device_id | Identyfikator urządzenia |
| firmware | Wersja firmware |
| location | Opcjonalna lokalizacja urządzenia |

Tags służą do filtrowania oraz grupowania danych.

---

## Fields

| Field | Typ | Opis |
|--------|-----|------|
| temperature | float | Temperatura |
| humidity | float | Wilgotność |
| pressure | float | Ciśnienie |
| gas_resistance | float | Parametr z BME688 |
| accel_x | float | Oś X |
| accel_y | float | Oś Y |
| accel_z | float | Oś Z |

Model można rozszerzać o kolejne pola bez zmiany struktury measurement.

---

# 8.5 Mapowanie MQTT → InfluxDB

Przykład komunikatu MQTT:

```json
{
  "device_id":"FS-001",
  "temperature":22.3,
  "humidity":48.1,
  "pressure":1008.6
}
```

Mapowanie:

| MQTT | InfluxDB |
|------|----------|
| device_id | tag |
| timestamp | timestamp |
| temperature | field |
| humidity | field |
| pressure | field |

Całe mapowanie realizowane jest przez Processing Engine.

---

# 8.6 Retencja danych

W MVP przyjmujemy:

- retencja: bezterminowa,
- brak automatycznej agregacji,
- brak downsamplingu.

Polityka retencji zostanie ponownie oceniona po zebraniu rzeczywistych danych.

---

# 8.7 Nazewnictwo

Zasady:

- nazwy małymi literami,
- separator `_`,
- jednostki nie są częścią nazwy pola,
- nazwy zgodne z terminologią czujników.

Przykłady:

```
temperature
gas_resistance
accel_x
```

---

# 8.8 Dostęp do danych

Dostęp realizowany jest wyłącznie przez Storage Service.

Pozostałe moduły nie wykonują bezpośrednich operacji na InfluxDB.

Takie podejście pozwala w przyszłości zmienić silnik bazy danych bez wpływu na pozostałe komponenty.

---

# 8.9 Typowe zapytania

System powinien umożliwiać:

- pobranie ostatniego pomiaru,
- pobranie historii pomiarów,
- pobranie danych z wybranego przedziału czasu,
- pobranie danych dla wybranego urządzenia.

Szczegółowe zapytania Flux będą częścią dokumentacji implementacyjnej.

---

# 8.10 Zależności

- MQTT nie komunikuje się z InfluxDB.
- NiceGUI nie komunikuje się z InfluxDB.
- REST API nie komunikuje się z InfluxDB.
- Jedynym komponentem zapisującym dane jest Storage Service.

---

# 8.11 Rozwój po MVP

Architektura umożliwia późniejsze dodanie:

- wielu bucketów,
- polityk retencji,
- automatycznej agregacji,
- Continuous Queries / Tasks,
- archiwizacji danych,
- wielu urządzeń.

Obecny model danych został zaprojektowany tak, aby umożliwić rozwój bez konieczności migracji istniejących rekordów.
