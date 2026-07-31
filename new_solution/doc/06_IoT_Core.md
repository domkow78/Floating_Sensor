
# Rozdział 6 – IoT Core
**IoT Platform Architecture Specification v1.0**

---

# 6.1 Cel

IoT Core jest centralnym komponentem platformy IoT.

Jego zadaniem jest odbiór danych z urządzeń, ich walidacja, zapis do bazy danych oraz udostępnienie informacji pozostałym komponentom systemu.

IoT Core nie odpowiada za prezentację danych ani za ich pozyskiwanie z czujników.

---

# 6.2 Odpowiedzialność

IoT Core odpowiada za:

- odbiór komunikatów MQTT,
- walidację danych,
- normalizację danych,
- zapis do InfluxDB,
- udostępnienie REST API,
- monitorowanie stanu urządzeń.

---

# 6.3 Architektura logiczna

```text
                 IoT Core
                     │
 ┌───────────────────┼────────────────────┐
 │                   │                    │
 ▼                   ▼                    ▼
MQTT Gateway   Processing Engine    REST API
 │                   │                    │
 └──────────────┬────┴──────────────┬─────┘
                ▼                   ▼
          Device Registry     Storage Service
                │                   │
                └─────────┬─────────┘
                          ▼
                       InfluxDB
```

Każdy moduł posiada jedną odpowiedzialność i komunikuje się wyłącznie przez zdefiniowane interfejsy.

---

# 6.4 Moduły

| Moduł | Odpowiedzialność |
|-------|-------------------|
| MQTT Gateway | Odbiór wiadomości MQTT |
| Processing Engine | Walidacja i przygotowanie danych |
| Device Registry | Informacje o urządzeniach |
| Storage Service | Zapis do InfluxDB |
| REST API | Udostępnianie danych aplikacji |

---

# 6.5 Kontrakty modułów

## MQTT Gateway

### Wejście
- komunikaty MQTT

### Wyjście
- ujednolicony obiekt pomiarowy

---

## Processing Engine

### Wejście
- obiekt pomiarowy

### Wyjście
- zwalidowany rekord danych

Zadania:
- sprawdzenie poprawności pól,
- kontrola wartości wymaganych,
- przygotowanie danych do zapisu.

---

## Storage Service

### Wejście
- rekord danych

### Wyjście
- zapis do InfluxDB

Storage Service nie wykonuje walidacji ani transformacji.

---

## REST API

### Wejście
- zapytania HTTP

### Wyjście
- odpowiedzi JSON

REST API nie komunikuje się bezpośrednio z MQTT.

---

# 6.6 Przepływ danych

```text
MQTT
   │
   ▼
MQTT Gateway
   │
   ▼
Processing Engine
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

Przepływ jest liniowy, co upraszcza diagnostykę i testowanie.

---

# 6.7 Struktura projektu

Proponowana struktura katalogów:

```text
iot-core/
│
├── app/
│   ├── mqtt/
│   ├── processing/
│   ├── storage/
│   ├── api/
│   ├── models/
│   ├── config/
│   └── diagnostics/
│
├── tests/
├── requirements.txt
└── main.py
```

Struktura odzwierciedla architekturę logiczną i powinna pozostać z nią zgodna.

---

# 6.8 Zależności

- MQTT Gateway nie zapisuje danych do bazy.
- REST API nie komunikuje się z brokerem MQTT.
- Storage Service nie zna struktury komunikatów MQTT.
- Processing Engine jest jedynym miejscem transformacji danych.

Takie rozdzielenie ogranicza sprzężenie pomiędzy modułami.

---

# 6.9 Zasady implementacyjne

- Każdy moduł posiada jasno zdefiniowany interfejs.
- Komunikacja pomiędzy modułami odbywa się przez modele danych.
- Logika biznesowa znajduje się wyłącznie w Processing Engine.
- Wszystkie operacje wejścia/wyjścia są odseparowane od logiki przetwarzania.

---

# 6.10 Rozwój po MVP

Architektura umożliwia późniejsze dodanie:

- obsługi wielu urządzeń,
- alarmów,
- harmonogramów,
- kolejek komunikatów,
- autoryzacji użytkowników,
- dodatkowych źródeł danych.

Moduły zostały zaprojektowane tak, aby rozszerzenia nie wymagały przebudowy istniejących interfejsów.
