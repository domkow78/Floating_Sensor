
# Rozdział 7 – REST API
**IoT Platform Architecture Specification v1.0**

---

# 7.1 Cel

REST API stanowi jedyny interfejs komunikacyjny pomiędzy IoT Core a aplikacjami klienckimi.

W wersji MVP jedynym klientem API jest aplikacja NiceGUI.

API nie komunikuje się bezpośrednio z urządzeniami ani z brokerem MQTT.

---

# 7.2 Zasady projektowe

- Architektura REST.
- Komunikacja HTTP/HTTPS.
- Format danych: JSON.
- Wersjonowanie od pierwszej wersji.
- Bezstanowość (stateless).
- Jeden endpoint odpowiada za jedną funkcję.

---

# 7.3 Wersjonowanie

Wszystkie endpointy posiadają prefiks:

```
/api/v1/
```

Przykład:

```
GET /api/v1/status
```

Zmiany niekompatybilne będą realizowane przez kolejne wersje API.

---

# 7.4 Architektura

```text
NiceGUI
    │
 HTTP/REST
    │
    ▼
 REST API
    │
    ▼
Processing Engine
    │
    ▼
InfluxDB
```

REST API nie zawiera logiki biznesowej. Odpowiada za udostępnianie danych i walidację żądań.

---

# 7.5 Endpointy MVP

| Metoda | Endpoint | Opis |
|--------|----------|------|
| GET | /api/v1/status | Status platformy |
| GET | /api/v1/device | Informacje o Floating Sensor |
| GET | /api/v1/telemetry/latest | Ostatni pomiar |
| GET | /api/v1/telemetry/history | Historia pomiarów |

Zakres MVP obejmuje wyłącznie operacje odczytu.

---

# 7.6 Standard odpowiedzi

Każda poprawna odpowiedź zwraca kod HTTP oraz dane w formacie JSON.

Przykład:

```json
{
  "status": "success",
  "data": {
    "temperature": 22.3,
    "humidity": 47.9,
    "pressure": 1009.2
  }
}
```

---

# 7.7 Model błędów

Jednolity format odpowiedzi:

```json
{
  "status": "error",
  "code": "RESOURCE_NOT_FOUND",
  "message": "Requested resource does not exist."
}
```

---

# 7.8 Kody HTTP

| Kod | Znaczenie |
|-----|-----------|
| 200 | OK |
| 400 | Niepoprawne żądanie |
| 404 | Nie znaleziono zasobu |
| 500 | Błąd serwera |

---

# 7.9 Kontrakty

## Wejście

- żądanie HTTP,
- parametry zapytania,
- nagłówki.

## Wyjście

- odpowiedź JSON,
- kod HTTP.

REST API nie zwraca danych w innych formatach.

---

# 7.10 Struktura modułu

```text
api/
├── routes/
├── schemas/
├── services/
├── models/
└── errors/
```

Role:

- routes – definicje endpointów,
- schemas – walidacja danych,
- services – komunikacja z IoT Core,
- models – modele odpowiedzi,
- errors – obsługa wyjątków.

---

# 7.11 Zależności

- REST API nie komunikuje się z MQTT.
- Nie zapisuje danych do InfluxDB.
- Korzysta wyłącznie z usług IoT Core.
- Wszystkie odpowiedzi przechodzą przez wspólny mechanizm serializacji.

---

# 7.12 Rozwój po MVP

Możliwe rozszerzenia:

- POST do konfiguracji urządzenia,
- autoryzacja użytkowników,
- tokeny JWT,
- OpenAPI/Swagger,
- filtrowanie i stronicowanie,
- WebSocket dla danych na żywo.

Obecna struktura API umożliwia dodanie tych funkcji bez zmiany istniejących endpointów.
