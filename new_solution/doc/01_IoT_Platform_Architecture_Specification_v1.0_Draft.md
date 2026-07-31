
# IoT Platform Architecture Specification
**Version:** 1.0.0  
**Status:** Draft

---

# 1. Wprowadzenie

## 1.1 Cel dokumentu

Niniejszy dokument opisuje architekturę pierwszej wersji platformy IoT oraz pierwszego urządzenia referencyjnego – **Floating Sensor**.

Dokument stanowi podstawę implementacji i obejmuje wyłącznie zakres wymagany do zbudowania pierwszego działającego prototypu (MVP).

## 1.2 Cel projektu

Celem projektu jest zbudowanie kompletnego łańcucha przetwarzania danych od autonomicznego modułu pomiarowego do aplikacji użytkownika.

## 1.3 Wizja projektu

Platforma IoT ma umożliwiać budowę autonomicznych modułów pomiarowych współpracujących z centralnym systemem zarządzania.

Pierwszą implementacją platformy jest **Floating Sensor**.

---

# 2. Floating Sensor

## 2.1 Definicja

Floating Sensor jest autonomicznym modułem pomiarowym przeznaczonym do prowadzenia pomiarów środowiskowych wewnątrz urządzeń, maszyn oraz zamkniętych przestrzeni.

Urządzenie ma postać zamkniętej kuli, którą można swobodnie umieścić w badanym środowisku bez konieczności montażu.

Floating Sensor jest pierwszym urządzeniem referencyjnym platformy IoT.

## 2.2 Architektura sprzętowa

Pierwsza wersja urządzenia wykorzystuje:

- ESP32-S3
- BME688
- LSM6DSV
- Akumulator
- Układ ładowania
- Stabilizator 3.3 V

## 2.3 Funkcje urządzenia

- pomiar temperatury,
- pomiar wilgotności,
- pomiar ciśnienia,
- pomiar jakości powietrza,
- pomiar ruchu i orientacji,
- komunikacja WiFi,
- komunikacja MQTT.

---

# 3. Zakres MVP

Wersja 1.0 obejmuje:

- Floating Sensor,
- MQTT,
- Mosquitto,
- IoT Core,
- InfluxDB,
- NiceGUI,
- Docker.

Poza zakresem pozostają:

- Grafana,
- OTA,
- obsługa wielu urządzeń,
- role użytkowników,
- AI,
- integracje z systemami zewnętrznymi.

---

# 4. Architektura systemu

```
Floating Sensor
      │
      ▼
    MQTT
      ▼
 Mosquitto
      ▼
   IoT Core
      ▼
  InfluxDB
      ▼
   NiceGUI
```

---

# 5. Zasady projektowe

1. Najpierw powstaje działający prototyp.
2. Dokumentacja wyprzedza implementację o jeden krok.
3. Architektura rozwijana jest ewolucyjnie.
4. Nie implementujemy funkcji „na zapas”.
5. Każdy komponent ma jedną odpowiedzialność.

---

# 6. Kryterium zakończenia MVP

MVP uznaje się za zakończone po uzyskaniu stabilnego przepływu:

Floating Sensor → MQTT → Mosquitto → IoT Core → InfluxDB → NiceGUI

---

# 7. Kolejne rozdziały

1. Architektura sprzętowa
2. Firmware ESP32
3. MQTT
4. IoT Core
5. REST API
6. InfluxDB
7. NiceGUI
8. Docker
9. Struktura repozytorium
10. Plan implementacji
