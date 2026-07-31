
# Rozdział 11 – Struktura repozytorium
**IoT Platform Architecture Specification v1.0**

---

# 11.1 Cel

Repozytorium stanowi centralne miejsce przechowywania kodu źródłowego,
dokumentacji oraz konfiguracji platformy IoT.

Jego struktura odzwierciedla architekturę systemu i powinna pozostać z nią zgodna.

---

# 11.2 Decyzje architektoniczne

Przyjęto następujące zasady:

- jedno repozytorium Git dla całego projektu (monorepo),
- każdy komponent posiada własny katalog,
- dokumentacja rozwijana razem z kodem,
- konfiguracja oddzielona od implementacji,
- testy znajdują się możliwie blisko kodu, którego dotyczą.

---

# 11.3 Struktura główna

```text
iot-platform/
│
├── docs/
├── firmware/
├── iot-core/
├── nicegui/
├── docker/
├── scripts/
├── tests/
├── tools/
├── .gitignore
├── README.md
└── LICENSE
```

Każdy katalog odpowiada jednemu obszarowi odpowiedzialności.

---

# 11.4 Dokumentacja

```text
docs/
│
├── architecture/
├── hardware/
├── firmware/
├── api/
├── deployment/
└── decisions/
```

Dokumentacja jest traktowana jako część produktu i podlega wersjonowaniu.

---

# 11.5 Firmware

```text
firmware/
│
├── src/
├── include/
├── lib/
├── test/
├── platformio.ini
└── README.md
```

Projekt firmware pozostaje niezależny od backendu.

---

# 11.6 IoT Core

```text
iot-core/
│
├── app/
├── tests/
├── requirements.txt
├── pyproject.toml
└── README.md
```

---

# 11.7 NiceGUI

```text
nicegui/
│
├── pages/
├── components/
├── services/
├── models/
├── static/
├── assets/
└── app.py
```

---

# 11.8 Docker

```text
docker/
│
├── compose.yaml
├── .env.example
├── mosquitto/
├── influxdb/
├── iot-core/
└── nicegui/
```

Plik `.env.example` zawiera wyłącznie przykładową konfigurację.

---

# 11.9 Scripts i Tools

## scripts/

Skrypty wspomagające:

- uruchamianie środowiska,
- tworzenie kopii zapasowych,
- eksport/import danych.

## tools/

Narzędzia deweloperskie:

- generatory,
- konwertery,
- narzędzia diagnostyczne.

---

# 11.10 Testy

Testy powinny obejmować:

- firmware,
- IoT Core,
- REST API,
- integrację komponentów.

Testy integracyjne mogą znajdować się również w katalogu głównym `tests/`.

---

# 11.11 Zasady nazewnictwa

- katalogi: małe litery,
- separator: `-` dla nazw katalogów głównych, `_` tylko gdy wymagany przez narzędzie,
- pliki Python: `snake_case.py`,
- klasy: `PascalCase`,
- moduły: jedna odpowiedzialność.

---

# 11.12 Strategia Git

Model pracy:

- `main` – stabilna wersja,
- krótkie gałęzie funkcjonalne (`feature/...`),
- Pull Request przed scaleniem,
- opisowe komunikaty commitów.

Przykłady:

```
feature/mqtt-client
feature/influx-storage
fix/wifi-reconnect
docs/chapter-08
```

---

# 11.13 Konfiguracja

W repozytorium nie przechowuje się:

- haseł,
- tokenów,
- kluczy API,
- danych produkcyjnych.

Konfiguracja środowiskowa znajduje się poza kodem.

---

# 11.14 Rozwój po MVP

Struktura umożliwia dodanie:

- kolejnych urządzeń,
- nowych usług backendowych,
- aplikacji mobilnej,
- modułów AI,
- narzędzi CI/CD.

Obecna organizacja repozytorium została zaprojektowana z myślą o długoterminowym rozwoju projektu bez konieczności reorganizacji katalogów.
