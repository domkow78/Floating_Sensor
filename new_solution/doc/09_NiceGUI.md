
# Rozdział 9 – NiceGUI
**IoT Platform Architecture Specification v1.0**

---

# 9.1 Cel

NiceGUI jest warstwą prezentacji platformy IoT.

Odpowiada za wizualizację danych, konfigurację systemu oraz prezentację stanu urządzenia.
Cała logika biznesowa pozostaje w IoT Core.

---

# 9.2 Decyzje architektoniczne

Przyjęto następujące decyzje:

- NiceGUI jest jedyną aplikacją webową w MVP.
- Komunikacja odbywa się wyłącznie przez REST API.
- NiceGUI nie komunikuje się z MQTT.
- NiceGUI nie komunikuje się bezpośrednio z InfluxDB.
- Interfejs nie zawiera logiki przetwarzania danych.

---

# 9.3 Architektura

```text
             User
              │
              ▼
         Web Browser
              │
              ▼
           NiceGUI
              │
        REST API Client
              │
              ▼
          IoT Core API
```

Warstwa prezentacji pozostaje całkowicie odseparowana od backendu.

---

# 9.4 Struktura aplikacji

```text
nicegui/
│
├── app.py
├── pages/
├── components/
├── services/
├── models/
├── config/
├── static/
└── assets/
```

Każdy katalog odpowiada jednej odpowiedzialności.

---

# 9.5 Moduły

| Moduł | Odpowiedzialność |
|-------|-------------------|
| Pages | Widoki aplikacji |
| Components | Wielokrotnego użytku komponenty GUI |
| Services | Komunikacja z REST API |
| Models | Modele danych interfejsu |
| Config | Konfiguracja aplikacji |

---

# 9.6 Strony MVP

## Dashboard

Prezentuje:

- aktualne wartości pomiarów,
- status połączenia,
- czas ostatniego pomiaru.

---

## Historia

Umożliwia:

- przegląd danych historycznych,
- wybór zakresu czasu.

---

## Status systemu

Prezentuje:

- stan IoT Core,
- stan MQTT,
- stan InfluxDB,
- stan Floating Sensor.

---

## Informacje

Wyświetla:

- wersję aplikacji,
- wersję firmware,
- identyfikator urządzenia,
- informacje diagnostyczne.

---

# 9.7 Przepływ danych

```text
REST API
     │
     ▼
 Service Layer
     │
     ▼
 UI Models
     │
     ▼
 Components
     │
     ▼
 Pages
```

Warstwa Services jest jedynym miejscem komunikacji z backendem.

---

# 9.8 Model danych

GUI wykorzystuje własne modele danych.

Nie operuje bezpośrednio na odpowiedziach REST API.

Przykład:

REST API

↓

TelemetryResponse

↓

TelemetryModel

↓

UI Component

Pozwala to oddzielić interfejs użytkownika od zmian w backendzie.

---

# 9.9 Kontrakty

## Wejście

- odpowiedzi REST API

## Wyjście

- prezentacja danych użytkownikowi

NiceGUI nie wykonuje zapisu danych do platformy w MVP.

---

# 9.10 Zależności

- NiceGUI zna wyłącznie REST API.
- Nie zna struktury InfluxDB.
- Nie zna MQTT.
- Nie komunikuje się z urządzeniem.

Takie podejście pozwala rozwijać frontend niezależnie od backendu.

---

# 9.11 Standard komponentów

Każdy komponent powinien:

- posiadać jedną odpowiedzialność,
- być możliwy do ponownego wykorzystania,
- korzystać wyłącznie z modeli danych,
- nie wykonywać zapytań HTTP.

Zapytania HTTP realizowane są wyłącznie przez warstwę Services.

---

# 9.12 Rozwój po MVP

Architektura umożliwia późniejsze dodanie:

- logowania użytkowników,
- panelu administracyjnego,
- konfiguracji urządzeń,
- wykresów Grafana,
- WebSocket dla danych na żywo,
- wielu urządzeń.

Obecny podział warstw umożliwia rozwój bez przebudowy aplikacji.
