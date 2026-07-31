
# Rozdział 2 – Architektura systemu
**IoT Platform Architecture Specification v1.0**

---

# 2.1 Cel architektury

Architektura systemu definiuje sposób współpracy wszystkich komponentów biorących udział
w zbieraniu, przetwarzaniu, zapisie oraz prezentacji danych pomiarowych.

Wersja 1.0 obejmuje wyłącznie architekturę niezbędną do uruchomienia pierwszego
prototypu Floating Sensor.

---

# 2.2 Diagram architektury

```text
+----------------------+
|   Floating Sensor    |
|----------------------|
| ESP32-S3             |
| BME688               |
| LSM6DSV              |
+----------+-----------+
           |
        WiFi / MQTT
           |
           v
+----------------------+
|      Mosquitto       |
|    MQTT Broker       |
+----------+-----------+
           |
           v
+----------------------+
|      IoT Core        |
|----------------------|
| MQTT Client          |
| Data Processing      |
| REST API             |
+----------+-----------+
           |
           v
+----------------------+
|      InfluxDB        |
| Time Series Database |
+----------+-----------+
           |
           v
+----------------------+
|      NiceGUI         |
|  Web User Interface  |
+----------------------+
```

---

# 2.3 Przepływ danych

1. Floating Sensor wykonuje pomiary.
2. Firmware publikuje dane do brokera MQTT.
3. IoT Core odbiera wiadomości MQTT.
4. Dane są walidowane i zapisywane do InfluxDB.
5. NiceGUI pobiera dane z IoT Core i prezentuje je użytkownikowi.

W wersji MVP przepływ jest jednokierunkowy – od czujnika do interfejsu użytkownika.

---

# 2.4 Komponenty systemu

## Floating Sensor

Odpowiada za:
- akwizycję danych,
- lokalne przetwarzanie,
- komunikację WiFi,
- publikację MQTT.

Nie przechowuje historii pomiarów.

---

## Mosquitto

Broker MQTT odpowiedzialny wyłącznie za transport wiadomości.

Nie wykonuje logiki biznesowej ani nie zapisuje danych.

---

## IoT Core

Centralny element systemu.

Odpowiada za:
- odbiór danych MQTT,
- walidację,
- zapis do InfluxDB,
- udostępnienie REST API.

Cała logika biznesowa znajduje się w tym komponencie.

---

## InfluxDB

Przechowuje dane pomiarowe jako bazę typu Time Series.

Nie komunikuje się bezpośrednio z urządzeniem.

---

## NiceGUI

Interfejs użytkownika umożliwiający:
- podgląd bieżących parametrów,
- przegląd historii pomiarów,
- podgląd statusu systemu.

Komunikacja odbywa się wyłącznie przez REST API IoT Core.

---

# 2.5 Zasady komunikacji

- Floating Sensor komunikuje się wyłącznie przez MQTT.
- NiceGUI komunikuje się wyłącznie z IoT Core.
- IoT Core jest jedynym komponentem mającym dostęp do InfluxDB.
- Każdy komponent posiada jedną odpowiedzialność.

---

# 2.6 Założenia architektoniczne

- Architektura jest modularna.
- Komponenty mogą być rozwijane niezależnie.
- Wszystkie elementy uruchamiane są jako kontenery Docker (z wyjątkiem firmware ESP32).
- Możliwe jest późniejsze dodanie nowych urządzeń bez przebudowy backendu.

---

# 2.7 Rozszerzenia poza MVP

Przewidywane kierunki rozwoju:

- obsługa wielu urządzeń,
- Grafana,
- OTA,
- alarmy,
- autoryzacja użytkowników,
- integracje z systemami zewnętrznymi.

Rozszerzenia te nie są częścią wersji 1.0.
