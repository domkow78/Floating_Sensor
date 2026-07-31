
# Rozdział 3 – Floating Sensor
**IoT Platform Architecture Specification v1.0**

---

# 3.1 Cel rozdziału

Rozdział definiuje architekturę referencyjnego urządzenia platformy IoT – **Floating Sensor**.

Nie opisuje szczegółów elektroniki ani projektu PCB. Celem jest określenie funkcji urządzenia, podziału na moduły oraz interfejsów pomiędzy nimi.

---

# 3.2 Przeznaczenie urządzenia

Floating Sensor jest autonomicznym modułem pomiarowym przeznaczonym do prowadzenia pomiarów środowiskowych wewnątrz urządzeń, komór badawczych oraz innych zamkniętych przestrzeni.

Urządzenie ma postać zamkniętej kuli, którą można swobodnie umieścić w badanym obiekcie bez konieczności montażu.

Jest to pierwsze urządzenie referencyjne rozwijanej platformy IoT.

---

# 3.3 Wymagania funkcjonalne

Urządzenie powinno:

- wykonywać cykliczne pomiary,
- pracować autonomicznie z własnego zasilania,
- komunikować się przez WiFi,
- publikować dane przez MQTT,
- umożliwiać zdalną identyfikację i diagnostykę.

---

# 3.4 Architektura logiczna

```text
                +------------------+
                |   Akumulator     |
                +--------+---------+
                         |
                 +-------v--------+
                 | Power Manager  |
                 +-------+--------+
                         |
                  +------v------+
                  |  ESP32-S3   |
      +-----------+------+------+-----------+
      |                  |                  |
+-----v-----+      +-----v-----+      +-----v------+
|  BME688   |      | LSM6DSV   |      |   WiFi     |
+-----------+      +-----------+      +-----+------+
                                               |
                                               v
                                            MQTT Broker
```

---

# 3.5 Moduły urządzenia

| Moduł | Odpowiedzialność |
|-------|-------------------|
| ESP32-S3 | Sterowanie całym urządzeniem |
| BME688 | Parametry środowiskowe |
| LSM6DSV | Ruch i orientacja |
| Power Manager | Zasilanie i ładowanie |
| WiFi | Łączność z siecią |
| MQTT Client | Transmisja danych |

Każdy moduł odpowiada za jeden wyraźnie określony obszar funkcjonalny.

---

# 3.6 Interfejsy

| Interfejs | Zastosowanie |
|-----------|--------------|
| I²C | Komunikacja z czujnikami |
| WiFi | Komunikacja sieciowa |
| MQTT | Publikacja danych |

W wersji MVP wszystkie czujniki komunikują się z ESP32 przez magistralę I²C.

---

# 3.7 Dane pomiarowe

Pierwsza wersja urządzenia udostępnia:

- temperaturę,
- wilgotność,
- ciśnienie,
- wskaźniki jakości powietrza (BME688),
- przyspieszenie,
- orientację/ruch.

Dokładny format komunikatów zostanie opisany w rozdziale MQTT.

---

# 3.8 Cykl pracy

1. Uruchomienie urządzenia.
2. Inicjalizacja zasilania i czujników.
3. Połączenie z WiFi.
4. Połączenie z brokerem MQTT.
5. Odczyt danych.
6. Publikacja pomiarów.
7. Przejście do kolejnego cyklu pomiarowego.

W MVP urządzenie pracuje w sposób ciągły. Mechanizmy oszczędzania energii zostaną rozważone w kolejnych wersjach.

---

# 3.9 Zasady projektowe

- Firmware nie zawiera logiki biznesowej.
- Floating Sensor nie przechowuje historii pomiarów.
- Urządzenie publikuje dane, ale ich nie interpretuje.
- Wszystkie decyzje analityczne należą do IoT Core.
- Architektura umożliwia wymianę czujników przy zachowaniu tego samego interfejsu programowego.

---

# 3.10 Poza zakresem MVP

Nie są objęte wersją 1.0:

- projekt mechaniczny obudowy,
- projekt PCB,
- analiza szczelności,
- optymalizacja zużycia energii,
- OTA,
- Bluetooth,
- dodatkowe czujniki.

Powyższe elementy będą rozwijane po zweryfikowaniu pierwszego działającego prototypu.
