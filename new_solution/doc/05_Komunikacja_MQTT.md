
# Rozdział 5 – Komunikacja MQTT
**IoT Platform Architecture Specification v1.0**

---

# 5.1 Cel

MQTT jest jedynym protokołem komunikacyjnym pomiędzy urządzeniem Floating Sensor
a platformą IoT. Protokół odpowiada wyłącznie za niezawodny transport komunikatów.

Logika biznesowa nie jest realizowana po stronie urządzenia.

---

# 5.2 Założenia

- Broker: Mosquitto
- Transport: TCP/IP przez WiFi
- Kodowanie danych: JSON
- Jedno urządzenie w MVP
- Architektura przygotowana do obsługi wielu urządzeń

---

# 5.3 Architektura komunikacji

```text
Floating Sensor
      |
 Publish
      |
      v
+----------------+
|   Mosquitto    |
+----------------+
      |
 Subscribe
      |
      v
   IoT Core
```

IoT Core jest jedynym odbiorcą danych z urządzenia.

---

# 5.4 Struktura tematów

Przyjmujemy jednolitą konwencję:

```
floatingsensor/<device_id>/<topic>
```

## Tematy MVP

| Topic | Kierunek | Opis |
|-------|----------|------|
| status | Sensor → IoT Core | Stan urządzenia |
| telemetry | Sensor → IoT Core | Dane pomiarowe |
| diagnostics | Sensor → IoT Core | Informacje diagnostyczne |

Pozostawiamy możliwość rozszerzenia o kolejne tematy bez zmiany obecnej struktury.

---

# 5.5 Identyfikacja urządzenia

Każdy Floating Sensor posiada unikalny `device_id`.

Identyfikator:

- jest niezmienny,
- znajduje się w każdym komunikacie,
- stanowi podstawę identyfikacji urządzenia w IoT Core.

---

# 5.6 Format komunikatów

Wszystkie komunikaty przesyłane są jako JSON.

Przykład komunikatu telemetrycznego:

```json
{
  "device_id": "FS-001",
  "timestamp": "2026-07-29T10:15:00Z",
  "temperature": 22.4,
  "humidity": 48.2,
  "pressure": 1008.4
}
```

Szczegółowa definicja pól zostanie przygotowana podczas implementacji API.

---

# 5.7 QoS i Retain

| Parametr | MVP |
|----------|-----|
| QoS | 1 |
| Retain (telemetria) | Nie |
| Retain (status) | Tak |

Takie ustawienia zapewniają dobrą równowagę pomiędzy niezawodnością a prostotą.

---

# 5.8 Last Will and Testament (LWT)

Każde urządzenie publikuje komunikat LWT.

Cel:

- wykrycie utraty połączenia,
- prezentacja statusu w NiceGUI,
- możliwość późniejszego generowania alarmów.

---

# 5.9 Obsługa błędów

Firmware powinien:

- wykrywać utratę połączenia,
- automatycznie ponawiać połączenie z brokerem,
- nie blokować głównej pętli programu,
- zachować pracę po chwilowych problemach sieciowych.

IoT Core powinien poprawnie obsługiwać brak nowych danych.

---

# 5.10 Kontrakt komunikacyjny

Firmware odpowiada za:

- poprawność danych,
- publikację komunikatów,
- identyfikację urządzenia.

IoT Core odpowiada za:

- odbiór,
- walidację,
- zapis do bazy,
- udostępnienie danych aplikacji.

Takie rozdzielenie odpowiedzialności upraszcza rozwój obu komponentów.

---

# 5.11 Rozszerzenia

Po zakończeniu MVP planowane jest dodanie:

- subskrypcji poleceń (`command`),
- zdalnej konfiguracji,
- OTA,
- szyfrowania i uwierzytelniania MQTT,
- obsługi wielu urządzeń.

Architektura tematów została zaprojektowana tak, aby umożliwić rozwój bez zmiany istniejących komunikatów.
